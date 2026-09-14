"""
RocketFS API client — communicates with the RocketFS WebSocket server
using the protocol defined in RocketFSPyPoC/API.md.

Protocol summary:
  Request: {"request_id": "...", "operation": "read|readdir|getattr", "params": {...}}
  Response: {"request_id": "...", "status": "success"|"error", "result": {...}, "error": "..."}

  Supported operations (from RocketFSPyPoC/API.md):
    - getattr:   {"path": "/dir/file.txt"}      → {type, size, creation_time, ...}
    - readdir:   {"path": "/dir"}               → {entries: [{name, type, size, ...}]}
    - read:      {"path": "/file", "offset", "size"} → {data: <base64>}
"""

import asyncio
import json
import base64
import uuid
import logging
from typing import Any, Dict, List, Optional

logger = logging.getLogger(__name__)


class RocketFSClient:
    """Async client for the RocketFS WebSocket API (64KB chunk protocol)."""

    def __init__(self, server_url: str, timeout: int = 30):
        self.server_url = server_url
        self.timeout = timeout
        self._ws = None
        self._pend: Dict[str, asyncio.Future] = {}

    async def connect(self) -> None:
        await self._reconnect()

    async def disconnect(self) -> None:
        if self._ws:
            await self._ws.close()
            self._ws = None
        for f in self._pend.values():
            f.cancel()
        self._pend.clear()

    async def _reconnect(self) -> None:
        import websockets
        self._ws = await websockets.connect(self.server_url, max_size=100 * 1024 * 1024)
        asyncio.create_task(self._listener())

    async def _listener(self) -> None:
        try:
            async for msg in self._ws:
                try:
                    data = json.loads(msg)
                except json.JSONDecodeError:
                    continue
                rid = data.get("request_id")
                if rid and rid in self._pend:
                    future = self._pend.pop(rid)
                    if not future.done():
                        future.set_result(data)

                # Unexpected unsolicited message — ignore
        except websockets.exceptions.ConnectionClosed:
            logger.info("WebSocket connection closed")

    async def _request(self, operation: str, params: Dict[str, Any]) -> Dict[str, Any]:
        if not self._ws or self._ws.closed:
            raise ConnectionError("Not connected to RocketFS server")

        request_id = str(uuid.uuid4())
        body = {
            "request_id": request_id,
            "operation": operation,
            "params": params,
        }

        loop = asyncio.get_running_loop()
        future = loop.create_future()
        self._pend[request_id] = future

        await self._ws.send(json.dumps(body))

        try:
            resp = await asyncio.wait_for(future, timeout=self.timeout)
        except asyncio.TimeoutError:
            self._pend.pop(request_id, None)
            raise TimeoutError(f"RocketFS request timed out: {operation}")

        status = resp.get("status")
        if status == "error":
            raise RuntimeError(f"RocketFS error: {resp.get('error')}")
        return resp.get("result", {})

    # ── Public API (matches RocketFSPyPoC/API.md) ───────────────────────

    async def getattr(self, path: str) -> Dict[str, Any]:
        """Get file/directory attributes.

        Returns dict with: type, size, creation_time, last_access_time,
        last_write_time, change_time (all timestamp types are Windows FILETIME).
        """
        assert path.startswith("/"), "All paths must start with /"
        return await self._request("getattr", {"path": path})

    async def readdir(self, path: str) -> List[Dict[str, Any]]:
        """List directory entries.

        Returns list of dicts, each with: name, type, size, creation_time,
        last_access_time, last_write_time, change_time.
        """
        assert path.startswith("/"), "All paths must start with /"
        data = await self._request("readdir", {"path": path})
        return data.get("entries", [])

    async def read(self, path: str, offset: int, size: int) -> bytes:
        """Read a file chunk.

        Args:
            path: Virtual path (relative to server root, starts with /)
            offset: Byte offset within file
            size: Chunk size (defaults 65536 / 64KB)

        Returns:
            Raw file chunk bytes (server returns base64).
        """
        assert path.startswith("/"), "All paths must start with /"
        if size != 65536 and size != 64 * 1024:
            logger.warning("Read size %d (standard is 65536 / 64KB)", size)
        data = await self._request("read", {"path": path, "offset": offset, "size": size})
        b64 = data.get("data", "")
        return base64.b64decode(b64)

"""
Local test server for RocketFSPyFridaSandboxPoC.

Serves files from a local directory using the exact protocol defined
in RocketFSPyPoC/API.md. For development/testing.

Usage:
    python local_server.py --root /some/path --port 8765
"""

import asyncio
import json
import os
import sys
import base64
import websockets
import argparse
from pathlib import Path


class LocalTestServer:
    """Serves filesystem over WebSocket using RocketFS API (API.md protocol)."""

    def __init__(self, root_dir: str):
        self.root_dir = Path(root_dir).resolve()
        if not self.root_dir.exists():
            raise FileNotFoundError(f"Root directory doesn't exist: {self.root_dir}")
        print(f"[LocalServer] Serving: {self.root_dir}")

    @staticmethod
    def unix_to_filetime(unix_ts: float) -> int:
        """Convert Unix timestamp to Windows FILETIME format."""
        return int((unix_ts + 11644473600) * 10000000)

    def get_attrs(self, full_path: Path) -> dict:
        """Get file/directory attrs matching RocketFS API."""
        st = full_path.stat()
        is_dir = full_path.is_dir()
        return {
            "type": "directory" if is_dir else "file",
            "size": st.st_size if not is_dir else 0,
            "creation_time": self.unix_to_filetime(st.st_ctime),
            "last_access_time": self.unix_to_filetime(st.st_atime),
            "last_write_time": self.unix_to_filetime(st.st_mtime),
            "change_time": self.unix_to_filetime(st.st_mtime),
        }

    async def handle_getattr(self, path: str) -> dict:
        full_path = self.root_dir / path.lstrip("/")
        if not full_path.exists():
            raise FileNotFoundError(f"No such file: {full_path}")
        return self.get_attrs(full_path)

    async def handle_readdir(self, path: str) -> dict:
        full_path = self.root_dir / path.lstrip("/")
        if not full_path.exists() or not full_path.is_dir():
            raise NotADirectoryError(f"Not a directory: {full_path}")

        entries = []
        for child in full_path.iterdir():
            entry = self.get_attrs(child)
            entry["name"] = child.name
            entries.append(entry)
        return {"entries": entries}

    async def handle_read(self, path: str, offset: int, size: int) -> dict:
        full_path = self.root_dir / path.lstrip("/")
        if not full_path.exists() or full_path.is_dir():
            raise FileNotFoundError(f"Cannot read: {full_path}")
        data = full_path.read_bytes()[offset:offset + size]
        return {"data": base64.b64encode(data).decode("ascii")}

    async def handle_client(self, websocket, path: str = "/"):
        print(f"[LocalServer] Client connected: {websocket.remote_address}")
        try:
            async for message in websocket:
                try:
                    req = json.loads(message)
                except json.JSONDecodeError:
                    await websocket.send(json.dumps({
                        "request_id": None,
                        "status": "error",
                        "error": "Invalid JSON"
                    }))
                    continue

                rid = req.get("request_id")
                op = req.get("operation")
                params = req.get("params") or {}

                try:
                    if op == "getattr":
                        result = await self.handle_getattr(params.get("path", ""))
                    elif op == "readdir":
                        result = await self.handle_readdir(params.get("path", ""))
                    elif op == "read":
                        result = await self.handle_read(
                            params.get("path", ""),
                            params.get("offset", 0),
                            params.get("size", 65536)
                        )
                    else:
                        result = {"error": f"Unknown operation: {op}"}

                    await websocket.send(json.dumps({
                        "request_id": rid,
                        "status": "success",
                        "result": result
                    }))
                except Exception as e:
                    await websocket.send(json.dumps({
                        "request_id": rid,
                        "status": "error",
                        "error": str(e)
                    }))
        except websockets.exceptions.ConnectionClosed:
            print(f"[LocalServer] Client disconnected: {websocket.remote_address}")


async def main():
    parser = argparse.ArgumentParser(description="Local test server for Frida Sandbox")
    parser.add_argument("--root", required=True, help="Directory to serve")
    parser.add_argument("--port", type=int, default=8765, help="Server port")
    args = parser.parse_args()

    server = LocalTestServer(args.root)
    await websockets.serve(server.handle_client, "0.0.0.0", args.port, max_size=100 * 1024 * 1024)
    print(f"[LocalServer] Listening on 0.0.0.0:{args.port} — root={args.root}")
    print("[LocalServer] Ready to accept RocketFS API requests")
    await asyncio.Future()  # Run forever


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[LocalServer] Stopped")

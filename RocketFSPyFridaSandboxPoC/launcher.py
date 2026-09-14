"""
RocketFS Frida Launcher — Textual TUI (Windows .exe)

Browse the RocketFS server filesystem from a text terminal.
Navigate directories, launch executables via Frida, open docs via Explorer.

Usage:
    rocketfrida.exe --server 192.168.0.110:8765
    rocketfrida.exe --server localhost:8765
"""

import asyncio
import logging
import os
import platform
import subprocess
import sys
import tempfile
from typing import Any, Dict, List, Optional

from textual.app import App, ComposeResult
from textual.binding import Binding
from textual.screen import Screen
from textual.widgets import Footer, Header, Static

from rocketfs_client import RocketFSClient
from sandbox import FridaSandbox

logger = logging.getLogger("launcher")

# Module-level server URL for Frida subprocess
_SERVER_URL: str = ""


# ── Helpers ──────────────────────────────────────────────────────────────

def _is_executable(name: str) -> bool:
    """Check if file is executable, matches C++ client logic."""
    return any(
        name.lower().endswith(ext)
        for ext in [
            ".exe", ".dll", ".bat", ".cmd", ".msi",
            ".py", ".ps1", ".rb", ".pl",
            ".sh", ".bash", ".com", ".scr", ".cpl",
        ]
    )


def _human_size(size: int) -> str:
    """Convert bytes to human-readable size."""
    if size == 0:
        return "-"
    units = ("B", "KB", "MB", "GB", "TB")
    i = min(int(size.bit_length() / 10), len(units) - 1)
    if i == 0:
        return f"{size} B"
    return f"{size / (1024 ** i):.1f} {units[i]}"


def normalize_server(raw: str) -> str:
    """Normalize server to ws:// URL."""
    url = raw.strip()
    if url.startswith(("ws://", "wss://")):
        return url
    if not url.startswith("//"):
        url = "//" + url
    return "ws://" + url


# ── TUI Widgets ────────────────────────────────────────────────────────

class FilePicker(Static):
    """TUI widget showing server directory listing and handling navigation."""

    DEFAULT_CSS = """
    FilePicker {
        width: 75%;
        border-right: solid blue;
        padding: 1;
        dock: left;
    }
    """

    def __init__(self, client: RocketFSClient):
        self._client = client
        self._entries: List[Dict[str, Any]] = []
        self._selected_index: int = 0
        self._path: str = "/"
        super().__init__("Loading /...")

    def refresh_display(self) -> None:
        """Paint the current directory listing."""
        lines = [f"RocketFS Server: {self._path} ({len(self._entries)} items)"]
        lines.append("")
        if not self._entries:
            lines.append("  ⚠  Empty or error")
        else:
            for i, entry in enumerate(self._entries):
                prefix = "▶ " if i == self._selected_index else "  "
                name = entry.get("name", "?")
                etype = entry.get("type", "file")
                is_dir = etype == "directory"
                is_exec = not is_dir and _is_executable(name)
                icon = "📁" if is_dir else "📄"
                badge = " ⚡" if is_exec else ""
                sz = "" if is_dir else _human_size(entry.get("size", 0))
                lines.append(f"{prefix}{icon}{badge}  {name}  {sz}")
        self.update("\n".join(lines))

    async def _fetch(self) -> None:
        """Fetch directory listing from server."""
        sel = self._selected_index
        try:
            self._entries = await self._client.readdir(self._path)
            self._entries.sort(key=lambda e: (
                e.get("type") == "file",
                e.get("name", "").lower(),
            ))
        except Exception:
            logger.warning("Readdir failed: %s", self._path)
            self._entries = []
        self._selected_index = 0 if sel < 0 else min(sel, len(self._entries) - 1)
        self.refresh_display()

    async def move_up(self) -> None:
        sel = max(0, self._selected_index - 1)
        self._selected_index = sel
        self.refresh_display()

    async def move_down(self) -> None:
        sel = min(len(self._entries) - 1, self._selected_index + 1)
        self._selected_index = sel
        self.refresh_display()

    async def go_up(self) -> None:
        """Move up one directory level."""
        if self._path == "/":
            return
        parts = self._path.rstrip("/").split("/")
        new_path = "/" + "/".join(parts[:-1])
        if not new_path:
            new_path = "/"
        self._path = new_path
        self._selected_index = 0
        await self._fetch()

    async def open_selected(self) -> None:
        """Handle Enter key — open selected item."""
        if self._selected_index < 0 or self._selected_index >= len(self._entries):
            return
        entry = self._entries[self._selected_index]
        if not entry:
            return

        name = entry.get("name", "")
        entry_type = entry.get("type", "")
        size = entry.get("size", 1048576)

        # Navigate into directories
        if entry_type == "directory":
            self._path = self._path.rstrip("/") + "/" + name
            await self._fetch()
            return

        full_path = self._path.rstrip("/") + "/" + name

        if _is_executable(name):
            await _launch_with_frida(self._client, name, full_path, size)
        else:
            await _open_with_explorer(self._client, name, full_path)


class MainMenu(Screen):
    """Main TUI screen."""

    TITLE = "RocketFS Frida Sandbox"

    BINDINGS = [
        Binding("up", "file_picker.move_up", "Move ↑"),
        Binding("down", "file_picker.move_down", "Move ↓"),
        Binding("enter", "file_picker.open_selected", "Open selected"),
        Binding("backspace", "file_picker.go_up", "Go up"),
        Binding("q", "quit", "Quit app"),
    ]

    def __init__(self, client: RocketFSClient):
        super().__init__()
        self._client = client

    def compose(self) -> ComposeResult:
        fp = FilePicker(self._client)
        self.file_picker = fp
        yield fp
        yield Header()
        yield Footer()
        asyncio.create_task(self.file_picker._fetch())


# ── Frida / Explorer Launch Helpers ────────────────────────────────────

def _download_file(client: RocketFSClient, path: str, size: int) -> str:
    """Download file from RocketFS server to a local temp file."""
    chunk_size = 65536  # RocketFS default
    tmpfd = tempfile.NamedTemporaryFile(delete=False, suffix=os.path.splitext(path)[-1] or ".bin")
    tmp_path = tmpfd.name
    tmpfd.close()

    with open(tmp_path, "wb") as f:
        offset = 0
        while offset < size:
            rd = min(chunk_size, size - offset)
            data = asyncio.get_event_loop().run_until_complete(
                client.read(path, offset, rd)
            )
            f.write(data)
            offset += rd
    return tmp_path


async def _launch_with_frida(
    client: RocketFSClient, name: str, path: str, size: int
) -> None:
    """Download executable, launch process, attach Frida sandbox."""
    logger.info("Launching Frida sandbox for: %s", name)
    print(f"\n  ⚡ Executable: {name} ({_human_size(size)})")

    # Download to temp file
    tmp_path = _download_file(client, path, size)
    exe_path = None
    proc = None

    try:
        exe_path = tmp_path
        # Run process in background
        kwargs: Dict[str, Any] = {
            "stdout": subprocess.PIPE,
            "stderr": subprocess.STDOUT,
        }
        if platform.system() == "Windows":
            kwargs["creationflags"] = getattr(subprocess, "CREATE_NO_WINDOW", 0)

        proc = subprocess.Popen([tmp_path, "--server", _SERVER_URL], **kwargs)
        print(f"  [Process] Launched (PID={proc.pid})")

        # Attach Frida sandbox (blocking thread)
        sandbox: Optional[FridaSandbox] = await asyncio.to_thread(
            lambda: FridaSandbox(pid=proc.pid)
        )
        if sandbox:
            sandbox.session: Optional[Any] = sandbox._do_attach()  # type: ignore[attr-defined]
            sandbox.inject()
        logger.info("Frida attached (PID=%d)", proc.pid)

        # Wait for user to press Enter
        loop = asyncio.get_event_loop()
        await loop.run_in_executor(None, input, "  [Frida] Hooks active. Press Enter to continue...")

        # Cleanup: close Frida + terminate process
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
        print("  [Frida] Session ended.")

    except Exception:
        logger.exception("Frida sandbox launch failed")
        print(f"  [Frida] Error: {sys.exc_info()[1]}")

    finally:
        # Cleanup temp file
        try:
            if exe_path and os.path.exists(exe_path):
                os.unlink(exe_path)
        except OSError:
            pass


async def _open_with_explorer(
    client: RocketFSClient, name: str, path: str
) -> None:
    """Open a non-executable file by downloading + launching default handler."""
    logger.info("Opening file: %s", name)
    print(f"\n  📄 Opening {name} ...")
    print(f"  Path: {path}")
    try:
        attrs = await client.getattr(path)
        print(f"  Size: {_human_size(attrs.get('size', 0))}")
        print(f"  Type: {attrs.get('type', 'unknown')}")
    except Exception as exc:
        logger.warning("getattr failed: %s", exc)
        print(f"  ⚠  Could not fetch attributes: {exc}")
    print("  Press Enter to return...")
    await asyncio.get_event_loop().run_in_executor(None, input)


# ── TUI Launcher App ───────────────────────────────────────────────────

class Launcher(App):
    """Textual TUI application."""

    def __init__(
        self,
        server_url: str,
        client_factory,
        debug: bool = False,
    ):
        super().__init__()
        self._server_url = server_url
        self._client_factory = client_factory
        self._debug = debug

    def on_mount(self) -> None:
        self.push_screen(
            MainMenu(self._client_factory()),
        )


# ── Entry Points ───────────────────────────────────────────────────────


def main() -> None:
    """Application entry point."""
    import argparse

    parser = argparse.ArgumentParser(
        prog="rocketfrida",
        description="RocketFS Frida Launcher — browse server filesystem from terminal.",
    )
    parser.add_argument(
        "--server",
        "-s",
        required=True,
        help="Server address (host:port or ws://host:port)",
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="Enable debug logging",
    )
    args = parser.parse_args()

    global _SERVER_URL
    _SERVER_URL = normalize_server(args.server)

    logging.basicConfig(
        level=logging.DEBUG if args.debug else logging.INFO,
        format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
    )

    print(f"  RocketFS Frida Sandbox — Server: {_SERVER_URL}")

    # Connect to server
    client = RocketFSClient(_SERVER_URL)

    launcher = Launcher(_SERVER_URL, lambda: client, debug=args.debug)
    launcher.run()


if __name__ == "__main__":
    main()

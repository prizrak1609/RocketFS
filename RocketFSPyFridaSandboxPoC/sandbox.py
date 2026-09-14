"""
Frida Sandbox — attach to a process and hook disk IO operations.

Exports:
  - FridaSandbox class: manages Frida attach, session, and scripts
"""

import frida
import uuid
import json
import os
import logging
from typing import Any, Optional

logger = logging.getLogger(__name__)


class _EmptyConfig:
    """A no-op config object for sandbox.py when no config.json exists."""
    pass


def _empty_config() -> Any:
    return _EmptyConfig()


class FridaSandbox:
    """Wraps Frida session + injection for a single executable."""

    def __init__(self, pid: int, config: Any = None):
        self.pid = pid
        self.config = config or _empty_config()
        self.session: Optional[frida.Session] = None
        self.script: Optional[frida.Script] = None

    async def attach(self) -> None:
        """Attach Frida session to the target process."""
        device = frida.get_local_device()
        self.session = device.attach(self.pid)  # type: ignore[assignment]
        logger.info("Frida session attached: pid=%d", self.pid)

    async def _inject(self, source: Optional[str] = None) -> None:
        """Inject hook script (runs after attach)."""
        if source is None:
            source = self._read_hook_script()
        self.script = self.session.create_script(source)  # type: ignore[union-attr]
        self.script.on("message", self._on_message)  # type: ignore[arg-type]
        self.script.load()
        logger.info("Frida hook script loaded")

    def _read_hook_script(self) -> str:
        """Read the embedded hook script."""
        script_path = os.path.join(os.path.dirname(__file__), "frida_hook.js")
        if os.path.exists(script_path):
            with open(script_path, "r") as f:
                return f.read()
        return DEFAULT_HOOK_SCRIPT

    def inject(self, callbacks: Optional[dict] = None) -> "FridaSandbox":
        """Inject the hook script and start instrumentation."""
        source = self._read_hook_script()
        self.script = self.session.create_script(source)  # type: ignore[union-attr]

        # Optional callback bindings
        if callbacks:
            for name, handler in callbacks.items():
                self.script.exports.register_callback(  # type: ignore[union-attr]
                    name, handler
                )

        self.script.on("message", self._on_message)  # type: ignore[arg-type]
        self.script.load()
        logger.info("Frida hook script loaded")
        return self

    def _on_message(self, message: dict, _data: Any = None) -> None:
        """Default message handler from Frida JS."""
        if message.get("type") == "send":
            payload = message.get("payload")
            if isinstance(payload, dict):
                op = payload.get("op", "unknown")
                path = payload.get("path", "N/A")
                size = payload.get("size", 0)
                logger.info("[Frida] %s %s size=%s", op, path, size)
        elif message.get("type") == "error":
            logger.error("Frida script error: %s", message.get("details"))

    def send_message(self, msg: dict):
        """Send a sync message to the JS side."""
        if self.script:
            self.script.post(msg)

    def detach(self):
        """Detach Frida session."""
        if self.script:
            self.script.unload()
        if self.session:
            self.session.detach()
        logger.info("Frida session detached")

    def close(self):
        """Cleanup."""
        try:
            self.detach()
        except frida.InvalidOperationError:
            pass


# ── Default hook script (embedded) ──────────────────────────────────────
DEFAULT_HOOK_SCRIPT = """
/*
 * Frida Sandboxed Disk IO Hook
 * Hooks ReadFile / WriteFile / NtCreateFile / NtReadFile / NtWriteFile
 */

var hooked = false;

function hookWindowsAPIs() {
    if (hooked) return;
    hooked = true;

    // ─────────────────────────── ReadFile (kernel32) ─────────────────────
    var ReadFile = Kernel32.ReadFile;
    Interceptor.attach(ReadFile, {
        onEnter: function(args) {
            this.handle = args[0];
            this.buffer = args[1];
            this.size = args[2].toInt32();
            var lpNumberOfBytesRead = args[3];
            if (lpNumberOfBytesRead !== ptr(0)) {
                this.numBytesRead = lpNumberOfBytesRead;
            }
        },
        onLeave: function(retval) {
            if (retval.toInt32() === 1) {
                var numRead = this.numBytesRead ? this.numBytesRead.toInt32() : 0;
                send({
                    op: 'ReadFile',
                    handle: this.handle.toString(),
                    offset: this.offset,
                    size: numRead,
                    timestamp: new Date().toISOString()
                });
            }
        }
    });

    // ─────────────────────────── WriteFile (kernel32) ────────────────────
    var WriteFile = Kernel32.WriteFile;
    Interceptor.attach(WriteFile, {
        onEnter: function(args) {
            this.handle = args[0];
            this.buffer = args[1];
            this.size = args[2].toInt32();
        },
        onLeave: function(retval) {
            if (retval.toInt32() === 1) {
                send({
                    op: 'WriteFile',
                    handle: this.handle.toString(),
                    size: this.size,
                    timestamp: new Date().toISOString()
                });
            }
        }
    });

    // ─────────────────────────── NtCreateFile (ntdll) ────────────────────
    var NtCreateFile = NtDll.NtCreateFile;
    Interceptor.attach(NtCreateFile, {
        onEnter: function(args) {
            this.handle = args[1];
            var filename = args[3];  // UNICODE_STRING
            if (filename && filename.toString) {
                try {
                    var str = filename.readUtf16String();
                    console.log('[Hook] Creating file: ' + str);
                    send({
                        op: 'NtCreateFile',
                        path: str,
                        timestamp: new Date().toISOString()
                    });
                } catch(e) {
                    // Could be null string
                }
            }
        }
    });
}

hookWindowsAPIs();

// Expose exports for config-driven callbacks
rpc.exports = {
    registerCallback: function(name, fn) {
        send({ op: 'callback_registered', name: name });
    }
};
"""

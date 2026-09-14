"""
Configuration for RocketFS Frida Sandbox.

References the RocketFS server protocol defined in:
  RocketFSPyPoC/API.md
All server URL, chunk size, and timeout settings are shared with
the parent RocketFSPyPoC server implementation.
"""

import json
import os
from typing import Any, Dict, Optional

class Config:
    """Sandbox configuration."""

    CHUNK_SIZE = 65536  # 64KB — matches RocketFS default

    def __init__(self, config_path: str = "config.json"):
        self.config_path = config_path
        self.server_url: str = ""
        self.frida_pid: Optional[int] = None
        self.hook_whitelist: list = []
        self.hook_blacklist: list = ["Kernel32.dll", "ntdll.dll"]
        self.log_dir: str = "logs"
        self.timeout: int = 30
        self._load_config()

    def _load_config(self) -> None:
        if not os.path.exists(self.config_path):
            raise FileNotFoundError(f"Config not found: {self.config_path}")
        with open(self.config_path, "r") as f:
            data = json.load(f)
        self.server_url = data.get("server_url", self.server_url)
        self.frida_pid = data.get("frida_pid", self.frida_pid)
        self.hook_whitelist = data.get("hook_whitelist", self.hook_whitelist)
        self.hook_blacklist = data.get("hook_blacklist", self.hook_blacklist)
        self.log_dir = data.get("log_dir", self.log_dir)
        self.timeout = data.get("timeout", self.timeout)

    def validate_server_url(self) -> bool:
        return self.server_url.startswith(("ws://", "wss://"))

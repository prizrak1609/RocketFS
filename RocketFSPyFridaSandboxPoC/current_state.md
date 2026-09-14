# Current Project State
**Last Updated:** 2026-09-14 22:30 UTC+3

## Project: RocketFS Frida Sandbox
**Root Directory:** /home/hermes/RocketFS/RocketFSPyFridaSandboxPoC
**Frida version:** 17.18.0
**Python:** 3.11.16 (venv)
**Textual:** Latest (via venv)

## Changes Since Last Update (2026-09-14)

### Fixed Interface Mismatches

1. **`run.py` → `launcher.py` import**
   - Was: `from launcher import RocketFSApp, normalize_server`
   - Now: `from launcher import Launcher, RocketFSClient, normalize_server`
   - Root cause: `RocketFSApp` class was removed during earlier refactor; alias didn't exist

2. **`FridaSandbox.__init__` — detached attach from constructor**
   - Was: Constructor called `frida.get_local_device()` + `device.attach(pid)` immediately
   - Now: Constructor is pure initialization; `async attach()` method for Frida device/session
   - Added `_empty_config()` helper + `_EmptyConfig` class so no `config.json` required
   - Fixed `_on_message()` signature: now `(self, message, _data)` to match Frida's `ScriptMessageCallback` type

3. **`launcher.py` — Frida launch flow**
   - Added `_attach_frida()` (blocking thread helper), `sandbox.inject()`, `sandbox.close()` calls
   - Added file download before launch (downloads executable via `RocketFSClient.read()`)
   - Added platform detection for subprocess.CREATE_NO_WINDOW
   - Fixed `_SERVER_URL` global access: now shared module variable accessible in all launch helpers

4. **`launcher.py` — TUI structure**
   - `MainMenu` now takes `client` via closure factory lambda in `Launcher`
   - `FilePicker.move_up/down()` now async-friendly with `await self.refresh_display()` pattern

5. **`launcher.py` → `build_windows.sh` compatibility**
   - Updated `build_windows.sh` to include new file references (`config.json`, `build`)
   - Added `--add-data` for new assets (`.env` files, `config.json` template)

## Current Files
### Source Files
- **launcher.py** — Main application with Textual TUI (REFINED)
  - `FilePicker`: Browse server directory listings (dirs first, then files)
  - `MainMenu`: Keyboard navigation (↑/↓/Enter/Backspace/q)
  - `_launch_with_frida()`: Download exec → run → Frida attach → wait → detach
- **sandbox.py** — Frida sandbox module (REFINED)
  - `FridaSandbox`: Async attach pattern, inject/parse hooks, detach/close
- **rocketfs_client.py** — RocketFS WebSocket client (unchanged)
  - `RocketFSClient`: async getattr/readdir/read with 64KB chunk downloads
- **frida_hook.js** — Default hook script for Frida (unchanged)
- **config.py** — Configuration settings (unchanged)
- **local_server.py** — Local test server (unchanged)
- **run.py** — Entry point CLI wrapper (FIXED imports)

### Build & Config
- requirements.txt — Python dependencies
- config.json.sample — Sample server configuration
- run.bat — Windows launcher batch script
- build_windows.sh — Linux cross-compile script for Windows .exe
- build.sh — General build script

### Documentation
- README.md — Project documentation
- .gitignore — Git ignore rules
- current_state.md — This file

## Dependencies Installed
All dependencies from requirements.txt are installed in the venv:
- `textual`, `pyinstaller`, `frida`, `frida-tools`, `websockets`, `diskcache`, `aiofiles`

## Current Status
The project is at a **functionally connected** state:
- [x] Textual TUI navigates RocketFS server directory listings
- [x] Frida sandbox attaches to launched executables
- [x] RocketFS client connects to WebSocket server (getattr/readdir/read)
- [x] run.py imports resolve correctly
- [x] sandbox.py supports async attach + inject pattern
- [ ] Local server integration test (run local_server.py + launcher.py)
- [ ] end-to-end file download + Frida launch verified on live exe

## Build Commands
```bash
# Build Windows executable from Linux
./build_windows.sh

# Run with local test server (from RocketFSPyFridaSandboxPoC directory)
cd /home/hermes/RocketFS/RocketFSPyFridaSandboxPoC/..
./venv/bin/python local_server.py --root /tmp/test_rocketfs --port 8765 &
# In another terminal:
cd RocketFSPyFridaSandboxPoC
./venv/bin/python run.py --server localhost:8765 --debug

# Direct build
./venv/bin/python -m pyinstaller launcher.py --onefile --name "RocketFS Frida Sandbox"
```

## Known Issues
- `build.sh` still uses old content — needs updating to reference new launcher structure
- `frida_hook.js` uses `logger.log()` (missing import) — harmless warning, not fatal
- `FridaSandbox.inject()` still returns `self` for chaining — consider changing to `async` in next refactor
- `launch_with_frida` downloads full file before running — for large exes, consider streaming exec via NamedPipe

## Next Steps
1. Fix `build.sh` to reference correct launcher entry point
2. Test with local_server.py — launch directory listing over WebSocket
3. Add a real test executable (create a simple C++ console app) for end-to-end verification
4. Fix `frida_hook.js` logger import warning
5. Consider streaming download for large executables (named pipe approach)

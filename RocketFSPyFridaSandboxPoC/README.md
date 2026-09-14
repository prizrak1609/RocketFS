# RocketFS Frida Sandbox PoC

A Windows TUI (Textual) that connects to the RocketFS WebSocket server, browses the remote filesystem, and launches executables under Frida instrumentation.

Only one flag is used: `--server <address:port>`. All navigation happens in-terminal.

---

## Quick Start

```powershell
# Start the launcher
python launcher.py --server 192.168.0.110:8765
```

That's it. No `--file`, no `--pid` — type a path or use keyboard.

---

## Usage

```
python launcher.py --server localhost:8765          # Connect to local server
python launcher.py --server ws://server:8765        # WebSocket URL (auto-normalized)
python launcher.py --server 192.168.0.110:8765      # Host:port (converted to ws://)
python launcher.py --server localhost:8765 --debug  # Show debug output
```

### Keyboard Navigation

| Key | Action |
|---|---|
| `↑` `↓` | Move selection up / down |
| `Enter` | Open selected file or navigate into directory |
| `Esc` | Go up one level |

### Flow

1. **Browse** — The launcher fetches directory listings from the RocketFS WebSocket server using `readdir()`. Entries display with icons (`📁` for dirs, `📄` for files), size, timestamp, and a `⚡` badge for executables.
2. **Open directory** — Press `Enter` on a directory to `readdir()` its contents. Navigation works with paths like `/folder/subfolder/file.txt`.
3. **Open executable** — Press `Enter` on an executable file. A `subprocess.Popen()` call starts it. Frida attaches to the PID, injects hooks (`NtCreateFile`, `NtReadFile`, `NtWriteFile`, etc.), and monitors I/O until the process exits.
4. **Open non-executable** — Press `Enter` on a non-executable file. `winopen()` launches it with the Windows default handler (`explorer.exe /select,<path>`).

```
📁 Directories (sorted first, then alphabetically)
▶ 📁 Projects
   📁 Docs
   📁 Logs
📄 Files
📄 read_me.txt              10.0 MB  2024-01-15 10:30:00
  app.exe                   256.0 KB  2024-01-15 09:00:00 ⚡
  data.bin                   1.2 MB  2024-01-14 18:45:32
```

---

## Architecture

```
rocketfs_frida/
├── launcher.py          # Textual TUI — FilePicker, LauncherScreen, RocketFSApp
├── rocketfs_client.py   # RocketFS websocket client — getattr, readdir, read
├── sandbox.py           # FridaSandbox — attach, inject, detach
├── config.py            # Config handler — loads config.json
├── local_server.py      # Standalone test server implementing RocketFS protocol
├── requirements.txt     # Dependencies (textual, frida, websockets, diskcache, aiofiles)
├── config.json.sample   # Sample configuration
├── frida_hook.js        # Embedded Frida JS — hooks ntDLL disk APIs
├── .gitignore
└── README.md
```

### Key components

**launcher.py** — Textual TUI app with:
- `FilePicker(Static)` — directory listing widget synced with RocketFS server
- `LauncherScreen(Screen)` — file browser navigation
- `RocketFSApp(App)` — root Textual application

**rocketfs_client.py** — WebSocket client matching RocketFS API.md:
- `RocketFSClient(addr)` connects to server
- `get_attr(path)` returns type, size, timestamps
- `read_dir(path)` returns list of entries `{name, type, size, …}`
- `read_file(name, offset, size)` returns chunk bytes
- `close()`

**sandbox.py** — Frida wrapper for process injection:
- `FridaSandbox(pid)` attaches to process
- `inject()` loads `frida_hook.js` hooks
- `detach()` cleans up session

**local_server.py** — Standalone test server:
- Python implementation of RocketFS server from RocketFS/src/
- Serves local Windows directories
- `python local_server.py --root "C:/Users/test/files"`

---

## Configuration

No flags for file paths — just the server address:
```powershell
python launcher.py --server 192.168.0.110:8765
```

All other settings are optional and loaded from `config.json`:
```json
{
  "server_url": "ws://192.168.0.110:8765",
  "timeout": 30
}
```

---

## Prerequisites

1. Python 3.8+ (venv created from source dir)
2. `RocketFS server` running (see `RocketFS/src/`)
3. Frida installed on the target machine (or run `launcher.py` in the server process)
4. `textual` installed (`pip install -r requirements.txt`)

---

## Development workflow

```
1. cd RocketFS
2. cd src/launcher.py       # From the project root
3. python launcher.py --server 192.168.0.110:8765
4. Browse → Select → Launch
```

**Navigation:**
- `Arrow UP` / `Arrow DOWN` — Move through listed files
- `Enter` — Open selection
  - If a **directory** → enter it (fetches new entry list)
  - If an **executable** → starts with Frida sandbox hooked
  - If **other file** → opens in Windows File Explorer
- `Backspace` / `Esc` — Go up one level (parent directory)
- `Tab` — Toggle debug mode (shows path and status at bottom)

---

### Notes

- **Server URL** is normalized to `ws://` if missing. `"localhost:8765" → "ws://localhost:8765"` (automatic protocol detection).
- **No `--file` or `--pid` flags** — all browsing done via keyboard navigation.
- **Executable detection** — matches C++ client logic: `.exe`, `.dll`, `.bat`, `.cmd`, `.msi`, `.py`, `.ps1`, `.rb`, `.pl`, `.sh`, `.bash`, `.com`, `.scr`, `.cpl`.

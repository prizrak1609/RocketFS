# RocketFS

A FUSE-based filesystem for Windows that mounts remote drives via WebSocket with on-demand file loading.

## Features

- **Remote File Access**: Access files from a remote server as if they were on your local drive
- **On-Demand Loading**: Files are loaded in 64KB chunks only when needed
- **WebSocket Communication**: Real-time bidirectional communication using WebSocket
- **Application Support**: Launch applications with dynamic libraries directly from the mounted drive
- **JSON API**: Simple JSON-based protocol for all operations
- **Read-Only**: Safe read-only access to remote files

## Requirements

### System Requirements

- Windows operating system
- [WinFsp](https://winfsp.dev/rel/) - FUSE for Windows (download and install)
- Python 3.7 or higher

### Python Dependencies

Install the required Python packages:

```powershell
pip install -r requirements.txt
```

Required packages:
- `winfspy` - Python bindings for WinFsp
- `websockets` - WebSocket client/server library
- `aiofiles` - Async file operations (for server)

## Installation

1. **Install WinFsp**
   - Download from https://winfsp.dev/rel/
   - Run the installer
   - Reboot if prompted

2. **Clone or download RocketFS**
   ```powershell
   git clone <repository-url>
   cd RocketFSPy
   ```

3. **Install Python dependencies**
   ```powershell
   pip install -r requirements.txt
   ```

4. **Create configuration file**
   ```powershell
   cp config.json.example config.json
   ```
   
   Edit `config.json` to set your server URL and mount point:
   ```json
   {
     "server_url": "ws://localhost:8765",
     "mount_point": "Z:",
     "timeout": 30
   }
   ```

## Usage

### Starting the Server

The example server serves files from a local directory:

```powershell
python server.py --root C:\path\to\files --host localhost --port 8765
```

Options:
- `--root`: Root directory to serve (required)
- `--host`: Host to bind to (default: localhost)
- `--port`: Port to listen on (default: 8765)

### Mounting the Filesystem

Start the RocketFS client to mount the remote drive:

```powershell
python rocketfs.py --config config.json
```

Options:
- `--config`: Path to configuration file (default: config.json)
- `--debug`: Enable debug logging

The filesystem will be mounted at the drive letter specified in your config (e.g., `Z:`).

Press `Ctrl+C` to unmount and exit.

### Using the Mounted Drive

Once mounted, you can:

1. **Browse files** - Open File Explorer and navigate to the mounted drive
2. **Open files** - Double-click files to open them in their default applications
3. **Run executables** - Launch applications directly from the mounted drive
4. **Copy files** - Copy files from the mounted drive to your local system

## Configuration

The `config.json` file supports the following settings:

```json
{
  "server_url": "ws://localhost:8765",
  "mount_point": "Z:",
  "timeout": 30
}
```

- **server_url** (required): WebSocket URL of the RocketFS server
- **mount_point** (required): Drive letter to mount the filesystem (e.g., "Z:")
- **timeout** (optional): Request timeout in seconds (default: 30)

**Note**: Chunk size is hardcoded to 64KB and cannot be configured.

## API Documentation

See [API.md](API.md) for complete WebSocket API documentation.

## Architecture

RocketFS consists of three main components:

1. **RocketFS Client** (`rocketfs.py`) - FUSE filesystem implementation
   - Implements FUSE operations (getattr, readdir, open, read)
   - Manages file handles
   - Reads files in 64KB chunks on-demand

2. **WebSocket Client** (`websocket_client.py`) - Server communication
   - Manages WebSocket connection
   - Handles request/response correlation
   - JSON serialization/deserialization

3. **Example Server** (`server.py`) - Reference implementation
   - Serves files from local directory
   - Implements all required API operations
   - Handles multiple concurrent clients

## How It Works

1. Client connects to WebSocket server
2. User opens File Explorer and navigates to mounted drive
3. RocketFS sends `readdir` request to server to list directory contents
4. User clicks on a file to open it
5. Application requests file data from OS
6. RocketFS sends `read` requests for 64KB chunks as needed
7. Server reads chunks from local filesystem and sends them back
8. Application receives data and displays/processes the file

## Troubleshooting

### Drive doesn't mount
- Ensure WinFsp is installed correctly
- Check that the drive letter isn't already in use
- Verify server is running and accessible

### Cannot connect to server
- Verify server URL in config.json
- Ensure server is running
- Check firewall settings

### Files don't open
- Check server logs for errors
- Verify file permissions on server
- Ensure file path is correct

### Slow performance
- Check network connection
- Verify server has adequate resources
- Large files are read in chunks, so initial load may take time

## Limitations

- **Read-only**: Write operations are not supported
- **No caching**: Files are always read from server (no local caching)
- **Network dependent**: Requires active connection to server

## License

[Add your license here]

## Contributing

[Add contribution guidelines here]

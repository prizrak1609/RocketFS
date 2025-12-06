"""
Example WebSocket server for RocketFS.
Serves files from a local directory over WebSocket.
"""
import asyncio
import websockets
import json
import os
import base64
import argparse
from datetime import datetime
import time
from pathlib import Path
from diskcache import Cache


class RocketFSServer:
    """WebSocket server for RocketFS."""
    
    def __init__(self, root_dir: str):
        """
        Initialize server.
        
        Args:
            root_dir: Root directory to serve files from
        """
        self.root_dir = os.path.abspath(root_dir)
        self.cache = Cache("cache", size_limit=2**60)
        self.cache.set(self.root_dir.lower(), self.root_dir)
        self.hours24_in_seconds = 86400
        if not os.path.exists(self.root_dir):
            raise ValueError(f"Root directory does not exist: {self.root_dir}")
        
        print(f"Server root directory: {self.root_dir}")
    
    def _get_full_path(self, path: str) -> str:
        """
        Convert virtual path to full filesystem path.
        
        Args:
            path: Virtual path (e.g., /folder/file.txt)
            
        Returns:
            Full filesystem path
        """
        # Normalize path
        path = path.replace('\\', '/')
        if path.startswith('/'):
            path = path[1:]
        
        full_path = os.path.join(self.root_dir, path)
        full_path = os.path.normpath(full_path)
        full_path = full_path.lower()
        if full_path in self.cache:
            self.cache.touch(full_path, expire=self.hours24_in_seconds, retry=True)
            full_path = self.cache.get(full_path)
        else:
            # windows could send path for files which was not send by server to client
            if self._path_exists_case_sensitive(Path(full_path)):
                self.cache.set(full_path.lower(), full_path)
                self.cache.touch(full_path.lower(), expire=self.hours24_in_seconds, retry=True)
                full_path = self.cache.get(full_path.lower())
            else:
                raise ValueError(f"Invalid path {full_path}: not found")
        
        # Security check: ensure path is within root_dir
        if not full_path.startswith(self.root_dir):
            raise ValueError(f"Invalid path {full_path}: outside root directory")
        
        return full_path
    
    def _get_file_attrs(self, full_path: str) -> dict:
        """
        Get file/directory attributes.
        
        Args:
            full_path: Full filesystem path
            
        Returns:
            Dictionary with file attributes
        """
        stat_info = os.stat(full_path)
        
        # Determine type
        file_type = 'directory' if os.path.isdir(full_path) else 'file'
        
        # Convert timestamps to Windows FILETIME format (100-nanosecond intervals since 1601-01-01)
        # For simplicity, we'll use Unix timestamps converted to nanoseconds
        def unix_to_filetime(unix_time):
            # Windows FILETIME epoch is 1601-01-01, Unix epoch is 1970-01-01
            # Difference is 11644473600 seconds
            return int((unix_time + 11644473600) * 10000000)
        
        return {
            'type': file_type,
            'size': stat_info.st_size if file_type == 'file' else 512,
            'creation_time': unix_to_filetime(stat_info.st_ctime),
            'last_access_time': unix_to_filetime(stat_info.st_atime),
            'last_write_time': unix_to_filetime(stat_info.st_mtime),
            'change_time': unix_to_filetime(stat_info.st_mtime),
        }

    def _path_exists_case_sensitive(self, p: Path) -> bool:
        """Check if path exists, enforce case sensitivity.

        Arguments:
          p: Path to check
        Returns:
          Boolean indicating if the path exists or not
        """
        # If it doesn't exist initially, return False
        if not p.exists():
            return False

        # Else loop over the path, checking each consecutive folder for
        # case sensitivity
        while True:
            # At root, p == p.parent --> break loop and return True
            if p == p.parent:
                return True
            # If string representation of path is not in parent directory, return False
            if str(p) not in map(str, p.parent.iterdir()):
                return False
            p = p.parent

    
    async def handle_getattr(self, params: dict) -> dict:
        """
        Handle getattr operation.
        
        Args:
            params: Request parameters with 'path'
            
        Returns:
            File attributes
        """
        path = params.get('path', '/')
        full_path = self._get_full_path(path)
        
        if not os.path.exists(full_path):
            raise FileNotFoundError(f"Path not found: {full_path}")
        
        attrs = self._get_file_attrs(full_path)
        print(f"Attributes for {full_path}: {attrs}")
        return attrs

    def _get_os_path_caseInsensitive(self, full_path: str) -> str:
        for path in self.cache:
            if path.lower() == full_path.lower():
                return path
        return full_path

    def _get_folder_entries(self, full_path: str) -> dict:
        entries = []
        for name in os.listdir(full_path):
            entry_path = os.path.join(full_path, name)
            self.cache.set(os.path.join(full_path.lower(), name.lower()), entry_path)
            try:
                attrs = self._get_file_attrs(entry_path)
                attrs['name'] = name.lower()
                entries.append(attrs)
            except Exception as e:
                print(f"Error getting attributes for {name}: {e}")
        
        print(f"Directory entries for {full_path}: {entries}")
        return {'entries': entries}
    
    async def handle_readdir(self, params: dict) -> dict:
        """
        Handle readdir operation.
        
        Args:
            params: Request parameters with 'path'
            
        Returns:
            Directory entries
        """
        path = params.get('path', '/')
        full_path = self._get_full_path(path)

        if not os.path.exists(full_path):
            raise FileNotFoundError(f"Path not found: {full_path}")
        
        if not os.path.isdir(full_path):
            raise NotADirectoryError(f"Not a directory: {full_path}")
        
        return self._get_folder_entries(full_path)
    
    def _get_file_data(self, full_path: str, seek: int, size: int) -> str:
        with open(full_path, 'rb') as f:
            f.seek(seek)
            data = f.read(size)
        
        data_b64 = base64.b64encode(data).decode('ascii')
        print(f"Read {len(data)} bytes from {full_path} at offset {seek}")
        return data_b64

    async def handle_read(self, params: dict) -> dict:
        """
        Handle read operation.
        
        Args:
            params: Request parameters with 'path', 'offset', 'size'
            
        Returns:
            File data (base64 encoded)
        """
        path = params.get('path')
        offset = params.get('offset', 0)
        size = params.get('size', 65536)
        
        full_path = self._get_full_path(path)
        
        if not os.path.exists(full_path):
            raise FileNotFoundError(f"Path not found: {full_path}")
        
        if os.path.isdir(full_path):
            raise IsADirectoryError(f"Is a directory: {full_path}")
        
        return {'data': self._get_file_data(full_path, offset, size)}
    
    async def handle_request(self, request: dict) -> dict:
        """
        Handle incoming request.
        
        Args:
            request: Request dictionary
            
        Returns:
            Response dictionary
        """
        request_id = request.get('request_id')
        operation = request.get('operation')
        params = request.get('params', {})
        
        try:
            # Route to appropriate handler
            if operation == 'getattr':
                result = await self.handle_getattr(params)
            elif operation == 'readdir':
                result = await self.handle_readdir(params)
            elif operation == 'read':
                result = await self.handle_read(params)
            else:
                raise ValueError(f"Unknown operation: {operation}")
            
            return {
                'request_id': request_id,
                'status': 'success',
                'result': result
            }
            
        except Exception as e:
            return {
                'request_id': request_id,
                'status': 'error',
                'error': str(e)
            }
    
    async def handle_client(self, websocket):
        """
        Handle WebSocket client connection.
        
        Args:
            websocket: WebSocket connection
            path: Connection path
        """
        client_addr = websocket.remote_address
        print(f"Client connected: {client_addr}")
        
        try:
            async for message in websocket:
                try:
                    # Parse request
                    request = json.loads(message)
                    print(f"Request: {request}")
                    
                    # Handle request
                    response = await self.handle_request(request)
                    
                    # Send response
                    response_json = json.dumps(response)
                    await websocket.send(response_json)
                    
                except json.JSONDecodeError as e:
                    print(f"Invalid JSON: {e}")
                except Exception as e:
                    print(f"Error handling request: {e}")
                    
        except websockets.exceptions.ConnectionClosed:
            print(f"Client disconnected: {client_addr}")
        except Exception as e:
            print(f"Error: {e}")


async def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(description='RocketFS WebSocket Server')
    parser.add_argument('--root', required=True, help='Root directory to serve')
    parser.add_argument('--host', default='localhost', help='Host to bind to')
    parser.add_argument('--port', type=int, default=8765, help='Port to listen on')
    args = parser.parse_args()
    
    # Create server
    server = RocketFSServer(args.root)
    
    # Start WebSocket server
    print(f"Starting server on {args.host}:{args.port}")
    # Set max_size to 10MB to handle large chunks
    async with websockets.serve(server.handle_client, args.host, args.port, max_size=100 * 1024 * 1024):
        print("Server started. Press Ctrl+C to stop.")
        await asyncio.Future()  # Run forever


if __name__ == '__main__':
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nServer stopped")

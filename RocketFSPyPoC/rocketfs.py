"""
RocketFS - FUSE filesystem implementation using winfspy.
"""
import argparse
import asyncio
import stat
import errno
from typing import Dict, Any
from winfspy import (
    FileSystem, BaseFileSystemOperations,
    FILE_ATTRIBUTE, CREATE_FILE_CREATE_OPTIONS,
    NTStatusMediaWriteProtected, NTStatusObjectNameNotFound,
    NTStatusDirectoryNotEmpty, NTStatusNotADirectory,
    NTStatusObjectNameCollision, NTStatusEndOfFile,
    enable_debug_log
)

from diskcache import Cache
from config import Config
from websocket_client import WebSocketClient


class RocketFS(BaseFileSystemOperations):
    """FUSE filesystem that serves files from remote server via WebSocket."""
    
    def __init__(self, ws_client: WebSocketClient, event_loop, cache_dir: str):
        """
        Initialize RocketFS.
        
        Args:
            ws_client: WebSocket client for server communication
            event_loop: Asyncio event loop to use for operations
        """
        super().__init__()
        self.cache = Cache(cache_dir, size_limit=2**60)
        self.ws_client = ws_client
        self.loop = event_loop
        self.hours24_in_seconds = 86400
        
        # Create and cache security descriptor to prevent garbage collection
        from winfspy.plumbing.security_descriptor import SecurityDescriptor
        self.security_descriptor = SecurityDescriptor.from_string("O:BAG:BAD:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;FA;;;WD)")
    
    def _run_async(self, coro):
        """Run async coroutine synchronously."""
        future = asyncio.run_coroutine_threadsafe(coro, self.loop)
        return future.result()
    
    def get_volume_info(self):
        """Get volume information."""
        return {
            'total_size': 1024 * 1024 * 1024 * 100,  # 100GB
            'free_size': 1024 * 1024 * 1024 * 50,    # 50GB
            'volume_label': 'RocketFS',
        }
    
    file_attrs_dict = {}
    def get_security_by_name(self, file_name):
        """Get security descriptor by file name."""
        if file_name.lower().endswith("desktop.ini") or file_name.lower().endswith("autorun.inf") \
            or file_name.lower().endswith("folder.gif") or file_name.lower().endswith("folder.jpg")\
                or file_name.lower().endswith("thumbs.db"):
            raise NTStatusObjectNameNotFound()

        try:
            # Get file attributes from server
            if file_name in self.file_attrs_dict:
                attrs = self.file_attrs_dict[file_name]
            else:
                attrs = self._run_async(self.ws_client.getattr(file_name))
                self.file_attrs_dict[file_name] = attrs 
            
            file_attributes = FILE_ATTRIBUTE.FILE_ATTRIBUTE_READONLY
            if attrs.get('type') == 'directory':
                file_attributes |= FILE_ATTRIBUTE.FILE_ATTRIBUTE_DIRECTORY
            
            # Return cached security descriptor (prevents garbage collection)
            return file_attributes, self.security_descriptor.handle, self.security_descriptor.size
        except Exception as e:
            print(f"Error in get_security_by_name for {file_name}: {e}")
            # import traceback
            # traceback.print_exc()
            raise NTStatusObjectNameNotFound()

    def get_security(self, file_handle):
        """Get security descriptor by file handle."""
        # file_handle is the file_context dict created in open()
        # Return the same cached security descriptor for all files
        return self.security_descriptor.handle, self.security_descriptor.size
    
    def create(self, file_name, create_options, granted_access, file_attributes, security_descriptor, allocation_size):
        """Create file operation - not supported (read-only filesystem)."""
        raise NTStatusMediaWriteProtected()
    
    open_dict = {}
    def open(self, file_name, create_options, granted_access):
        """
        Open file.
        
        Args:
            file_name: Path to file
            create_options: Create options
            granted_access: Access rights
            
        Returns:
            File context dict
        """
        if file_name.lower().endswith("desktop.ini") or file_name.lower().endswith("autorun.inf"):
            raise NTStatusObjectNameNotFound()

        if file_name in self.open_dict:
            return self.open_dict[file_name]

        try:
            # Get file attributes from server
            attrs = self._run_async(self.ws_client.getattr(file_name))
            
            # Create file context dict (this gets passed to other methods)
            file_context = {
                'path': file_name,
                'attrs': attrs
            }
            
            self.open_dict[file_name] = file_context
            return file_context
            
        except Exception as e:
            print(f"Error opening {file_name}: {e}")
            raise NTStatusObjectNameNotFound()
    
    def close(self, file_context):
        """
        Close file.
        
        Args:
            file_context: File context dict
        """
        # Nothing to clean up
        pass
    
    def read(self, file_context, offset, length):
        """
        Read data from file.
        
        Args:
            file_context: File context dict
            offset: Read offset
            length: Number of bytes to read
            
        Returns:
            Bytes read
        """
        path = file_context['path']

        file_size = file_context['attrs'].get('size', 0)
        
        # Check if offset is beyond file size
        if offset >= file_size:
            raise NTStatusEndOfFile()
        
        # Adjust length if reading beyond file size
        if offset + length > file_size:
            length = file_size - offset
        
        cache_key = f"{path}_{offset}_{length}"
        if cache_key in self.cache:
            self.cache.touch(cache_key, expire=self.hours24_in_seconds, retry=True)
            print(f"Cache hit {cache_key}")
            return self.cache.get(cache_key)

        try:
            # Read chunk from server (hardcoded 64KB chunk size)
            data = self._run_async(self.ws_client.read(path, offset, length))
            self.cache.set(cache_key, data)
            return data
        except Exception as e:
            print(f"Error reading {path} at offset {offset}: {e}")
            return b''
    
    def write(self, file_context, buffer, offset, write_to_end_of_file, constrained_io):
        """Write operation - not supported (read-only filesystem)."""
        raise NTStatusMediaWriteProtected()
    
    def get_file_info(self, file_context):
        """
        Get file information.
        
        Args:
            file_context: File context dict
            
        Returns:
            File info dict
        """
        attrs = file_context['attrs']
        
        info = {
            'file_attributes': FILE_ATTRIBUTE.FILE_ATTRIBUTE_READONLY,
            'file_size': attrs.get('size', 0),
            'allocation_size': attrs.get('size', 0),
        }
        
        if attrs.get('type') == 'directory':
            info['file_attributes'] |= FILE_ATTRIBUTE.FILE_ATTRIBUTE_DIRECTORY
        
        # Add timestamps if available
        if 'creation_time' in attrs:
            info['creation_time'] = attrs['creation_time']
        if 'last_access_time' in attrs:
            info['last_access_time'] = attrs['last_access_time']
        if 'last_write_time' in attrs:
            info['last_write_time'] = attrs['last_write_time']
        if 'change_time' in attrs:
            info['change_time'] = attrs['change_time']
        
        return info
    
    read_directory_dict = {}
    def read_directory(self, file_context, marker):
        """
        Read directory contents.
        
        Args:
            file_context: File context dict
            marker: Continuation marker
            
        Returns:
            List of directory entries
        """
        path = file_context['path']
        attrs = file_context['attrs']
        
        # Check if it's a directory
        if attrs.get('type') != 'directory':
            raise NTStatusNotADirectory()
        
        if path in self.read_directory_dict:
            return self.read_directory_dict[path]

        try:
            # Get directory listing from server
            entries = self._run_async(self.ws_client.readdir(path))
            
            # Convert to winfspy format
            dir_list = []
            for entry in entries:
                file_attrs = FILE_ATTRIBUTE.FILE_ATTRIBUTE_READONLY
                if entry.get('type') == 'directory':
                    file_attrs |= FILE_ATTRIBUTE.FILE_ATTRIBUTE_DIRECTORY
                
                dir_entry = {
                    'file_name': entry['name'],
                    'file_attributes': file_attrs,
                    'file_size': entry.get('size', 0),
                    'allocation_size': entry.get('size', 0),
                }
                
                # Add timestamps if available
                if 'creation_time' in entry:
                    dir_entry['creation_time'] = entry['creation_time']
                if 'last_access_time' in entry:
                    dir_entry['last_access_time'] = entry['last_access_time']
                if 'last_write_time' in entry:
                    dir_entry['last_write_time'] = entry['last_write_time']
                if 'change_time' in entry:
                    dir_entry['change_time'] = entry['change_time']
                
                dir_list.append(dir_entry)
            
            self.read_directory_dict[path] = dir_list
            return dir_list
            
        except Exception as e:
            print(f"Error reading directory {path}: {e}")
            return []


def main():
    """Main entry point."""
    import threading
    
    parser = argparse.ArgumentParser(description='RocketFS - Remote filesystem over WebSocket')
    parser.add_argument('--config', default='config.json', help='Configuration file path')
    parser.add_argument('--debug', action='store_true', help='Enable debug logging')
    args = parser.parse_args()
    
    # Load configuration
    try:
        config = Config(args.config)
        print(f"Loaded configuration: {config}")
    except Exception as e:
        print(f"Error loading configuration: {e}")
        return 1
    
    # Enable debug logging if requested
    if args.debug:
        enable_debug_log()
    
    # Create event loop in separate thread
    loop = asyncio.new_event_loop()
    
    def run_event_loop():
        asyncio.set_event_loop(loop)
        loop.run_forever()
    
    loop_thread = threading.Thread(target=run_event_loop, daemon=True)
    loop_thread.start()
    
    # Create WebSocket client
    ws_client = WebSocketClient(config.server_url, config.timeout)
    
    # Connect to server
    print(f"Connecting to {config.server_url}...")
    try:
        future = asyncio.run_coroutine_threadsafe(ws_client.connect(), loop)
        future.result()
        print("Connected to server")
    except Exception as e:
        print(f"Failed to connect to server: {e}")
        loop.call_soon_threadsafe(loop.stop)
        return 1
    
    # Create and mount filesystem
    fs = None
    try:
        rocket_fs = RocketFS(ws_client, loop, config.cache_dir)
        fs = FileSystem(
            config.mount_point,
            rocket_fs,
            debug=True,
            prefix="\\RocketFS\\share",
            case_sensitive_search=True,
            sector_size=512,
            sectors_per_allocation_unit=1,
            volume_creation_time=0,
            volume_serial_number=0,
            file_info_timeout=1000,
            case_preserved_names=True,
            unicode_on_disk=True,
            persistent_acls=False,
            post_cleanup_when_modified_only=True,
            um_file_context_is_user_context2=True,
            file_system_name="RocketFS",
        )
        
        print(f"Mounted at {config.mount_point}")
        print("Press Ctrl+C to unmount and exit")
        
        # Run until interrupted
        fs.start()
        
        # Keep the program running
        import time
        while True:
            time.sleep(1)
        
    except KeyboardInterrupt:
        print("\nUnmounting...")
    except Exception as e:
        print(f"Error: {e}")
        # import traceback
        # traceback.print_exc()
        return 1
    finally:
        # Cleanup
        try:
            if fs is not None:
                fs.stop()
        except Exception as e:
            print(f"Error stopping filesystem: {e}")
        
        # Disconnect websocket
        try:
            future = asyncio.run_coroutine_threadsafe(ws_client.disconnect(), loop)
            future.result(timeout=5)
        except Exception as e:
            print(f"Error disconnecting: {e}")
        
        # Stop event loop
        loop.call_soon_threadsafe(loop.stop)
    
    print("Unmounted successfully")
    return 0


if __name__ == '__main__':
    exit(main())

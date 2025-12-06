"""
WebSocket client for RocketFS server communication.
"""
import asyncio
import json
import uuid
import websockets
from typing import Dict, Any, Optional


class WebSocketClient:
    """WebSocket client for communicating with RocketFS server."""
    
    def __init__(self, server_url: str, timeout: int = 30):
        """
        Initialize WebSocket client.
        
        Args:
            server_url: WebSocket server URL
            timeout: Request timeout in seconds
        """
        self.server_url = server_url
        self.timeout = timeout
        self.websocket = None
        self.pending_requests: Dict[str, asyncio.Future] = {}
        self.receive_task = None
    
    async def connect(self):
        """Connect to WebSocket server."""
        # Set max_size to 10MB to handle large chunks
        self.websocket = await websockets.connect(self.server_url, max_size=100 * 1024 * 1024)
        # Start receiving messages
        self.receive_task = asyncio.create_task(self._receive_messages())
    
    async def disconnect(self):
        """Disconnect from WebSocket server."""
        if self.receive_task:
            self.receive_task.cancel()
            try:
                await self.receive_task
            except asyncio.CancelledError:
                pass
        
        if self.websocket:
            await self.websocket.close()
            self.websocket = None
    
    async def _receive_messages(self):
        """Continuously receive and process messages from server."""
        try:
            async for message in self.websocket:
                data = json.loads(message)
                request_id = data.get('request_id')
                
                if request_id and request_id in self.pending_requests:
                    future = self.pending_requests.pop(request_id)
                    if not future.cancelled():
                        future.set_result(data)
        except asyncio.CancelledError:
            pass
        except Exception as e:
            print(f"Error receiving messages: {e}")
    
    async def send_request(self, operation: str, params: Dict[str, Any]) -> Dict[str, Any]:
        """
        Send request to server and wait for response.
        
        Args:
            operation: Operation name
            params: Operation parameters
            
        Returns:
            Response data
            
        Raises:
            Exception: If request fails or times out
        """
        if not self.websocket:
            raise Exception("Not connected to server")
        
        # Generate unique request ID
        request_id = str(uuid.uuid4())
        
        # Create request
        request = {
            'request_id': request_id,
            'operation': operation,
            'params': params
        }
        
        # Create future for response
        future = asyncio.Future()
        self.pending_requests[request_id] = future
        
        try:
            # Send request
            await self.websocket.send(json.dumps(request))
            
            # Wait for response with timeout
            response = await asyncio.wait_for(future, timeout=self.timeout)
            
            # Check if response indicates error
            if response.get('status') == 'error':
                raise Exception(f"{response}")
            
            return response.get('result', {})
            
        except asyncio.TimeoutError:
            self.pending_requests.pop(request_id, None)
            raise Exception(f"Request timeout for operation: {operation}")
        except Exception as e:
            self.pending_requests.pop(request_id, None)
            raise
    
    async def getattr(self, path: str) -> Dict[str, Any]:
        """Get file/directory attributes."""
        return await self.send_request('getattr', {'path': path})
    
    async def readdir(self, path: str) -> list:
        """List directory contents."""
        result = await self.send_request('readdir', {'path': path})
        return result.get('entries', [])
    
    async def read(self, path: str, offset: int, size: int) -> bytes:
        """
        Read file chunk.
        
        Args:
            path: File path
            offset: Read offset
            size: Number of bytes to read
            
        Returns:
            File data as bytes
        """
        result = await self.send_request('read', {
            'path': path,
            'offset': offset,
            'size': size
        })
        
        # Data is returned as base64 encoded string
        import base64
        data_b64 = result.get('data', '')
        return base64.b64decode(data_b64)

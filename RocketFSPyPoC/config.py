"""
Configuration management for RocketFS.
"""
import json
import os


class Config:
    """Configuration handler for RocketFS."""
    
    # Hardcoded chunk size as per requirements
    CHUNK_SIZE = 65536  # 64KB
    
    def __init__(self, config_path='config.json'):
        """
        Initialize configuration from JSON file.
        
        Args:
            config_path: Path to configuration file
        """
        self.config_path = config_path
        self.server_url = None
        self.mount_point = None
        self.cache_dir = None
        self.timeout = 30
        
        self._load_config()
    
    def _load_config(self):
        """Load configuration from file."""
        if not os.path.exists(self.config_path):
            raise FileNotFoundError(f"Configuration file not found: {self.config_path}")
        
        with open(self.config_path, 'r') as f:
            config_data = json.load(f)
        
        # Required fields
        self.server_url = config_data.get('server_url')
        if not self.server_url:
            raise ValueError("server_url is required in configuration")
        
        self.mount_point = config_data.get('mount_point')
        if not self.mount_point:
            raise ValueError("mount_point is required in configuration")
        
        # Optional fields
        self.timeout = config_data.get('timeout', 30)
        self.cache_dir = config_data.get('cache_dir', 'RocketFSCache')
    
    def __repr__(self):
        return f"Config(server={self.server_url}, mount={self.mount_point}, cache={self.cache_dir})"

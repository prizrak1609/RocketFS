# RocketFS WebSocket API

This document defines the WebSocket API protocol for RocketFS.

## Protocol Overview

All communication uses JSON-formatted messages over WebSocket. Each request has a unique ID and specifies an operation with parameters. The server responds with the same request ID and either success result or error message.

## Request Format

```json
{
  "request_id": "unique-id-string",
  "operation": "operation_name",
  "params": {
    "param1": "value1",
    "param2": "value2"
  }
}
```

### Fields

- **request_id** (string, required): Unique identifier for correlating requests with responses
- **operation** (string, required): Name of the operation to perform
- **params** (object, required): Operation-specific parameters

## Response Format

```json
{
  "request_id": "unique-id-string",
  "status": "success",
  "result": {
    "data": "result-data"
  }
}
```

Or in case of error:

```json
{
  "request_id": "unique-id-string",
  "status": "error",
  "error": "Error message"
}
```

### Fields

- **request_id** (string, required): Same ID as the request
- **status** (string, required): Either "success" or "error"
- **result** (object, optional): Operation results (only present if status is "success")
- **error** (string, optional): Error message (only present if status is "error")

## Operations

### getattr

Get attributes for a file or directory.

**Request:**
```json
{
  "request_id": "123",
  "operation": "getattr",
  "params": {
    "path": "/path/to/file.txt"
  }
}
```

**Response:**
```json
{
  "request_id": "123",
  "status": "success",
  "result": {
    "type": "file",
    "size": 1024,
    "creation_time": 132345678900000000,
    "last_access_time": 132345678900000000,
    "last_write_time": 132345678900000000,
    "change_time": 132345678900000000
  }
}
```

**Parameters:**
- `path` (string): Path to file or directory

**Result Fields:**
- `type` (string): Either "file" or "directory"
- `size` (integer): File size in bytes (0 for directories)
- `creation_time` (integer): Creation time in Windows FILETIME format
- `last_access_time` (integer): Last access time in Windows FILETIME format
- `last_write_time` (integer): Last write time in Windows FILETIME format
- `change_time` (integer): Change time in Windows FILETIME format

### readdir

List contents of a directory.

**Request:**
```json
{
  "request_id": "124",
  "operation": "readdir",
  "params": {
    "path": "/path/to/directory"
  }
}
```

**Response:**
```json
{
  "request_id": "124",
  "status": "success",
  "result": {
    "entries": [
      {
        "name": "file1.txt",
        "type": "file",
        "size": 1024,
        "creation_time": 132345678900000000,
        "last_access_time": 132345678900000000,
        "last_write_time": 132345678900000000,
        "change_time": 132345678900000000
      },
      {
        "name": "subfolder",
        "type": "directory",
        "size": 0,
        "creation_time": 132345678900000000,
        "last_access_time": 132345678900000000,
        "last_write_time": 132345678900000000,
        "change_time": 132345678900000000
      }
    ]
  }
}
```

**Parameters:**
- `path` (string): Path to directory

**Result Fields:**
- `entries` (array): List of directory entries, each containing:
  - `name` (string): File or directory name
  - `type` (string): Either "file" or "directory"
  - `size` (integer): File size in bytes (0 for directories)
  - `creation_time` (integer): Creation time in Windows FILETIME format
  - `last_access_time` (integer): Last access time in Windows FILETIME format
  - `last_write_time` (integer): Last write time in Windows FILETIME format
  - `change_time` (integer): Change time in Windows FILETIME format

### read

Read a chunk of data from a file.

**Request:**
```json
{
  "request_id": "125",
  "operation": "read",
  "params": {
    "path": "/path/to/file.txt",
    "offset": 0,
    "size": 65536
  }
}
```

**Response:**
```json
{
  "request_id": "125",
  "status": "success",
  "result": {
    "data": "SGVsbG8gV29ybGQh..."
  }
}
```

**Parameters:**
- `path` (string): Path to file
- `offset` (integer): Byte offset to start reading from
- `size` (integer): Number of bytes to read (typically 65536 for 64KB chunks)

**Result Fields:**
- `data` (string): File data encoded as base64 string

## Notes

- All paths use forward slashes (/) as separators
- Paths are relative to the server's root directory
- Timestamps are in Windows FILETIME format (100-nanosecond intervals since January 1, 1601)
- File data is base64 encoded in responses
- The default chunk size is 64KB (65536 bytes)

# Test HTTP Server

Mock HTTP server that simulates Docker container metrics for testing the ESP32 e-paper monitor.

## Setup

1. **Install Python dependencies:**

   ```bash
   pip install -r requirements.txt
   ```

2. **Run the server:**
   ```bash
   python server.py
   ```

The server will start on `http://0.0.0.0:5000` (accessible from any device on your network).

## Endpoints

### `GET /`

Returns API information and available endpoints.

### `GET /health`

Health check endpoint.

### `GET /status`

Returns mock Docker metrics:

```json
{
  "status": "ok",
  "timestamp": "2025-12-16T10:30:45.123456",
  "server": {
    "hostname": "docker-host-1",
    "uptime": "15 days, 3 hours"
  },
  "containers": {
    "total": 5,
    "running": 4,
    "stopped": 1
  },
  "resources": {
    "cpu_percent": 45.3,
    "memory_percent": 67.2,
    "disk_percent": 52.1
  },
  "container_list": [
    {"name": "web-app", "status": "running"},
    {"name": "database", "status": "running"},
    ...
  ]
}
```

## Configuration

Update the `SERVER_URL` in your ESP32 credentials:

```cpp
const char *SERVER_URL = "http://YOUR_PC_IP:5000/status";
```

Replace `YOUR_PC_IP` with your computer's local IP address (e.g., `192.168.1.100`).

## Finding Your IP Address

**Windows:**

```bash
ipconfig
```

Look for "IPv4 Address" under your active network adapter.

**Linux/Mac:**

```bash
ifconfig
# or
ip addr show
```

## Testing

You can test the server with curl:

```bash
curl http://localhost:5000/status
```

Or open in your browser: `http://localhost:5000/status`

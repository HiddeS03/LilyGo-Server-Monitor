# Docker Game Server Monitor

Production-ready monitoring server for Docker game servers with real-time stats and logs.

## Features

- **Real-time container monitoring** (online/offline status, uptime)
- **In-game log parsing** (Minecraft chat, player joins/leaves, Satisfactory events)
- **Player count tracking** for Minecraft servers
- **System monitoring** (CPU temperature, RAM usage)
- **Docker API integration** - no mock data!

## Monitored Servers

- `minecraft_bingo_server` - Minecraft Bingo/Fetchr
- `minecraft_server` - Main Minecraft server
- `satisfactory-server` - Satisfactory dedicated server

## Setup

### 1. Install Dependencies

```bash
cd docker_monitor_server
pip install -r requirements.txt
```

### 2. Docker Socket Access

The server needs access to Docker socket. Run it on the same machine as your Docker host.

**Option A: Run as root (easiest for testing)**

```bash
sudo python server.py
```

**Option B: Add user to docker group (recommended)**

```bash
sudo usermod -aG docker $USER
# Log out and back in, then:
python server.py
```

### 3. Run the Server

```bash
python server.py
```

Server runs on `http://0.0.0.0:5000`

## API Response Format

### GET /status

```json
{
  "timestamp": "2025-12-17T10:30:45.123456",
  "system": {
    "cpu_temp": 45.3,
    "memory_percent": 67.2
  },
  "servers": {
    "minecraft_bingo": {
      "name": "minecraft_bingo_server",
      "status": "running",
      "online": true,
      "uptime": "3h 25m",
      "players": 2,
      "logs": [
        "Player1 joined the game",
        "<Player1> Hello!",
        "Player2 joined the game"
      ]
    },
    "minecraft": {
      "name": "minecraft_server",
      "status": "running",
      "online": true,
      "uptime": "15h 42m",
      "players": 5,
      "logs": [
        "Server tick took 2.5s",
        "<Admin> Welcome everyone!",
        "Player3 left the game"
      ]
    },
    "satisfactory": {
      "name": "satisfactory-server",
      "status": "running",
      "online": true,
      "uptime": "2d 5h 12m",
      "logs": [
        "LogNet: Join succeeded: Player1",
        "LogWorld: Save game complete"
      ]
    }
  }
}
```

## Log Filtering

### Minecraft

- Filters out noise (UUID messages, "Can't keep up", etc.)
- Shows: player joins/leaves, chat, important events
- Extracts player names and counts

### Satisfactory

- Shows last 3 log lines
- Truncates long lines to 100 characters

## System Requirements

- **Linux server** (for CPU temp and memory monitoring)
- **Docker Engine** running
- **Python 3.8+**
- Network access from ESP32 device

## Troubleshooting

### "Permission denied" accessing Docker

Add your user to docker group:

```bash
sudo usermod -aG docker $USER
```

### CPU temperature returns None

Install `lm-sensors`:

```bash
sudo apt install lm-sensors
sudo sensors-detect
```

### Container not found

Verify container names match your docker-compose.yml:

```bash
docker ps -a
```

Update container names in server.py if needed.

## Security Notes

- This server exposes Docker container logs
- Use firewall rules to restrict access
- Consider adding authentication for production
- Run behind reverse proxy with SSL in production

## ESP32 Configuration

Update your ESP32 credentials.h:

```cpp
const char *SERVER_URL = "http://YOUR_SERVER_IP:5000/status";
```

Replace `YOUR_SERVER_IP` with your Docker host's IP address.

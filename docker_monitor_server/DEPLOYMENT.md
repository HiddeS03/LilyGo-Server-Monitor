# Docker Monitor Server - Deployment Guide

## Installation on Ubuntu

### 1. Install Dependencies

```bash
# Update package list
sudo apt update

# Install Python 3 and pip
sudo apt install python3 python3-pip python3-venv -y

# Install Docker if not already installed
sudo apt install docker.io -y
sudo systemctl enable docker
sudo systemctl start docker

# Add your user to docker group (to run without sudo)
sudo usermod -aG docker $USER
# Log out and back in for group changes to take effect
```

### 2. Set Up Application

```bash
# Navigate to the server directory
cd /path/to/LilyGo-Server-Monitor/docker_monitor_server

# Create virtual environment (recommended)
python3 -m venv venv
source venv/bin/activate

# Install Python dependencies
pip install -r requirements.txt
```

### 3. Test the Server Manually

```bash
# Run the server to test it works
python3 server.py

# In another terminal, test the endpoint
curl http://localhost:5000/status

# Press Ctrl+C to stop
```

### 4. Install as systemd Service

```bash
# Edit the service file with correct paths and username
nano docker-monitor.service

# Update these lines:
#   User=your_username          -> your actual Ubuntu username
#   WorkingDirectory=/path/...  -> full path to docker_monitor_server directory
#   ExecStart=...               -> if using venv: /path/to/venv/bin/python3 server.py

# Copy service file to systemd directory
sudo cp docker-monitor.service /etc/systemd/system/

# Reload systemd to recognize new service
sudo systemctl daemon-reload

# Enable service to start on boot
sudo systemctl enable docker-monitor

# Start the service now
sudo systemctl start docker-monitor

# Check status
sudo systemctl status docker-monitor
```

## Managing the Service

### Service Commands

```bash
# Start the service
sudo systemctl start docker-monitor

# Stop the service
sudo systemctl stop docker-monitor

# Restart the service
sudo systemctl restart docker-monitor

# Check status
sudo systemctl status docker-monitor

# View logs (real-time)
sudo journalctl -u docker-monitor -f

# View logs (last 100 lines)
sudo journalctl -u docker-monitor -n 100

# Disable auto-start on boot
sudo systemctl disable docker-monitor

# Enable auto-start on boot
sudo systemctl enable docker-monitor
```

## Troubleshooting

### Service won't start

1. **Check logs:**

   ```bash
   sudo journalctl -u docker-monitor -n 50
   ```

2. **Check Python dependencies:**

   ```bash
   cd /path/to/docker_monitor_server
   source venv/bin/activate  # if using venv
   pip install -r requirements.txt
   ```

3. **Check Docker socket permissions:**

   ```bash
   # Make sure user is in docker group
   groups $USER

   # If not, add user to docker group
   sudo usermod -aG docker $USER
   ```

4. **Check Docker is running:**
   ```bash
   sudo systemctl status docker
   ```

### Port already in use

If port 5000 is already in use, edit `server.py` and change:

```python
app.run(host='0.0.0.0', port=5000)  # Change 5000 to another port
```

Then restart the service:

```bash
sudo systemctl restart docker-monitor
```

### Python dependencies issues

If you encounter `urllib3` compatibility issues:

```bash
# Option 1: Use compatible versions
pip install urllib3==1.26.15 docker==6.1.3

# Option 2: Upgrade to latest
pip install --upgrade urllib3 docker

# Option 3: Use Docker SDK v7
pip install docker>=7.0.0
```

## Firewall Configuration

If you need to access the server from other devices on your network:

```bash
# Allow port 5000 through firewall
sudo ufw allow 5000/tcp

# Check firewall status
sudo ufw status
```

## Alternative: Using Virtual Environment in Service

If you want to use a Python virtual environment (recommended):

1. **Create and set up venv:**

   ```bash
   cd /path/to/docker_monitor_server
   python3 -m venv venv
   source venv/bin/activate
   pip install -r requirements.txt
   ```

2. **Update service file ExecStart line:**

   ```ini
   ExecStart=/path/to/docker_monitor_server/venv/bin/python3 server.py
   ```

3. **Reload and restart:**
   ```bash
   sudo systemctl daemon-reload
   sudo systemctl restart docker-monitor
   ```

## Checking Service Status at Boot

To verify the service starts automatically after reboot:

```bash
# Reboot the system
sudo reboot

# After reboot, check if service is running
sudo systemctl status docker-monitor

# Check if service is enabled for auto-start
sudo systemctl is-enabled docker-monitor
```

## Accessing from ESP32

Make sure your Ubuntu server's firewall allows incoming connections, and use the server's IP address in your ESP32 credentials.h:

```cpp
const char *SERVER_URL = "http://192.168.2.68:5000/status";  // Your Ubuntu IP
```

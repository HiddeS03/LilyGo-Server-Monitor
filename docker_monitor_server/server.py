"""
Docker Game Server Monitor
Monitors Minecraft and Satisfactory servers with real-time logs and stats
"""

from flask import Flask, jsonify
from flask_cors import CORS
import docker
import re
import os
from datetime import datetime
from collections import deque

app = Flask(__name__)
CORS(app)

# Docker client
docker_client = docker.from_env()

# Store recent log lines for each container (max 10 lines)
log_buffers = {
    'minecraft_bingo_server': deque(maxlen=10),
    'minecraft_server': deque(maxlen=10),
    'satisfactory-server': deque(maxlen=10)
}

# Minecraft log patterns
MINECRAFT_PLAYER_JOIN = re.compile(r'(\w+)\s+joined the game')
MINECRAFT_PLAYER_LEAVE = re.compile(r'(\w+)\s+left the game')
MINECRAFT_CHAT = re.compile(r'<(\w+)>\s+(.+)')
MINECRAFT_SERVER_START = re.compile(r'Done \([\d.]+s\)!')
MINECRAFT_PLAYER_COUNT = re.compile(r'There are (\d+) of a max of (\d+) players online')

def get_cpu_temperature():
    """Get CPU temperature from system (Linux only)"""
    try:
        # Try thermal zone (Raspberry Pi, most Linux systems)
        temp_paths = [
            '/sys/class/thermal/thermal_zone0/temp',
            '/sys/class/thermal/thermal_zone1/temp'
        ]
        
        for path in temp_paths:
            if os.path.exists(path):
                with open(path, 'r') as f:
                    temp = float(f.read().strip()) / 1000.0
                    return round(temp, 1)
        
        # Fallback: try sensors command
        import subprocess
        result = subprocess.run(['sensors'], capture_output=True, text=True, timeout=2)
        if result.returncode == 0:
            # Parse sensors output for CPU temp
            for line in result.stdout.split('\n'):
                if 'CPU' in line or 'Core' in line:
                    match = re.search(r'([+-]?\d+\.\d+)°C', line)
                    if match:
                        return float(match.group(1))
        
        return None
    except Exception as e:
        print(f"Error getting CPU temperature: {e}")
        return None

def get_memory_usage():
    """Get system memory usage percentage"""
    try:
        with open('/proc/meminfo', 'r') as f:
            lines = f.readlines()
            mem_total = None
            mem_available = None
            
            for line in lines:
                if line.startswith('MemTotal:'):
                    mem_total = int(line.split()[1])
                elif line.startswith('MemAvailable:'):
                    mem_available = int(line.split()[1])
            
            if mem_total and mem_available:
                mem_used = mem_total - mem_available
                mem_percent = (mem_used / mem_total) * 100
                return round(mem_percent, 1)
        
        return None
    except Exception as e:
        print(f"Error getting memory usage: {e}")
        return None

def parse_minecraft_log_line(line):
    """Extract meaningful information from Minecraft log line"""
    # Remove timestamp and thread info
    match = re.search(r'\[.*?\]\s*\[.*?\]:\s*(.+)', line)
    if match:
        clean_line = match.group(1)
        
        # Filter out noisy lines
        noise_patterns = [
            'Can\'t keep up',
            'Running',
            'UUID of player',
            'Loading properties',
            'Preparing spawn area'
        ]
        
        for pattern in noise_patterns:
            if pattern in clean_line:
                return None
        
        return clean_line
    
    return None

def get_minecraft_player_count(container):
    """Get current player count from Minecraft server using RCON or logs"""
    try:
        # Check recent logs for player join/leave messages
        logs = container.logs(tail=50).decode('utf-8', errors='ignore')
        
        # Count unique players from recent activity
        players_online = set()
        for line in logs.split('\n'):
            join_match = MINECRAFT_PLAYER_JOIN.search(line)
            if join_match:
                players_online.add(join_match.group(1))
            
            leave_match = MINECRAFT_PLAYER_LEAVE.search(line)
            if leave_match:
                players_online.discard(leave_match.group(1))
        
        return len(players_online)
    except Exception as e:
        print(f"Error getting player count: {e}")
        return 0

def get_container_logs(container_name, lines=5):
    """Get recent logs from a container"""
    try:
        container = docker_client.containers.get(container_name)
        logs = container.logs(tail=lines).decode('utf-8', errors='ignore')
        
        log_lines = []
        for line in logs.split('\n'):
            if line.strip():
                if 'minecraft' in container_name.lower():
                    parsed = parse_minecraft_log_line(line)
                    if parsed:
                        log_lines.append(parsed)
                else:
                    # For Satisfactory, keep raw logs (limited)
                    log_lines.append(line.strip()[-100:])  # Last 100 chars
        
        return log_lines[-lines:]  # Return last N lines
    except Exception as e:
        print(f"Error getting logs for {container_name}: {e}")
        return [f"Error: {str(e)}"]

def get_container_status(container_name):
    """Get status of a specific container"""
    try:
        container = docker_client.containers.get(container_name)
        
        status = {
            'name': container_name,
            'status': container.status,
            'online': container.status == 'running',
            'uptime': None,
            'logs': get_container_logs(container_name, 3)
        }
        
        # Get uptime if running
        if status['online']:
            started_at = container.attrs['State']['StartedAt']
            # Parse ISO timestamp
            start_time = datetime.fromisoformat(started_at.replace('Z', '+00:00'))
            uptime_seconds = (datetime.now(start_time.tzinfo) - start_time).total_seconds()
            
            # Format uptime
            hours = int(uptime_seconds // 3600)
            minutes = int((uptime_seconds % 3600) // 60)
            status['uptime'] = f"{hours}h {minutes}m"
        
        # Get player count for Minecraft servers
        if 'minecraft' in container_name.lower() and status['online']:
            status['players'] = get_minecraft_player_count(container)
        
        return status
    except docker.errors.NotFound:
        return {
            'name': container_name,
            'status': 'not_found',
            'online': False,
            'uptime': None,
            'logs': ['Container not found']
        }
    except Exception as e:
        print(f"Error getting status for {container_name}: {e}")
        return {
            'name': container_name,
            'status': 'error',
            'online': False,
            'uptime': None,
            'logs': [f'Error: {str(e)}']
        }

@app.route('/status', methods=['GET'])
def get_status():
    """
    Returns real-time status of game servers
    """
    # Get container statuses
    minecraft_bingo = get_container_status('minecraft_bingo_server')
    minecraft = get_container_status('minecraft_server')
    satisfactory = get_container_status('satisfactory-server')
    
    # Get system stats
    cpu_temp = get_cpu_temperature()
    mem_usage = get_memory_usage()
    
    response = {
        'timestamp': datetime.now().isoformat(),
        'system': {
            'cpu_temp': cpu_temp,
            'memory_percent': mem_usage
        },
        'servers': {
            'minecraft_bingo': minecraft_bingo,
            'minecraft': minecraft,
            'satisfactory': satisfactory
        }
    }
    
    return jsonify(response)

@app.route('/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({
        'status': 'healthy',
        'service': 'docker-game-server-monitor',
        'timestamp': datetime.now().isoformat()
    })

@app.route('/', methods=['GET'])
def index():
    """API information"""
    return jsonify({
        'name': 'Docker Game Server Monitor',
        'version': '2.0.0',
        'endpoints': {
            '/': 'API information',
            '/health': 'Health check',
            '/status': 'Real-time game server status'
        },
        'monitored_servers': [
            'minecraft_bingo_server',
            'minecraft_server',
            'satisfactory-server'
        ]
    })

if __name__ == '__main__':
    print("=" * 70)
    print("Docker Game Server Monitor")
    print("=" * 70)
    print("\nMonitored Containers:")
    print("  • minecraft_bingo_server")
    print("  • minecraft_server")
    print("  • satisfactory-server")
    print("\nEndpoints:")
    print("  http://localhost:5000/         - API info")
    print("  http://localhost:5000/health   - Health check")
    print("  http://localhost:5000/status   - Server status")
    print("\nServer starting on port 5000...")
    print("Press Ctrl+C to stop\n")
    
    app.run(host='0.0.0.0', port=5000, debug=True)

"""
Test HTTP Server for Docker Server Monitor
Provides mock Docker container metrics for testing the ESP32 client
"""

from flask import Flask, jsonify
from flask_cors import CORS
from datetime import datetime
import random

app = Flask(__name__)
CORS(app)  # Enable CORS for all routes

# Mock container data
MOCK_CONTAINERS = [
    {"name": "web-app", "status": "running"},
    {"name": "database", "status": "running"},
    {"name": "redis-cache", "status": "running"},
    {"name": "nginx-proxy", "status": "running"},
    {"name": "monitoring", "status": "stopped"}
]

@app.route('/status', methods=['GET'])
def get_status():
    """
    Returns mock Docker server status with metrics
    """
    # Simulate varying metrics
    cpu_usage = round(random.uniform(15.0, 85.0), 1)
    memory_usage = round(random.uniform(40.0, 90.0), 1)
    disk_usage = round(random.uniform(30.0, 70.0), 1)
    
    running_containers = sum(1 for c in MOCK_CONTAINERS if c["status"] == "running")
    total_containers = len(MOCK_CONTAINERS)
    
    response = {
        "status": "ok",
        "timestamp": datetime.now().isoformat(),
        "server": {
            "hostname": "docker-host-1",
            "uptime": "15 days, 3 hours"
        },
        "containers": {
            "total": total_containers,
            "running": running_containers,
            "stopped": total_containers - running_containers
        },
        "resources": {
            "cpu_percent": cpu_usage,
            "memory_percent": memory_usage,
            "disk_percent": disk_usage
        },
        "container_list": MOCK_CONTAINERS
    }
    
    return jsonify(response)

@app.route('/health', methods=['GET'])
def health_check():
    """
    Simple health check endpoint
    """
    return jsonify({
        "status": "healthy",
        "service": "docker-monitor-test-server",
        "timestamp": datetime.now().isoformat()
    })

@app.route('/', methods=['GET'])
def index():
    """
    API information endpoint
    """
    return jsonify({
        "name": "Docker Monitor Test Server",
        "version": "1.0.0",
        "endpoints": {
            "/": "API information",
            "/health": "Health check",
            "/status": "Mock Docker container metrics"
        }
    })

if __name__ == '__main__':
    print("=" * 60)
    print("Docker Monitor Test Server")
    print("=" * 60)
    print("\nEndpoints:")
    print("  http://localhost:5000/         - API info")
    print("  http://localhost:5000/health   - Health check")
    print("  http://localhost:5000/status   - Docker metrics (mock data)")
    print("\nServer starting on port 5000...")
    print("Press Ctrl+C to stop\n")
    
    # Run on all interfaces so ESP32 can access it
    app.run(host='0.0.0.0', port=5000, debug=True)

#!/usr/bin/env python3
"""
Web Interface for Multikernel OS
This provides a REST API and WebSocket interface for real-time system visualization
"""

from flask import Flask, render_template, jsonify, send_from_directory
from flask_socketio import SocketIO, emit
from flask_cors import CORS
import subprocess
import threading
import time
import re
import json
import os
from datetime import datetime

app = Flask(__name__, static_folder='web_static', template_folder='web_templates')
CORS(app)
socketio = SocketIO(app, cors_allowed_origins="*")

# Global state
system_state = {
    'running': False,
    'cores': {},
    'stats': {},
    'processes': [],
    'messages': [],
    'start_time': None
}

class MultikernelInterface:
    def __init__(self):
        self.process = None
        self.monitoring_thread = None
        self.running = False
        
    def start_system(self):
        """Start the multikernel OS as a subprocess"""
        if self.running:
            return {"status": "already_running"}
        
        try:
            # Start the multikernel_os process
            self.process = subprocess.Popen(
                ['./multikernel_os'],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                universal_newlines=True,
                bufsize=1
            )
            
            self.running = True
            system_state['running'] = True
            system_state['start_time'] = datetime.now().isoformat()
            
            # Start monitoring thread
            self.monitoring_thread = threading.Thread(target=self._monitor_output)
            self.monitoring_thread.daemon = True
            self.monitoring_thread.start()
            
            return {"status": "started"}
        except Exception as e:
            return {"status": "error", "message": str(e)}
    
    def stop_system(self):
        """Stop the multikernel OS"""
        if not self.running:
            return {"status": "not_running"}
        
        try:
            if self.process:
                self.process.terminate()
                self.process.wait(timeout=5)
            self.running = False
            system_state['running'] = False
            return {"status": "stopped"}
        except Exception as e:
            return {"status": "error", "message": str(e)}
    
    def _monitor_output(self):
        """Monitor the multikernel OS output and parse statistics"""
        while self.running and self.process:
            try:
                line = self.process.stdout.readline()
                if not line:
                    break
                
                # Parse different types of output
                self._parse_line(line.strip())
                
            except Exception as e:
                print(f"Monitor error: {e}")
                break
        
        self.running = False
        system_state['running'] = False
    
    def _parse_line(self, line):
        """Parse output lines and update system state"""
        if not line:
            return
        
        # Parse process creation
        if "Process" in line and "assigned to Core" in line:
            match = re.search(r'Process (\d+) assigned to Core (\d+) \(load=(\d+)\)', line)
            if match:
                pid = int(match.group(1))
                core = int(match.group(2))
                load = int(match.group(3))
                
                # Update state
                if core not in system_state['cores']:
                    system_state['cores'][core] = {'load': 0, 'processes': []}
                
                system_state['cores'][core]['load'] = load
                system_state['cores'][core]['processes'].append(pid)
                system_state['processes'].append({
                    'pid': pid,
                    'core': core,
                    'timestamp': time.time()
                })
                
                # Emit to WebSocket
                socketio.emit('process_created', {
                    'pid': pid,
                    'core': core,
                    'load': load
                })
        
        # Parse statistics
        elif "Messages Sent:" in line:
            match = re.search(r'Messages Sent:\s+(\d+)', line)
            if match:
                # Track message stats
                pass
        
        # Parse core statistics
        elif "--- Core" in line:
            match = re.search(r'--- Core (\d+) ---', line)
            if match:
                core_id = int(match.group(1))
                if core_id not in system_state['stats']:
                    system_state['stats'][core_id] = {}
        
        # Emit general log
        socketio.emit('log', {'message': line, 'timestamp': time.time()})

# Global interface instance
interface = MultikernelInterface()

# ============================================================================
# REST API ENDPOINTS
# ============================================================================

@app.route('/')
def index():
    """Serve the main web interface"""
    return render_template('index.html')

@app.route('/api/status')
def get_status():
    """Get current system status"""
    return jsonify({
        'running': system_state['running'],
        'start_time': system_state['start_time'],
        'cores': len(system_state['cores']),
        'processes': len(system_state['processes'])
    })

@app.route('/api/start', methods=['POST'])
def start_system():
    """Start the multikernel OS"""
    result = interface.start_system()
    return jsonify(result)

@app.route('/api/stop', methods=['POST'])
def stop_system():
    """Stop the multikernel OS"""
    result = interface.stop_system()
    return jsonify(result)

@app.route('/api/stats')
def get_stats():
    """Get detailed statistics"""
    return jsonify({
        'cores': system_state['cores'],
        'stats': system_state['stats'],
        'total_processes': len(system_state['processes']),
        'total_messages': len(system_state['messages'])
    })

@app.route('/api/cores')
def get_cores():
    """Get per-core information"""
    return jsonify(system_state['cores'])

# ============================================================================
# WEBSOCKET EVENTS
# ============================================================================

@socketio.on('connect')
def handle_connect():
    """Handle client connection"""
    print('Client connected')
    emit('status', {'running': system_state['running']})

@socketio.on('disconnect')
def handle_disconnect():
    """Handle client disconnection"""
    print('Client disconnected')

@socketio.on('request_stats')
def handle_stats_request():
    """Send current statistics to client"""
    emit('stats_update', {
        'cores': system_state['cores'],
        'stats': system_state['stats']
    })

# ============================================================================
# MAIN
# ============================================================================

if __name__ == '__main__':
    print("=" * 60)
    print("  MULTIKERNEL OS - WEB INTERFACE")
    print("  Access at: http://localhost:5000")
    print("=" * 60)
    socketio.run(app, host='0.0.0.0', port=5000, debug=True, allow_unsafe_werkzeug=True)

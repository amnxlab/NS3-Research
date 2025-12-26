import http.server
import socketserver
import subprocess
import json
import os
import threading
import time
import queue
from pathlib import Path
import sys

# CONFIG
PORT = 8081
SCRIPT_DIR = Path(__file__).parent.resolve()
RUN_ALL_SCRIPT = SCRIPT_DIR / "run_all.py"

class SimulationManager:
    def __init__(self):
        self.process = None
        self.output_history = []
        self.is_running = False
        self.clients = set() # Queues for active SSE clients
        self.lock = threading.Lock()

    def start_simulation(self, cmd):
        with self.lock:
            if self.is_running:
                return False, "Simulation already running"
            
            self.output_history = [] # Clear previous history
            self.is_running = True
            
            # Start background thread
            threading.Thread(target=self._run_process, args=(cmd,), daemon=True).start()
            return True, "Started"

    def stop_simulation(self):
        with self.lock:
            if self.process and self.is_running:
                self.process.terminate()
                self.broadcast("data: [ABORTED BY USER]\n\n")
                return True
            return False

    def _run_process(self, cmd):
        try:
            self.broadcast(f"data: [STARTING] {' '.join(cmd)}\n\n")
            
            self.process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
                cwd=SCRIPT_DIR
            )

            for line in self.process.stdout:
                msg = f"data: {line.strip()}\n\n"
                self.broadcast(msg)
            
            self.process.wait()
            self.broadcast(f"data: [FINISHED] Exit: {self.process.returncode}\n\n")
            
        except Exception as e:
            self.broadcast(f"data: [ERROR] {str(e)}\n\n")
        finally:
            with self.lock:
                self.is_running = False
                self.process = None

    def broadcast(self, msg):
        # Store in history
        self.output_history.append(msg)
        # Send to all connected clients
        dead_clients = set()
        for q in self.clients:
            try:
                q.put(msg)
            except:
                dead_clients.add(q)
        
        # Cleanup
        if dead_clients:
            with self.lock:
                self.clients -= dead_clients

    def register_client(self):
        q = queue.Queue()
        with self.lock:
            self.clients.add(q)
            # Send history first
            for msg in self.output_history:
                q.put(msg)
        return q

    def remove_client(self, q):
        with self.lock:
            if q in self.clients:
                self.clients.remove(q)

# Global Singleton
sim_manager = SimulationManager()

class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    pass

class SimulationHandler(http.server.BaseHTTPRequestHandler):
    def _set_headers(self, status=200, content_type='application/json'):
        self.send_response(status)
        self.send_header('Content-type', content_type)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'POST, GET, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()

    def do_OPTIONS(self):
        self._set_headers()

    def do_GET(self):
        if self.path == '/list_graphs':
            args = {}
            # Simple manual listing of graphs directory
            try:
                graphs = []
                graphs_dir = SCRIPT_DIR / "graphs"
                if graphs_dir.exists():
                    # Get png files, sorted by newest first
                    files = sorted(graphs_dir.glob("*.png"), key=os.path.getmtime, reverse=True)
                    graphs = [f.name for f in files]
                
                self._set_headers()
                self.wfile.write(json.dumps(graphs).encode())
            except Exception as e:
                self._set_headers(500)
                self.wfile.write(json.dumps({"error": str(e)}).encode())

    def do_POST(self):
        if self.path == '/run':
            content_length = int(self.headers['Content-Length'])
            post_data = self.rfile.read(content_length)
            data = json.loads(post_data)

            # Build Command w/ same logic as before
            cmd = ['python3', str(RUN_ALL_SCRIPT)]
            
            # Protocols
            protocols = data.get('protocols', [])
            if not protocols:
                self._set_headers(400)
                self.wfile.write(json.dumps({"error": "No protocols"}).encode())
                return
            cmd.extend(['--protocols'] + protocols)
            
            # List params
            list_params = ['nodes', 'range', 'min_z', 'max_z', 'area_x', 'area_y', 'vbf_width', 'simtime', 'initial_energy']
            for p in list_params:
                val = data.get(p)
                if val:
                    cmd.append(f'--{p.replace("_", "-")}')
                    cmd.extend([v.strip() for v in str(val).split(',')])
            
            # Single params
            single_params = ['runs', 'mobility']
            for p in single_params:
                val = data.get(p)
                if val:
                    cmd.append(f'--{p.replace("_", "-")}')
                    cmd.append(str(val))
            
            # Flags
            if data.get('skip_build'): cmd.append('--skip-build')
            if data.get('skip_analysis'): cmd.append('--skip-analysis')
            if data.get('skip_plots'): cmd.append('--skip-plots')

            # Execute - We will stream this differently for simple HTTP
            # For simplicity, we'll run it and return the process ID or start a thread
            # To emulate streaming, we use chunked transfer encoding manually
            
            self.send_response(200)
            self.send_header('Content-Type', 'text/event-stream')
            self.send_header('Cache-Control', 'no-cache')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()

            self.wfile.write(f"data: [STARTING SIMULATION]\n\n".encode())
            self.wfile.write(f"data: CMD: {' '.join(cmd)}\n\n".encode())
            
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
                cwd=SCRIPT_DIR
            )

            for line in process.stdout:
                msg = f"data: {line.strip()}\n\n"
                self.wfile.write(msg.encode())
                self.wfile.flush()
            
            process.wait()
            self.wfile.write(f"data: [FINISHED] Exit: {process.returncode}\n\n".encode())

print(f"✅  API Server listening on port {PORT}")
with socketserver.TCPServer(("", PORT), SimulationHandler) as httpd:
    httpd.serve_forever()

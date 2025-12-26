#!/usr/bin/env python3
"""
Minimal UWSN Dashboard API Server
Simple, reliable, no dependencies beyond stdlib.
"""
import http.server
import json
import os
import subprocess
import threading
import urllib.parse
from pathlib import Path

PORT = 8081
SCRIPT_DIR = Path(__file__).parent.resolve()
RUN_ALL = SCRIPT_DIR / "run_all.py"
GRAPHS_DIR = SCRIPT_DIR / "graphs"

# === Global State ===
class State:
    running = False
    output = []
    process = None
    lock = threading.Lock()

state = State()

def run_simulation(cmd):
    """Background thread to run simulation."""
    try:
        state.output.append(f"$ {' '.join(cmd)}")
        
        env = os.environ.copy()
        env['PYTHONUNBUFFERED'] = '1'
        
        state.process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            cwd=str(SCRIPT_DIR),
            env=env
        )
        
        for line in state.process.stdout:
            state.output.append(line.rstrip('\n\r'))
        
        state.process.wait()
        state.output.append(f"[EXIT CODE: {state.process.returncode}]")
    except Exception as e:
        state.output.append(f"[ERROR: {e}]")
    finally:
        with state.lock:
            state.running = False
            state.process = None

class APIHandler(http.server.BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass  # Silent
    
    def send_json(self, data, status=200):
        body = json.dumps(data).encode()
        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', len(body))
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.send_header('Cache-Control', 'no-cache')
        self.end_headers()
        self.wfile.write(body)
    
    def do_OPTIONS(self):
        self.send_json({})
    
    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        query = urllib.parse.parse_qs(parsed.query)
        
        if path == '/status':
            self.send_json({
                'running': state.running,
                'lines': len(state.output)
            })
        
        elif path == '/output':
            from_idx = int(query.get('from', ['0'])[0])
            self.send_json({
                'running': state.running,
                'lines': state.output[from_idx:],
                'total': len(state.output)
            })
        
        elif path == '/folders':
            # List all simulation folders (sorted newest first)
            folders = []
            if GRAPHS_DIR.exists():
                for d in sorted(GRAPHS_DIR.iterdir(), key=os.path.getmtime, reverse=True):
                    if d.is_dir():
                        # Count images in folder
                        images = list(d.glob("*.png"))
                        folders.append({
                            'name': d.name,
                            'count': len(images),
                            'mtime': os.path.getmtime(d)
                        })
            self.send_json(folders)
        
        elif path == '/folder':
            # Get images in a specific folder
            folder_name = query.get('name', [''])[0]
            folder_path = GRAPHS_DIR / folder_name
            images = []
            if folder_path.exists() and folder_path.is_dir():
                for f in sorted(folder_path.glob("*.png"), key=os.path.getmtime, reverse=True):
                    images.append(f.name)
            self.send_json(images)
        
        elif path == '/graphs':
            # Legacy: list all PNG files in graphs dir (flat)
            files = []
            if GRAPHS_DIR.exists():
                for f in sorted(GRAPHS_DIR.glob("*.png"), key=os.path.getmtime, reverse=True):
                    files.append(f.name)
            self.send_json(files)
        
        else:
            self.send_json({'error': 'Not found'}, 404)
    
    def do_POST(self):
        if self.path == '/run':
            # Check if already running
            with state.lock:
                if state.running:
                    self.send_json({'error': 'Already running'}, 400)
                    return
                state.running = True
                state.output = []
            
            # Parse body
            length = int(self.headers.get('Content-Length', 0))
            body = self.rfile.read(length).decode() if length > 0 else '{}'
            data = json.loads(body)
            
            # Build command
            cmd = ['python3', '-u', str(RUN_ALL)]
            
            protocols = data.get('protocols', [])
            if protocols:
                cmd.extend(['--protocols'] + protocols)
            
            # Add params
            for key in ['nodes', 'range', 'min_z', 'max_z', 'area_x', 'area_y', 'vbf_width', 'simtime', 'initial_energy']:
                val = data.get(key)
                if val:
                    cmd.append(f'--{key.replace("_", "-")}')
                    for v in str(val).split(','):
                        if v.strip():
                            cmd.append(v.strip())
            
            for key in ['runs', 'mobility']:
                val = data.get(key)
                if val:
                    cmd.append(f'--{key}')
                    cmd.append(str(val))
            
            if data.get('skip_build'):
                cmd.append('--skip-build')
            if data.get('skip_analysis'):
                cmd.append('--skip-analysis')
            if data.get('skip_plots'):
                cmd.append('--skip-plots')
            
            # Start background thread
            threading.Thread(target=run_simulation, args=(cmd,), daemon=True).start()
            self.send_json({'status': 'started'})
        
        elif self.path == '/stop':
            with state.lock:
                if state.process and state.running:
                    state.process.terminate()
                    state.output.append("[ABORTED BY USER]")
                    self.send_json({'status': 'stopped'})
                else:
                    self.send_json({'status': 'not_running'})
        
        else:
            self.send_json({'error': 'Not found'}, 404)

if __name__ == '__main__':
    from http.server import ThreadingHTTPServer
    server = ThreadingHTTPServer(('0.0.0.0', PORT), APIHandler)
    print(f"✅ API running on http://localhost:{PORT}")
    server.serve_forever()

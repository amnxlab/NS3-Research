#!/usr/bin/env python3
import os
import sys
import subprocess
import shutil
import time
import urllib.request
import webbrowser
from pathlib import Path

def main():
    print("------------------------------------------------")
    print("🚀 UWSN Dashboard Launcher (Python Version)")
    print("------------------------------------------------")

    # 1. Kill old processes
    print("Stopping old servers...")
    subprocess.run(["pkill", "-f", "python3 -m http.server 8000"])
    subprocess.run(["pkill", "-f", "api_server.py"])

    # 2. Setup Paths
    script_dir = Path(__file__).parent.resolve()
    ns3_root = script_dir.parent
    
    public_dir = script_dir / "public"
    public_results = public_dir / "results"
    public_graphs = public_dir / "graphs"
    
    real_results = ns3_root / "results"
    real_graphs = script_dir / "graphs"

    # Ensure directories exist
    real_results.mkdir(parents=True, exist_ok=True)
    real_graphs.mkdir(parents=True, exist_ok=True)
    public_dir.mkdir(parents=True, exist_ok=True)

    # 3. Create Symlinks
    print("🔧 Configuring Symlinks...")
    if public_results.exists() or public_results.is_symlink():
        public_results.unlink()
    public_results.symlink_to(real_results)

    if public_graphs.exists() or public_graphs.is_symlink():
        public_graphs.unlink()
    public_graphs.symlink_to(real_graphs)

    # 3.5 Create VS Code settings for auto port forwarding
    vscode_dir = ns3_root / ".vscode"
    vscode_dir.mkdir(parents=True, exist_ok=True)
    vscode_settings = vscode_dir / "settings.json"
    
    import json
    settings = {}
    if vscode_settings.exists():
        try:
            with open(vscode_settings) as f:
                settings = json.load(f)
        except:
            pass
    
    # Add port forwarding settings
    settings["remote.autoForwardPorts"] = True
    settings["remote.autoForwardPortsSource"] = "process"
    settings["remote.portsAttributes"] = {
        "8000": {"label": "UWSN Web Server", "onAutoForward": "openBrowser"},
        "8081": {"label": "UWSN API Server", "onAutoForward": "silent"}
    }
    
    with open(vscode_settings, "w") as f:
        json.dump(settings, f, indent=4)
    print("📝 VS Code port forwarding configured...")

    # 4. Start Servers
    print("🌐 Starting Web Server (8000)...")
    web_log = open(public_dir / "web_server.log", "w")
    subprocess.Popen(
        ["python3", "-m", "http.server", "8000"],
        cwd=public_dir,
        stdout=web_log,
        stderr=web_log
    )

    print("🔌 Starting API Server (8081)...")
    api_log = open(script_dir / "api_server.log", "w")
    subprocess.Popen(
        ["python3", "-u", str(script_dir / "api_server.py")],
        cwd=ns3_root, 
        stdout=api_log,
        stderr=api_log
    )

    # Wait for API to be ready (up to 5 seconds)
    print("⏳ Waiting for API server to be ready...")
    api_ready = False
    for i in range(10):
        try:
            req = urllib.request.urlopen("http://localhost:8081/status", timeout=1)
            if req.status == 200:
                api_ready = True
                break
        except:
            pass
        time.sleep(0.5)
    
    if api_ready:
        print("✅ API server is ready!")
    else:
        print("⚠️  API server may still be starting...")

    print("------------------------------------------------")
    print("✅ DASHBOARD LIVE AT: http://localhost:8000")
    print("------------------------------------------------")
    
    # Auto-open browser
    print("🌐 Opening browser...")
    webbrowser.open("http://localhost:8000")

if __name__ == "__main__":
    main()

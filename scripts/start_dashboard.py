#!/usr/bin/env python3
"""
UWSN Dashboard Launcher
Starts the combined server (web + API on port 8000).
"""
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
    print("🚀 UWSN Dashboard Launcher")
    print("------------------------------------------------")

    # 1. Kill old processes
    print("Stopping old servers...")
    subprocess.run(["pkill", "-f", "python3 -m http.server 8000"], capture_output=True)
    subprocess.run(["pkill", "-f", "api_server.py"], capture_output=True)
    subprocess.run(["pkill", "-f", "combined_server.py"], capture_output=True)

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

    # 4. Install EDBR/EVBF protocols (auto-integration with AquaSim-NG)
    print("📦 Installing EDBR/EVBF protocols...")
    install_script = script_dir / "install_edbr_evbf.sh"
    if install_script.exists():
        result = subprocess.run(
            ["bash", str(install_script)],
            cwd=script_dir,
            capture_output=True,
            text=True
        )
        if result.returncode == 0:
            print("✅ EDBR/EVBF protocols installed successfully!")
        else:
            print("⚠️  EDBR/EVBF installation had issues (may already be installed)")
            # Don't fail - protocols might already be installed
    else:
        print("⚠️  install_edbr_evbf.sh not found - skipping protocol installation")

    # 5. Start Combined Server (Web + API on port 8000)
    print("🌐 Starting Combined Server (port 8000)...")
    server_log = open(script_dir / "server.log", "w")
    subprocess.Popen(
        ["python3", "-u", str(script_dir / "combined_server.py")],
        cwd=ns3_root, 
        stdout=server_log,
        stderr=server_log
    )

    # Wait for server to be ready (up to 5 seconds)
    print("⏳ Waiting for server to be ready...")
    server_ready = False
    for i in range(10):
        try:
            req = urllib.request.urlopen("http://localhost:8000/status", timeout=1)
            if req.status == 200:
                server_ready = True
                break
        except:
            pass
        time.sleep(0.5)
    
    if server_ready:
        print("✅ Server is ready!")
    else:
        print("⚠️  Server may still be starting...")

    print("------------------------------------------------")
    print("✅ DASHBOARD: http://localhost:8000")
    print("   (Single port - no forwarding issues!)")
    print("------------------------------------------------")
    
    # Auto-open browser
    print("🌐 Opening browser...")
    webbrowser.open("http://localhost:8000")

if __name__ == "__main__":
    main()

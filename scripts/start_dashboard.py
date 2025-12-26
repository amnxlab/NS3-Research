#!/usr/bin/env python3
import os
import sys
import subprocess
import shutil
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

    # 4. Start Servers
    print("🌐 Starting Web Server (8000)...")
    # Using subprocess.Popen to run in background
    # We change cwd to public_dir for the static server
    subprocess.Popen(
        ["python3", "-m", "http.server", "8000"],
        cwd=public_dir,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL
    )

    print("🔌 Starting API Server (8081)...")
    # API server needs to run from NS3 root or scripts? 
    # api_server.py expects to be run such that `scripts/start_dashboard.py` calls it?
    # Actually api_server.py handles paths relative to itself.
    subprocess.Popen(
        ["python3", str(script_dir / "api_server.py")],
        cwd=ns3_root, # Run from root
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL
    )

    print("------------------------------------------------")
    print("✅ DASHBOARD LIVE AT: http://localhost:8000")
    print("------------------------------------------------")

if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
UWSN Protocol Comparison - Experiment Runner
=============================================
Automates running multiple simulation experiments with different:
- Protocols (VBF, DBR, flooding)
- Node counts
- Data rates (CBR traffic)
- Random seeds (run numbers)

Usage:
    python run_experiments.py [--runs 30] [--parallel 4]
    python run_experiments.py --data-rates 1000 5000 10000 --runs 10
"""

import subprocess
import os
import sys
import argparse
import time
from pathlib import Path
from multiprocessing import Pool, cpu_count
from datetime import datetime

# Configuration - Updated paths after reorganization
NS3_PATH = Path.home() / "ns3" / "ns-allinone-3.42" / "ns-3.42"
SCRATCH_PATH = NS3_PATH / "scratch"
RESEARCH_DIR = Path.home() / "NS3-Research"
RESULTS_DIR = RESEARCH_DIR / "results"
SRC_DIR = RESEARCH_DIR / "src"
PROGRAM_NAME = "uwsn-research"

# Experiment parameters
PROTOCOLS = ["VBF", "DBR", "flooding"]
NODE_COUNTS = [50, 100, 150, 200]
DATA_RATES = [10000]  # CBR data rates in bps (default: 10kbps)
PACKET_SIZE = 40  # bytes
DEFAULT_RUNS = 30
DEFAULT_SIM_TIME = 1000


def setup_simulation():
    """Copy simulation file to scratch folder and build."""
    source_file = SRC_DIR / "uwsn-research.cc"
    dest_file = SCRATCH_PATH / "uwsn-research.cc"
    
    if not source_file.exists():
        print(f"ERROR: Source file not found: {source_file}")
        sys.exit(1)
    
    # Copy to scratch
    print(f"Copying {source_file} to {dest_file}")
    import shutil
    shutil.copy2(source_file, dest_file)
    
    # Build
    print("Building ns-3...")
    os.chdir(NS3_PATH)
    result = subprocess.run(
        ["./ns3", "build"],
        capture_output=True,
        text=True
    )
    
    if result.returncode != 0:
        print("Build failed!")
        print(result.stderr)
        sys.exit(1)
    
    print("Build successful!")
    return True


def run_single_experiment(params):
    """Run a single experiment with given parameters."""
    protocol, nodes, run_number, sim_time, data_rate, packet_size, csv_file = params
    
    cmd = [
        "./ns3", "run",
        f"{PROGRAM_NAME} "
        f"--protocol={protocol} "
        f"--nodes={nodes} "
        f"--runNumber={run_number} "
        f"--simStop={sim_time} "
        f"--dataRate={data_rate} "
        f"--packetSize={packet_size} "
        f"--csvFile={csv_file}"
    ]
    
    try:
        os.chdir(NS3_PATH)
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=3600  # 1 hour timeout per run
        )
        
        if result.returncode == 0:
            return (True, protocol, nodes, run_number, data_rate, None)
        else:
            return (False, protocol, nodes, run_number, data_rate, result.stderr)
    
    except subprocess.TimeoutExpired:
        return (False, protocol, nodes, run_number, data_rate, "Timeout")
    except Exception as e:
        return (False, protocol, nodes, run_number, data_rate, str(e))


def run_experiments_sequential(experiments, csv_file, packet_size):
    """Run experiments sequentially with progress tracking."""
    total = len(experiments)
    completed = 0
    failed = []
    
    print(f"\nRunning {total} experiments sequentially...")
    print("=" * 60)
    
    start_time = time.time()
    
    for params in experiments:
        protocol, nodes, run_num, sim_time, data_rate = params
        params_with_csv = (protocol, nodes, run_num, sim_time, data_rate, packet_size, csv_file)
        
        completed += 1
        progress = (completed / total) * 100
        elapsed = time.time() - start_time
        eta = (elapsed / completed) * (total - completed) if completed > 0 else 0
        
        print(f"[{completed}/{total}] ({progress:.1f}%) "
              f"Protocol={protocol}, Nodes={nodes}, Rate={data_rate}bps, Run={run_num} "
              f"| ETA: {eta/60:.1f}min")
        
        success, proto, n, run, rate, err = run_single_experiment(params_with_csv)
        
        if not success:
            failed.append((proto, n, run, rate, err))
            print(f"  FAILED: {err[:50]}..." if err else "  FAILED")
    
    return failed


def run_experiments_parallel(experiments, csv_file, num_workers, packet_size):
    """Run experiments in parallel."""
    total = len(experiments)
    
    # Add csv_file and packet_size to all params
    experiments_with_csv = [
        (p, n, r, s, rate, packet_size, csv_file) for p, n, r, s, rate in experiments
    ]
    
    print(f"\nRunning {total} experiments with {num_workers} parallel workers...")
    print("=" * 60)
    
    start_time = time.time()
    failed = []
    completed = 0
    
    with Pool(num_workers) as pool:
        for result in pool.imap_unordered(run_single_experiment, experiments_with_csv):
            completed += 1
            success, proto, n, run, rate, err = result
            
            progress = (completed / total) * 100
            elapsed = time.time() - start_time
            eta = (elapsed / completed) * (total - completed) if completed > 0 else 0
            
            status = "OK" if success else "FAIL"
            print(f"[{completed}/{total}] ({progress:.1f}%) "
                  f"{proto}, Nodes={n}, Rate={rate}bps, Run={run} -> {status} "
                  f"| ETA: {eta/60:.1f}min")
            
            if not success:
                failed.append((proto, n, run, rate, err))
    
    return failed


def main():
    parser = argparse.ArgumentParser(
        description="UWSN Protocol Comparison - Experiment Runner"
    )
    parser.add_argument(
        "--runs", type=int, default=DEFAULT_RUNS,
        help=f"Number of runs per configuration (default: {DEFAULT_RUNS})"
    )
    parser.add_argument(
        "--parallel", type=int, default=0,
        help="Number of parallel workers (0 = sequential, default: 0)"
    )
    parser.add_argument(
        "--protocols", nargs="+", default=PROTOCOLS,
        help=f"Protocols to test (default: {PROTOCOLS})"
    )
    parser.add_argument(
        "--nodes", nargs="+", type=int, default=NODE_COUNTS,
        help=f"Node counts to test (default: {NODE_COUNTS})"
    )
    parser.add_argument(
        "--simtime", type=int, default=DEFAULT_SIM_TIME,
        help=f"Simulation time in seconds (default: {DEFAULT_SIM_TIME})"
    )
    parser.add_argument(
        "--data-rates", nargs="+", type=int, default=DATA_RATES,
        help=f"CBR data rates in bps (default: {DATA_RATES})"
    )
    parser.add_argument(
        "--packet-size", type=int, default=PACKET_SIZE,
        help=f"Packet size in bytes (default: {PACKET_SIZE})"
    )
    parser.add_argument(
        "--skip-build", action="store_true",
        help="Skip copying and building (use existing build)"
    )
    parser.add_argument(
        "--output", type=str, default=None,
        help="Output CSV file path"
    )
    
    args = parser.parse_args()
    
    # Create results directory
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)
    
    # Generate output filename
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    if args.output:
        csv_file = args.output
    else:
        csv_file = str(RESULTS_DIR / f"uwsn_results_{timestamp}.csv")
    
    print("=" * 60)
    print("   UWSN Protocol Comparison - Experiment Runner")
    print("=" * 60)
    print(f"Protocols: {args.protocols}")
    print(f"Node counts: {args.nodes}")
    print(f"CBR Data rates: {args.data_rates} bps")
    print(f"Packet size: {args.packet_size} bytes")
    print(f"Runs per config: {args.runs}")
    print(f"Simulation time: {args.simtime}s")
    print(f"Output file: {csv_file}")
    print(f"Parallel workers: {args.parallel if args.parallel > 0 else 'Sequential'}")
    print("=" * 60)
    
    # Setup
    if not args.skip_build:
        setup_simulation()
    
    # Generate experiment list
    experiments = []
    for protocol in args.protocols:
        for node_count in args.nodes:
            for data_rate in args.data_rates:
                for run in range(1, args.runs + 1):
                    experiments.append((protocol, node_count, run, args.simtime, data_rate))
    
    total_experiments = len(experiments)
    print(f"\nTotal experiments to run: {total_experiments}")
    
    # Estimate time
    est_time_per_run = 60  # seconds (rough estimate)
    if args.parallel > 0:
        est_total = (total_experiments * est_time_per_run) / args.parallel
    else:
        est_total = total_experiments * est_time_per_run
    print(f"Estimated time: {est_total/3600:.1f} hours")
    
    # Confirm
    response = input("\nProceed? [Y/n]: ").strip().lower()
    if response and response != 'y':
        print("Aborted.")
        sys.exit(0)
    
    # Run experiments
    start_time = time.time()
    
    if args.parallel > 0:
        failed = run_experiments_parallel(experiments, csv_file, args.parallel, args.packet_size)
    else:
        failed = run_experiments_sequential(experiments, csv_file, args.packet_size)
    
    elapsed = time.time() - start_time
    
    # Summary
    print("\n" + "=" * 60)
    print("   EXPERIMENT COMPLETE")
    print("=" * 60)
    print(f"Total experiments: {total_experiments}")
    print(f"Successful: {total_experiments - len(failed)}")
    print(f"Failed: {len(failed)}")
    print(f"Total time: {elapsed/3600:.2f} hours ({elapsed/60:.1f} minutes)")
    print(f"Results saved to: {csv_file}")
    
    if failed:
        print("\nFailed experiments:")
        for proto, n, run, rate, err in failed[:10]:  # Show first 10
            print(f"  - {proto}, Nodes={n}, Rate={rate}bps, Run={run}: {err[:50] if err else 'Unknown error'}")
        if len(failed) > 10:
            print(f"  ... and {len(failed) - 10} more")
    
    print("\nNext steps:")
    print(f"  1. Analyze results: python analyze_results.py {csv_file}")
    print(f"  2. Generate plots: python plot_results.py {csv_file}")


if __name__ == "__main__":
    main()


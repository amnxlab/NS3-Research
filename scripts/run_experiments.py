#!/usr/bin/env python3
"""
UWSN Protocol Comparison - Experiment Runner
=============================================
Automates running multiple simulation experiments with different:
- Protocols (VBF, DBR, flooding)
- Node counts
- Random seeds (run numbers)

Parses simulation output to extract PDR from Aqua-Sim channel counters.

Usage:
    python run_experiments.py [--runs 30] [--parallel 4]
    python run_experiments.py --protocols VBF DBR --nodes 100 --runs 10
"""

import subprocess
import os
import sys
import argparse
import time
import re
import csv
from pathlib import Path
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


def parse_simulation_output(output):
    """
    Parse simulation stdout to extract metrics from Aqua-Sim PrintCounters.
    
    Expected format from PrintCounters:
        Sent Pkts: 1234
        Sent Data: 5678
        Recv Pkts: 900
        ...
    """
    metrics = {
        'sent_pkts': 0,
        'recv_pkts': 0,
        'pdr': 0.0
    }
    
    # Parse Aqua-Sim channel counter output
    # Look for patterns like "Sent Pkts: 1234" or "SendUp Pkts: 567"
    sent_match = re.search(r'Sent\s+Pkts[:\s]+(\d+)', output, re.IGNORECASE)
    recv_match = re.search(r'(?:Recv|SendUp)\s+Pkts[:\s]+(\d+)', output, re.IGNORECASE)
    
    if sent_match:
        metrics['sent_pkts'] = int(sent_match.group(1))
    
    if recv_match:
        metrics['recv_pkts'] = int(recv_match.group(1))
    
    # Calculate PDR
    if metrics['sent_pkts'] > 0:
        metrics['pdr'] = metrics['recv_pkts'] / metrics['sent_pkts']
    
    return metrics


def run_single_experiment(params):
    """Run a single experiment with given parameters."""
    protocol, nodes, run_number, sim_time, data_rate, packet_size, range_m = params
    
    cmd = [
        "./ns3", "run",
        f"{PROGRAM_NAME} "
        f"--protocol={protocol} "
        f"--nodes={nodes} "
        f"--runNumber={run_number} "
        f"--simStop={sim_time} "
        f"--dataRate={data_rate} "
        f"--packetSize={packet_size} "
        f"--range={range_m} "
        f"--csvFile=/dev/null"  # Don't write individual CSVs
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
            # Parse the output to get PDR and other metrics
            metrics = parse_simulation_output(result.stdout)
            return {
                'success': True,
                'protocol': protocol,
                'nodes': nodes,
                'run_number': run_number,
                'sim_time': sim_time,
                'data_rate': data_rate,
                'packet_size': packet_size,
                'range': range_m,
                'sent_pkts': metrics['sent_pkts'],
                'recv_pkts': metrics['recv_pkts'],
                'pdr': metrics['pdr'],
                'stdout': result.stdout,
                'error': None
            }
        else:
            return {
                'success': False,
                'protocol': protocol,
                'nodes': nodes,
                'run_number': run_number,
                'error': result.stderr
            }
    
    except subprocess.TimeoutExpired:
        return {
            'success': False,
            'protocol': protocol,
            'nodes': nodes,
            'run_number': run_number,
            'error': "Timeout"
        }
    except Exception as e:
        return {
            'success': False,
            'protocol': protocol,
            'nodes': nodes,
            'run_number': run_number,
            'error': str(e)
        }


def parse_energy_from_output(output):
    """Extract energy metrics from simulation output."""
    energy = {
        'network_lifetime': 0.0,
        'dead_nodes': 0,
        'total_initial_energy': 0.0,
        'total_consumed_energy': 0.0,
        'total_residual_energy': 0.0,
        'avg_energy_per_node': 0.0
    }
    
    # Parse various metrics from output
    patterns = {
        'network_lifetime': r'Network Lifetime:\s+([\d.]+)',
        'dead_nodes': r'Dead Nodes:\s+(\d+)',
        'total_initial_energy': r'Total Initial Energy:\s+([\d.]+)',
        'total_consumed_energy': r'Total Consumed Energy:\s+([\d.]+)',
        'total_residual_energy': r'Total Residual Energy:\s+([\d.]+)',
        'avg_energy_per_node': r'Avg Energy/Node:\s+([\d.]+)'
    }
    
    for key, pattern in patterns.items():
        match = re.search(pattern, output, re.IGNORECASE)
        if match:
            energy[key] = float(match.group(1))
    
    return energy


def run_experiments(experiments, csv_file):
    """Run all experiments and save results to CSV."""
    total = len(experiments)
    completed = 0
    failed = []
    results = []
    
    print(f"\nRunning {total} experiments...")
    print("=" * 70)
    
    start_time = time.time()
    
    # Open CSV for writing
    with open(csv_file, 'w', newline='') as f:
        fieldnames = [
            'run', 'protocol', 'nodes', 'sim_time', 'range',
            'data_rate_bps', 'packet_size_bytes',
            'sent_pkts', 'recv_pkts', 'pdr',
            'network_lifetime_s', 'dead_nodes',
            'total_initial_energy', 'total_consumed_energy',
            'total_residual_energy', 'avg_energy_per_node'
        ]
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        
        for params in experiments:
            result = run_single_experiment(params)
            completed += 1
            
            progress = (completed / total) * 100
            elapsed = time.time() - start_time
            eta = (elapsed / completed) * (total - completed) if completed > 0 else 0
            
            if result['success']:
                # Parse energy from output
                energy = parse_energy_from_output(result['stdout'])
                
                # Write to CSV
                row = {
                    'run': result['run_number'],
                    'protocol': result['protocol'],
                    'nodes': result['nodes'],
                    'sim_time': result['sim_time'],
                    'range': result['range'],
                    'data_rate_bps': result['data_rate'],
                    'packet_size_bytes': result['packet_size'],
                    'sent_pkts': result['sent_pkts'],
                    'recv_pkts': result['recv_pkts'],
                    'pdr': result['pdr'],
                    'network_lifetime_s': energy['network_lifetime'],
                    'dead_nodes': energy['dead_nodes'],
                    'total_initial_energy': energy['total_initial_energy'],
                    'total_consumed_energy': energy['total_consumed_energy'],
                    'total_residual_energy': energy['total_residual_energy'],
                    'avg_energy_per_node': energy['avg_energy_per_node']
                }
                writer.writerow(row)
                f.flush()  # Ensure data is written
                
                print(f"[{completed}/{total}] ({progress:.1f}%) "
                      f"{result['protocol']:8} n={result['nodes']:3} run={result['run_number']:2} "
                      f"PDR={result['pdr']*100:5.1f}% sent={result['sent_pkts']:5} recv={result['recv_pkts']:4} "
                      f"| ETA: {eta/60:.1f}min")
            else:
                failed.append(result)
                print(f"[{completed}/{total}] ({progress:.1f}%) "
                      f"{result['protocol']:8} n={result['nodes']:3} run={result['run_number']:2} "
                      f"FAILED: {result['error'][:40] if result['error'] else 'Unknown'}")
    
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
        "--data-rate", type=int, default=10000,
        help="Data rate in bps (default: 10000)"
    )
    parser.add_argument(
        "--packet-size", type=int, default=40,
        help="Packet size in bytes (default: 40)"
    )
    parser.add_argument(
        "--range", type=int, default=100,
        help="Transmission range in meters (default: 100)"
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
    
    print("=" * 70)
    print("   UWSN Protocol Comparison - Experiment Runner")
    print("=" * 70)
    print(f"Protocols: {args.protocols}")
    print(f"Node counts: {args.nodes}")
    print(f"Data rate: {args.data_rate} bps")
    print(f"Packet size: {args.packet_size} bytes")
    print(f"Transmission range: {args.range} m")
    print(f"Runs per config: {args.runs}")
    print(f"Simulation time: {args.simtime}s")
    print(f"Output file: {csv_file}")
    
    # Calculate total experiments
    total_experiments = len(args.protocols) * len(args.nodes) * args.runs
    print(f"\nTotal experiments to run: {total_experiments}")
    print(f"  = {len(args.protocols)} protocols × {len(args.nodes)} node counts × {args.runs} runs")
    
    # Estimate time
    est_time_per_run = args.simtime / 20  # Rough estimate
    est_total = total_experiments * est_time_per_run
    print(f"Estimated time: {est_total/60:.1f} minutes ({est_total/3600:.2f} hours)")
    print("=" * 70)
    
    # Setup
    if not args.skip_build:
        setup_simulation()
    
    # Generate experiment list
    experiments = []
    for protocol in args.protocols:
        for node_count in args.nodes:
            for run in range(1, args.runs + 1):
                experiments.append((
                    protocol, node_count, run, args.simtime,
                    args.data_rate, args.packet_size, args.range
                ))
    
    # Confirm
    response = input("\nProceed? [Y/n]: ").strip().lower()
    if response and response != 'y':
        print("Aborted.")
        sys.exit(0)
    
    # Run experiments
    start_time = time.time()
    failed = run_experiments(experiments, csv_file)
    elapsed = time.time() - start_time
    
    # Summary
    print("\n" + "=" * 70)
    print("   EXPERIMENT COMPLETE")
    print("=" * 70)
    print(f"Total experiments: {total_experiments}")
    print(f"Successful: {total_experiments - len(failed)}")
    print(f"Failed: {len(failed)}")
    print(f"Total time: {elapsed/60:.1f} minutes ({elapsed/3600:.2f} hours)")
    print(f"Results saved to: {csv_file}")
    
    if failed:
        print(f"\nFailed experiments ({len(failed)}):")
        for result in failed[:10]:
            print(f"  - {result['protocol']}, Nodes={result['nodes']}, "
                  f"Run={result['run_number']}: {result['error'][:50] if result['error'] else 'Unknown'}")
        if len(failed) > 10:
            print(f"  ... and {len(failed) - 10} more")
    
    print("\nNext steps:")
    print(f"  1. Analyze results: python analyze_results.py {csv_file}")
    print(f"  2. Generate plots:  python plot_results.py {csv_file}")


if __name__ == "__main__":
    main()

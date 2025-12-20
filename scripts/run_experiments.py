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

# Auto-detect paths
def find_ns3_installation():
    """Find ns-3 installation (version 3.40-3.49) by scanning user home directory."""
    script_dir = Path(__file__).resolve().parent
    research_dir = script_dir.parent
    
    # Pattern to match ns-3.4x versions (3.40 through 3.49)
    ns3_version_pattern = re.compile(r'ns-3\.4\d$')
    ns3_allinone_pattern = re.compile(r'ns-allinone-3\.4\d$')
    
    found_installations = []
    
    print("Searching for ns-3.4x installation in home directory...")
    
    # Search home directory thoroughly
    home_dir = Path.home()
    try:
        # Scan all directories in home (one level deep)
        for item in home_dir.iterdir():
            if not item.is_dir() or item.name.startswith('.'):
                continue
            
            try:
                # Pattern 1: Check if this is ns-allinone-3.4x
                if ns3_allinone_pattern.match(item.name):
                    for subitem in item.iterdir():
                        if subitem.is_dir() and ns3_version_pattern.match(subitem.name):
                            if (subitem / "ns3").exists():
                                found_installations.append(subitem)
                                print(f"  Found: {subitem}")
                                break
                
                # Pattern 2: Check if this is direct ns-3.4x directory
                elif ns3_version_pattern.match(item.name):
                    if (item / "ns3").exists():
                        found_installations.append(item)
                        print(f"  Found: {item}")
                
                # Pattern 3: Check subdirectories (e.g., ~/ns3/ns-allinone-3.4x/ns-3.4x)
                else:
                    for subitem in item.iterdir():
                        if not subitem.is_dir():
                            continue
                        
                        # Check for ns-allinone-3.4x in subdirectory
                        if ns3_allinone_pattern.match(subitem.name):
                            for subsubitem in subitem.iterdir():
                                if subsubitem.is_dir() and ns3_version_pattern.match(subsubitem.name):
                                    if (subsubitem / "ns3").exists():
                                        found_installations.append(subsubitem)
                                        print(f"  Found: {subsubitem}")
                                        break
                        
                        # Check for direct ns-3.4x in subdirectory
                        elif ns3_version_pattern.match(subitem.name):
                            if (subitem / "ns3").exists():
                                found_installations.append(subitem)
                                print(f"  Found: {subitem}")
            
            except (PermissionError, OSError):
                continue
    
    except (PermissionError, OSError):
        print("  Warning: Limited access to home directory")
    
    # Also check common system locations
    system_paths = [
        Path("/usr/local/ns3"),
        Path("/usr/local"),
        Path("/opt"),
    ]
    
    for base in system_paths:
        if not base.exists():
            continue
        
        try:
            for item in base.iterdir():
                if not item.is_dir():
                    continue
                
                if ns3_allinone_pattern.match(item.name):
                    for subitem in item.iterdir():
                        if subitem.is_dir() and ns3_version_pattern.match(subitem.name):
                            if (subitem / "ns3").exists():
                                found_installations.append(subitem)
                                print(f"  Found: {subitem}")
                                break
                
                elif ns3_version_pattern.match(item.name):
                    if (item / "ns3").exists():
                        found_installations.append(item)
                        print(f"  Found: {item}")
        
        except (PermissionError, OSError):
            continue
    
    # Return the first found installation (prefer newer versions)
    if found_installations:
        # Remove duplicates and sort by version number (newest first)
        unique_installations = list(set(found_installations))
        unique_installations.sort(key=lambda p: p.name, reverse=True)
        detected = unique_installations[0]
        print(f"\n✓ Auto-detected ns-3 version: {detected.name}")
        print(f"  Location: {detected}")
        return detected
    
    # If not found, ask user
    print("\nERROR: Could not auto-detect ns-3 installation (versions 3.40-3.49).")
    print(f"Searched in: {home_dir} and system directories")
    print("\nPlease enter the full path to your ns-3.4x directory")
    print("(e.g., /home/username/ns3/ns-allinone-3.42/ns-3.42):")
    user_path = input("> ").strip()
    
    ns3_path = Path(user_path)
    if ns3_path.exists() and (ns3_path / "ns3").exists():
        print(f"✓ Using: {ns3_path}")
        return ns3_path
    else:
        print(f"ERROR: Invalid ns-3 path: {user_path}")
        sys.exit(1)

# Configuration - Dynamic path detection
SCRIPT_DIR = Path(__file__).resolve().parent
RESEARCH_DIR = SCRIPT_DIR.parent
NS3_PATH = find_ns3_installation()
SCRATCH_PATH = NS3_PATH / "scratch"
RESULTS_DIR = RESEARCH_DIR / "results"
SRC_DIR = RESEARCH_DIR / "src"
PROGRAM_NAME = "uwsn-research"

print(f"Using paths:")
print(f"  Research Dir: {RESEARCH_DIR}")
print(f"  NS-3 Path: {NS3_PATH}")
print(f"  Source Dir: {SRC_DIR}")
print(f"  Results Dir: {RESULTS_DIR}\n")

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
    Parse simulation stdout to extract metrics.
    
    Expected format:
        Sender Sent: 123
        Sink Received: 100
        PDR: 81.3%
    """
    metrics = {
        'sent_pkts': 0,
        'recv_pkts': 0,
        'pdr': 0.0
    }
    
    # Parse sender sent
    sent_match = re.search(r'Sender Sent:\s*(\d+)', output)
    if sent_match:
        metrics['sent_pkts'] = int(sent_match.group(1))
    
    # Parse sink received
    recv_match = re.search(r'Sink Received:\s*(\d+)', output)
    if recv_match:
        metrics['recv_pkts'] = int(recv_match.group(1))
    
    # Parse PDR directly
    pdr_match = re.search(r'PDR:\s*([\d.]+)%', output)
    if pdr_match:
        metrics['pdr'] = float(pdr_match.group(1)) / 100.0
    elif metrics['sent_pkts'] > 0:
        metrics['pdr'] = metrics['recv_pkts'] / metrics['sent_pkts']
    
    return metrics


def run_single_experiment(params):
    """Run a single experiment with given parameters."""
    protocol, nodes, run_number, sim_time, data_rate, packet_size, range_m, use_cbr = params
    
    cbr_flag = "--cbr=true" if use_cbr else "--cbr=false"
    
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
        f"{cbr_flag} "
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
        
        # Combine stdout and stderr (ns3 may output to either)
        combined_output = result.stdout + "\n" + result.stderr
        
        if result.returncode == 0:
            # Parse the output to get PDR and other metrics
            metrics = parse_simulation_output(combined_output)
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
                'stdout': combined_output,
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


def run_experiments(experiments, csv_file, debug=False):
    """Run all experiments and save results to CSV."""
    total = len(experiments)
    completed = 0
    failed = []
    results = []
    first_run = True
    
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
            
            # Debug: print first experiment output
            if debug and first_run and result['success']:
                first_run = False
                print("\n" + "=" * 70)
                print("DEBUG: First experiment output (for parsing verification)")
                print("=" * 70)
                # Show relevant parts of output
                output = result.get('stdout', '')
                for line in output.split('\n'):
                    if any(x in line for x in ['Sent Pkts', 'SendUp Pkts', 'Recv Pkts', 
                                                'NetworkTotal', 'Channel Counters',
                                                'Network Lifetime', 'Dead Nodes',
                                                'Total Initial Energy', 'Total Consumed']):
                        print(f"  {line}")
                print(f"\nParsed: sent={result['sent_pkts']}, recv={result['recv_pkts']}, pdr={result['pdr']:.4f}")
                print("=" * 70 + "\n")
            
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
        "--cbr", action="store_true",
        help="Use CBR (constant bit rate) traffic instead of OnOff"
    )
    parser.add_argument(
        "--skip-build", action="store_true",
        help="Skip copying and building (use existing build)"
    )
    parser.add_argument(
        "--output", type=str, default=None,
        help="Output CSV file path"
    )
    parser.add_argument(
        "--debug", action="store_true",
        help="Print first experiment output for debugging"
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
    print(f"Traffic mode: {'CBR' if args.cbr else 'OnOff'}")
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
                    args.data_rate, args.packet_size, args.range, args.cbr
                ))
    
    # Confirm
    response = input("\nProceed? [Y/n]: ").strip().lower()
    if response and response != 'y':
        print("Aborted.")
        sys.exit(0)
    
    # Run experiments
    start_time = time.time()
    failed = run_experiments(experiments, csv_file, debug=args.debug)
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

#!/usr/bin/env python3
"""
UWSN Protocol Comparison V2 - Complete Workflow
================================================
Runs experiments, analyzes results, and generates plots - ALL IN ONE

Usage:
    python3 run_experiments_v2_all.py --protocols VBF DBR --nodes 100 --runs 15
"""

import subprocess
import os
import sys
import argparse
import time
import re
import csv
import shutil
from pathlib import Path
from datetime import datetime

# Import analysis functions
import pandas as pd
import matplotlib.pyplot as plt

# Auto-detect paths (same as run_experiments_v2.py)
def find_ns3_installation():
    """Find ns-3 installation (version 3.40-3.49) by scanning user home directory."""
    script_dir = Path(__file__).resolve().parent
    research_dir = script_dir.parent
    
    ns3_version_pattern = re.compile(r'ns-3\.4\d$')
    ns3_allinone_pattern = re.compile(r'ns-allinone-3\.4\d$')
    
    found_installations = []
    
    print("Searching for ns-3.4x installation in home directory...")
    
    home_dir = Path.home()
    try:
        for item in home_dir.iterdir():
            if not item.is_dir() or item.name.startswith('.'):
                continue
            
            try:
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
                
                else:
                    for subitem in item.iterdir():
                        if not subitem.is_dir():
                            continue
                        
                        if ns3_allinone_pattern.match(subitem.name):
                            for subsubitem in subitem.iterdir():
                                if subsubitem.is_dir() and ns3_version_pattern.match(subsubitem.name):
                                    if (subsubitem / "ns3").exists():
                                        found_installations.append(subsubitem)
                                        print(f"  Found: {subsubitem}")
                                        break
                        
                        elif ns3_version_pattern.match(subitem.name):
                            if (subitem / "ns3").exists():
                                found_installations.append(subitem)
                                print(f"  Found: {subitem}")
            
            except (PermissionError, OSError):
                continue
    
    except (PermissionError, OSError):
        print("  Warning: Limited access to home directory")
    
    system_paths = [Path("/usr/local/ns3"), Path("/usr/local"), Path("/opt")]
    
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
    
    if found_installations:
        unique_installations = list(set(found_installations))
        unique_installations.sort(key=lambda p: p.name, reverse=True)
        detected = unique_installations[0]
        print(f"\n✓ Auto-detected ns-3 version: {detected.name}")
        print(f"  Location: {detected}")
        return detected
    
    print("\nERROR: Could not auto-detect ns-3 installation (versions 3.40-3.49).")
    print(f"Searched in: {home_dir} and system directories")
    sys.exit(1)

# Configuration
SCRIPT_DIR = Path(__file__).resolve().parent
RESEARCH_DIR = SCRIPT_DIR.parent
NS3_PATH = find_ns3_installation()
SCRATCH_PATH = NS3_PATH / "scratch"
RESULTS_DIR = RESEARCH_DIR / "results"
GRAPHS_DIR = SCRIPT_DIR / "graphs"
SRC_DIR = RESEARCH_DIR / "src" / "uswn-research.cc"
PROGRAM_NAME = "uwsn-research_v2"

print(f"Using paths:")
print(f"  Research Dir: {RESEARCH_DIR}")
print(f"  NS-3 Path: {NS3_PATH}")
print(f"  Source Dir: {SRC_DIR}")
print(f"  Results Dir: {RESULTS_DIR}")
print(f"  Graphs Dir: {GRAPHS_DIR}\n")

# Experiment parameters
DEFAULT_RUNS = 30
DEFAULT_SIM_TIME = 1000
DEFAULT_RANGE = 100
DEFAULT_MIN_DEPTH = 0.0
DEFAULT_MAX_DEPTH = 400.0
DEFAULT_VBF_WIDTH = 100.0
DEFAULT_MOBILITY = "random"
DEFAULT_AREA_X = 400.0
DEFAULT_AREA_Y = 400.0
DEFAULT_MIN_Z = 0.0
DEFAULT_MAX_Z = 400.0
DEFAULT_INITIAL_ENERGY = 500.0



def setup_simulation():
    """Copy simulation file to scratch folder and build."""
    source_file = SRC_DIR / f"{PROGRAM_NAME}.cc"
    dest_file = SCRATCH_PATH / f"{PROGRAM_NAME}.cc"
    
    if not source_file.exists():
        print(f"ERROR: Source file not found: {source_file}")
        sys.exit(1)
    
    print(f"Copying {source_file} to {dest_file}...")
    shutil.copy2(source_file, dest_file)
    
    print("Building ns-3...")
    process = subprocess.Popen(
        ["./ns3", "build"],
        cwd=NS3_PATH,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True
    )
    
    for line in process.stdout:
        print(line, end='', flush=True)
        
    process.wait()
    
    if process.returncode != 0:
        print(f"ERROR: Build failed!")
        sys.exit(1)
    
    print("✓ Build successful!\n")


def run_single_experiment(params):
    """Run a single simulation experiment."""
    protocol, nodes, run_number, sim_time, comm_range, min_z, max_z, vbf_width, mobility, area_x, area_y, initial_energy, csv_file = params
    
    print(f"Running: {protocol} | Nodes: {nodes} | Run: {run_number} | Range: {comm_range}m | Area: {area_x}x{area_y}x[{min_z},{max_z}]")
    


    cmd = [
        "./ns3", "run",
        f"{PROGRAM_NAME} "
        f"--protocol={protocol} "
        f"--nodes={nodes} "
        f"--runNumber={run_number} "
        f"--simStop={sim_time} "
        f"--range={comm_range} "
        f"--minDepth={min_z} "
        f"--maxDepth={max_z} "
        f"--width={vbf_width} "
        f"--mobility={mobility} "
        f"--areaX={area_x} "
        f"--areaY={area_y} "
        f"--initialEnergy={initial_energy} "
        f"--csvFile={csv_file} "
    ]
    
    # NEW: Trace for Lifetime (Phase 1)
    trace_dir = SCRIPT_DIR / "traces"
    trace_dir.mkdir(exist_ok=True)
    trace_filename = f"trace_{protocol}_{nodes}_{run_number}_{int(time.time())}.csv"
    trace_file_path = trace_dir / trace_filename
    
    # Append to the existing argument string (last element of cmd list)
    cmd[-1] += f"--lifetimeTraceFile={trace_file_path}"



    try:
        process = subprocess.Popen(
            cmd,
            cwd=NS3_PATH,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )
        
        full_output = []
        for line in process.stdout:
            print(line, end='', flush=True)
            full_output.append(line)
            
        process.wait()
        
        result_stdout = "".join(full_output)

        if process.returncode == 0:
            # Parse PDR from output
            pdr_match = re.search(r'PDR:\s+([\d.]+)%', result_stdout)

            sent_match = re.search(r'Sender Sent:\s+(\d+)', result_stdout)
            recv_match = re.search(r'Sink Received:\s+(\d+)', result_stdout)
            
            pdr_str = f"PDR: {pdr_match.group(1)}%" if pdr_match else "PDR: N/A"
            if sent_match and recv_match:
                pdr_str += f" (Sent: {sent_match.group(1)}, Recv: {recv_match.group(1)})"
            
            print(f"  ✓ {pdr_str}")
            return (True, str(trace_file_path.resolve())) # Return path
        else:
            print(f"  ✗ Failed: {protocol} | Nodes: {nodes} | Run: {run_number}")
            print(f"     Error: {result.stderr[:200]}")
            return (False, None)
            
    except subprocess.TimeoutExpired:
        print(f"  ✗ Timeout: {protocol} | Nodes: {nodes} | Run: {run_number}")
        return (False, None)
    except Exception as e:
        print(f"  ✗ Exception: {e}")
        return (False, None)




def run_experiments(args, csv_file):
    """Run all experiments."""
    print(f"\n{'='*70}")
    print(f"STEP 1: RUNNING EXPERIMENTS")
    print(f"{'='*70}\n")
    
    if not args.skip_build:
        setup_simulation()
    

    experiments = []
    for protocol in args.protocols:
        for node_count in args.nodes:
            for comm_range in args.range:
                for min_z in args.min_z:
                    for max_z in args.max_z:
                        for vbf_width in args.vbf_width:
                            for mobility in args.mobility:
                                for area_x in args.area_x:
                                    for area_y in args.area_y:
                                        for sim_time in args.simtime:
                                            for init_energy in args.initial_energy:
                                                for run in range(1, args.runs + 1):
                                                    experiments.append((
                                                        protocol,
                                                        node_count,
                                                        run,
                                                        sim_time,
                                                        comm_range,
                                                        min_z,
                                                        max_z,
                                                        vbf_width,
                                                        mobility,
                                                        area_x,
                                                        area_y,
                                                        init_energy,
                                                        csv_file
                                                    ))
    
    total_experiments = len(experiments)
    print(f"Total experiments: {total_experiments}")
    print(f"Protocols: {args.protocols}")
    print(f"Node counts: {args.nodes}")
    print(f"Ranges: {args.range}")
    print(f"Area: X={args.area_x}, Y={args.area_y}, Z=[{args.min_z}, {args.max_z}]")
    print(f"VBF Widths: {args.vbf_width}")
    print(f"Mobility: {args.mobility}")
    print(f"Runs per config: {args.runs}\n")

    
    start_time = time.time()
    completed = 0
    failed = 0
    

    all_traces = []

    # ANSI color codes for separators
    CYAN = '\033[96m'
    MAGENTA = '\033[95m'
    RESET = '\033[0m'

    for i, params in enumerate(experiments, 1):
        # Alternating color separator
        color = CYAN if i % 2 == 1 else MAGENTA
        print(f"\n{color}{'─' * 70}{RESET}")
        print(f"[{i}/{total_experiments}] ", end="")
        success, result_data = run_single_experiment(params)

        if success:
            completed += 1
            if result_data: # It's a path
               all_traces.append((params[0], params[1], result_data)) # protocol, nodes, path
        else:
            failed += 1


    
    elapsed_time = time.time() - start_time
    
    print(f"\n{'='*70}")
    print(f"Experiments Complete!")
    print(f"{'='*70}")
    print(f"Completed: {completed}/{total_experiments}")
    print(f"Failed: {failed}")
    print(f"Time: {elapsed_time:.2f} seconds\n")
    
    return all_traces




def analyze_results(csv_file):
    """Analyze simulation results."""
    print(f"\n{'='*70}")
    print(f"STEP 2: ANALYZING RESULTS")
    print(f"{'='*70}\n")
    
    try:
        df = pd.read_csv(csv_file)
        print(f"Loaded {len(df)} rows from {csv_file}\n")
    except Exception as e:
        print(f"ERROR: Could not load {csv_file}: {e}")
        return None
    
    # Analysis by protocol
    print("=" * 70)
    print("   ANALYSIS BY PROTOCOL")
    print("=" * 70)
    
    for protocol in sorted(df['protocol'].unique()):
        data = df[df['protocol'] == protocol]
        n_runs = len(data)
        
        print(f"\n{protocol} (n={n_runs} runs)")
        print("-" * 40)
        
        # Calculate statistics
        if 'pdr' in data.columns:
            pdr_mean = data['pdr'].mean() * 100  # Convert to percentage
            pdr_std = data['pdr'].std() * 100
            print(f"  PDR: {pdr_mean:.2f}% ± {pdr_std:.2f}%")
        
        if 'total_consumed_energy' in data.columns:
            energy_mean = data['total_consumed_energy'].mean()
            energy_std = data['total_consumed_energy'].std()
            print(f"  Total Energy: {energy_mean:.2f} ± {energy_std:.2f} J")
        
        if 'total_residual_energy' in data.columns:
            residual_mean = data['total_residual_energy'].mean()
            print(f"  Residual Energy: {residual_mean:.2f} J")
        
        if 'sender_sent' in data.columns and 'sink_recv' in data.columns:
            total_sent = data['sender_sent'].sum()
            total_recv = data['sink_recv'].sum()
            print(f"  Total Packets: Sent={total_sent}, Received={total_recv}")
    
    return df


def analyze_and_plot_lifetime(trace_files, output_dir):
    """
    Plots Alive Nodes vs Simulation Time.
    Shows full simulation timeline with visible drop at node death.
    """
    print(f"\n{'='*70}")
    print(f"STEP 2.5: PLOTTING LIFETIME (Phase 1)")
    print(f"{'='*70}\n")
    
    if not trace_files:
        print("No traces available.")
        return

    plt.figure(figsize=(10, 6))
    
    # Group by Protocol + Nodes
    # trace_files is list of (protocol, nodes, path)
    
    # We might have multiple runs for same config. We should aggregate or show all?
    # For Phase 1, showing all curves is fine, or one representative. 
    # Let's plot all lines but use same color/label for same config.
    
    config_colors = {}
    
    # Sort by protocol then nodes
    trace_files.sort(key=lambda x: (x[0], x[1]))
    
    plotted_labels = set()
    
    for protocol, nodes, path in trace_files:
        try:
            if not os.path.exists(path): continue
            df = pd.read_csv(path)
            if df.empty or 'Time' not in df.columns or 'AliveNodes' not in df.columns: continue
            
            label = f"{protocol} ({nodes} nodes)"
            
            # Simple plot - data is already truncated by C++ simulation
            if label not in plotted_labels:
                plt.plot(df['Time'], df['AliveNodes'], label=label, alpha=0.8)
                plotted_labels.add(label)
            else:
                plt.plot(df['Time'], df['AliveNodes'], alpha=0.3, linestyle='--') # Faint lines for other runs
                
        except Exception as e:
            print(f"Warning: Failed to plot {path}: {e}")

    plt.title("Network Lifetime: Alive Nodes vs Time (Full Simulation)", fontsize=13)
    plt.xlabel("Simulation Time (s)")
    plt.ylabel("Number of Alive Nodes")
    plt.legend(loc='best')
    plt.grid(True, linestyle='--', alpha=0.5)

    
    out_file = output_dir / "Network_Lifetime_Alive_Nodes.png"
    plt.savefig(out_file, dpi=300)
    plt.close()
    print(f"✓ Saved plot: {out_file}")





def plot_results(csv_file, output_dir):
    """Generate comprehensive visualization matrix using plot_comprehensive.py."""
    print(f"\n{'='*70}")
    print(f"STEP 3: GENERATING COMPREHENSIVE VISUALIZATION MATRIX")
    print(f"{'='*70}\n")
    
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Call the comprehensive plotting script
    plot_script = SCRIPT_DIR / "plot_comprehensive.py"
    
    if not plot_script.exists():
        print(f"ERROR: Comprehensive plotting script not found: {plot_script}")
        return False
    
    print(f"Running comprehensive visualization script...")
    print(f"  Input: {csv_file}")
    print(f"  Output: {output_dir}\n")
    
    result = subprocess.run(
        [sys.executable, str(plot_script), str(csv_file), "--output-dir", str(output_dir)],
        capture_output=True,
        text=True
    )
    
    if result.returncode == 0:
        print(result.stdout)
        print(f"\n✓ Comprehensive visualization matrix saved to: {output_dir}\n")
        return True
    else:
        print(f"ERROR: Comprehensive plotting failed!")
        print(result.stderr)
        return False


def main():
    parser = argparse.ArgumentParser(
        description="UWSN Protocol Comparison V2 - Complete Workflow (Experiments + Analysis + Plots)"
    )
    parser.add_argument(
        "--runs", type=int, default=DEFAULT_RUNS,
        help=f"Number of runs per configuration (default: {DEFAULT_RUNS})"
    )
    parser.add_argument(
        "--protocols", nargs='+', default=["VBF", "DBR", "flooding", "EDBR", "EVBF", "EDBRv2", "EVBFv2"],
        help="Protocols to test (default: VBF DBR flooding EDBR EVBF EDBRv2 EVBFv2)"
    )
    parser.add_argument(
        "--nodes", nargs='+', type=int, default=[100],
        help="Node counts to test (default: 100)"
    )
    parser.add_argument(
        "--simtime", nargs='+', type=int, default=[DEFAULT_SIM_TIME],
        help=f"Simulation time in seconds (default: {DEFAULT_SIM_TIME})"
    )

    parser.add_argument(
        "--range", nargs='+', type=float, default=[DEFAULT_RANGE],
        help=f"Communication range(s) in meters (default: {DEFAULT_RANGE})"
    )

    parser.add_argument(
        "--skip-build", action="store_true",
        help="Skip copying and building (use existing build)"
    )
    parser.add_argument(
        "--skip-analysis", action="store_true",
        help="Skip analysis step (only run experiments)"
    )
    parser.add_argument(
        "--skip-plots", action="store_true",
        help="Skip plotting step"
    )
    parser.add_argument(
        "--output", default=None,
        help="Output CSV file path (default: auto-generated)"
    )
    
    parser.add_argument(
        "--min-depth", nargs='+', type=float, default=[],
        help="DEPRECATED: Use --min-z instead. Minimum depth(s) in meters."
    )
    parser.add_argument(
        "--max-depth", nargs='+', type=float, default=[],
        help="DEPRECATED: Use --max-z instead. Maximum depth(s) in meters."
    )
    parser.add_argument(
        "--area-x", nargs='+', type=float, default=[DEFAULT_AREA_X],
        help=f"X dimension(s) of the area in meters (default: {DEFAULT_AREA_X})"
    )
    parser.add_argument(
        "--area-y", nargs='+', type=float, default=[DEFAULT_AREA_Y],
        help=f"Y dimension(s) of the area in meters (default: {DEFAULT_AREA_Y})"
    )
    parser.add_argument(
        "--min-z", nargs='+', type=float, default=[DEFAULT_MIN_Z],
        help=f"Minimum Z (depth) dimension(s) in meters (default: {DEFAULT_MIN_Z})"
    )
    parser.add_argument(
        "--max-z", nargs='+', type=float, default=[DEFAULT_MAX_Z],
        help=f"Maximum Z (depth) dimension(s) in meters (default: {DEFAULT_MAX_Z})"
    )
    
    parser.add_argument(
        "--vbf-width", nargs='+', type=float, default=[DEFAULT_VBF_WIDTH],
        help=f"VBF routing pipe radius/width(s) in meters (default: {DEFAULT_VBF_WIDTH})"
    )
    parser.add_argument(
        "--mobility", nargs='+', choices=["fixed", "random"], default=[DEFAULT_MOBILITY],
        help=f"Mobility model(s) (default: {DEFAULT_MOBILITY})"
    )
    parser.add_argument(
        "--initial-energy", nargs='+', type=float, default=[DEFAULT_INITIAL_ENERGY],
        help=f"Initial energy in Joules (default: {DEFAULT_INITIAL_ENERGY})"
    )


    args = parser.parse_args()

    # Handle Deprecation/Aliasing for Depth
    if args.min_depth:
        print("Note: --min-depth is deprecated, treating as --min-z")
        if not args.min_z or args.min_z == [DEFAULT_MIN_Z]: # Override if default
             args.min_z = args.min_depth
    if args.max_depth:
        print("Note: --max-depth is deprecated, assuming mapping to max-z")
        if not args.max_z or args.max_z == [DEFAULT_MAX_Z]:
             args.max_z = args.max_depth

    # Ensure min_z is a list (if it was single float default)
    # The default above is [DEFAULT_MIN_Z] so it is a list.
    # But DEFAULT_MIN_DEPTH was float.
    # Let's just ensure listiness in run_experiments.
    pass

    
    # Create directories
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)
    GRAPHS_DIR.mkdir(parents=True, exist_ok=True)
    
    # Generate output filename - one file per session with all runs
    if args.output:
        csv_file = Path(args.output)
    else:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        csv_file = RESULTS_DIR / f"results_{timestamp}.csv"
    
    csv_file = str(csv_file.resolve())
    
    print(f"\n{'='*70}")
    print(f"UWSN PROTOCOL COMPARISON V2 - COMPLETE WORKFLOW")
    print(f"{'='*70}")
    print(f"Results: {csv_file}")
    print(f"Graphs: {GRAPHS_DIR}")
    print(f"{'='*70}\n")
    
    # Step 1: Run experiments (all runs saved to csv_file)
    traces = run_experiments(args, csv_file)
    if not traces:
        print("Warning: No traces collected (or experiments failed).")
        pass

    # Step 2: Analyze results
    if not args.skip_analysis:
        df = analyze_results(csv_file)
        if df is None:
            print("ERROR: Could not analyze results!")
            # sys.exit(1) # Don't exit, try plotting if possible
    
    # Step 3: Generate comprehensive visualization matrix
    if not args.skip_plots:
        csv_basename = Path(csv_file).stem
        graphs_output_dir = GRAPHS_DIR / csv_basename
        graphs_output_dir.mkdir(parents=True, exist_ok=True)
        
        plot_results(csv_file, graphs_output_dir)
        
        # Plot Lifetime
        analyze_and_plot_lifetime(traces, graphs_output_dir)


    else:
        graphs_output_dir = GRAPHS_DIR
    
    # Final summary
    print(f"\n{'='*70}")
    print(f"✅ COMPLETE WORKFLOW FINISHED!")
    print(f"{'='*70}")
    print(f"Results: {csv_file}")
    print(f"Graphs: {graphs_output_dir}")
    print(f"{'='*70}\n")


if __name__ == "__main__":
    main()


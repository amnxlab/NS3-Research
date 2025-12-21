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

# Import analysis and plotting functions
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from scipy import stats

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
DEFAULT_MOBILITY = "static"

# Color scheme for protocols
PROTOCOL_COLORS = {
    'VBF': '#2ecc71',
    'DBR': '#3498db',
    'flooding': '#e74c3c'
}

PROTOCOL_MARKERS = {
    'VBF': 'o',
    'DBR': 's',
    'flooding': '^'
}


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
    result = subprocess.run(
        ["./ns3", "build"],
        cwd=NS3_PATH,
        capture_output=True,
        text=True
    )
    
    if result.returncode != 0:
        print(f"ERROR: Build failed!")
        print(result.stdout)
        print(result.stderr)
        sys.exit(1)
    
    print("✓ Build successful!\n")


def run_single_experiment(params):
    """Run a single simulation experiment."""
    protocol, nodes, run_number, sim_time, comm_range, min_depth, max_depth, vbf_width, mobility, csv_file = params
    
    print(f"Running: {protocol} | Nodes: {nodes} | Run: {run_number} | Range: {comm_range}m | Mobility: {mobility}")
    
    cmd = [
        "./ns3", "run",
        f"{PROGRAM_NAME} "
        f"--protocol={protocol} "
        f"--nodes={nodes} "
        f"--runNumber={run_number} "
        f"--simStop={sim_time} "
        f"--range={comm_range} "
        f"--minDepth={min_depth} "
        f"--maxDepth={max_depth} "
        f"--width={vbf_width} "
        f"--mobility={mobility} "
        f"--csvFile={csv_file}"
    ]
    
    try:
        result = subprocess.run(
            cmd,
            cwd=NS3_PATH,
            capture_output=True,
            text=True,
            timeout=sim_time + 60
        )
        
        if result.returncode == 0:
            print(f"  ✓ Completed: {protocol} | Nodes: {nodes} | Run: {run_number}")
            return True
        else:
            print(f"  ✗ Failed: {protocol} | Nodes: {nodes} | Run: {run_number}")
            return False
            
    except subprocess.TimeoutExpired:
        print(f"  ✗ Timeout: {protocol} | Nodes: {nodes} | Run: {run_number}")
        return False
    except Exception as e:
        print(f"  ✗ Exception: {e}")
        return False


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
            for run in range(1, args.runs + 1):
                experiments.append((
                    protocol,
                    node_count,
                    run,
                    args.simtime,
                    args.range,
                    args.min_depth,
                    args.max_depth,
                    args.vbf_width,
                    args.mobility,
                    csv_file
                ))
    
    total_experiments = len(experiments)
    print(f"Total experiments: {total_experiments}")
    print(f"Protocols: {args.protocols}")
    print(f"Node counts: {args.nodes}")
    print(f"Runs per config: {args.runs}\n")
    
    start_time = time.time()
    completed = 0
    failed = 0
    
    for i, params in enumerate(experiments, 1):
        print(f"[{i}/{total_experiments}] ", end="")
        if run_single_experiment(params):
            completed += 1
        else:
            failed += 1
    
    elapsed_time = time.time() - start_time
    
    print(f"\n{'='*70}")
    print(f"Experiments Complete!")
    print(f"{'='*70}")
    print(f"Completed: {completed}/{total_experiments}")
    print(f"Failed: {failed}")
    print(f"Time: {elapsed_time:.2f} seconds\n")
    
    return completed > 0


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


def plot_results(df, output_dir):
    """Generate all plots."""
    print(f"\n{'='*70}")
    print(f"STEP 3: GENERATING PLOTS")
    print(f"{'='*70}\n")
    
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    plt.style.use('seaborn-v0_8-whitegrid')
    sns.set_palette("husl")
    
    # Plot 1: PDR Comparison
    print("Generating PDR comparison...")
    fig, ax = plt.subplots(figsize=(10, 6))
    
    protocols = sorted(df['protocol'].unique())
    pdr_data = df.groupby('protocol')['pdr'].agg(['mean', 'std']).reset_index()
    pdr_data['mean'] *= 100  # Convert to percentage
    pdr_data['std'] *= 100
    
    x_pos = np.arange(len(protocols))
    bars = ax.bar(x_pos, pdr_data['mean'],
                  yerr=pdr_data['std'],
                  color=[PROTOCOL_COLORS.get(p, 'gray') for p in protocols],
                  capsize=5, alpha=0.8, edgecolor='black', linewidth=1.2)
    
    ax.set_xlabel('Protocol', fontsize=12, fontweight='bold')
    ax.set_ylabel('Packet Delivery Ratio (%)', fontsize=12, fontweight='bold')
    ax.set_title('PDR Comparison', fontsize=14, fontweight='bold')
    ax.set_xticks(x_pos)
    ax.set_xticklabels(protocols, fontsize=11)
    ax.set_ylim(0, 100)
    ax.grid(axis='y', alpha=0.3)
    
    for bar in bars:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.1f}%', ha='center', va='bottom', fontweight='bold')
    
    plt.tight_layout()
    output_file = output_dir / 'v2_pdr_comparison.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"  ✓ Saved: {output_file}")
    
    # Plot 2: Energy Comparison
    print("Generating energy comparison...")
    fig, ax = plt.subplots(figsize=(10, 6))
    
    protocols = sorted(df['protocol'].unique())
    energy_data = df.groupby('protocol')['total_consumed_energy'].agg(['mean', 'std']).reset_index()
    
    x_pos = np.arange(len(protocols))
    bars = ax.bar(x_pos, energy_data['mean'],
                  yerr=energy_data['std'],
                  color=[PROTOCOL_COLORS.get(p, 'gray') for p in protocols],
                  capsize=5, alpha=0.8, edgecolor='black', linewidth=1.2)
    
    ax.set_xlabel('Protocol', fontsize=12, fontweight='bold')
    ax.set_ylabel('Total Energy Consumed (J)', fontsize=12, fontweight='bold')
    ax.set_title('Energy Consumption Comparison', fontsize=14, fontweight='bold')
    ax.set_xticks(x_pos)
    ax.set_xticklabels(protocols, fontsize=11)
    ax.grid(axis='y', alpha=0.3)
    
    for bar in bars:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.1f}J', ha='center', va='bottom', fontweight='bold')
    
    plt.tight_layout()
    output_file = output_dir / 'v2_energy_comparison.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"  ✓ Saved: {output_file}")
    
    # Plot 2: Energy Distribution (Box Plot)
    print("Generating energy distribution...")
    fig, ax = plt.subplots(figsize=(10, 6))
    
    sns.boxplot(data=df, x='protocol', y='total_consumed_energy', ax=ax,
                palette=PROTOCOL_COLORS, linewidth=1.5)
    
    ax.set_xlabel('Protocol', fontsize=12, fontweight='bold')
    ax.set_ylabel('Total Energy Consumed (J)', fontsize=12, fontweight='bold')
    ax.set_title('Energy Consumption Distribution', fontsize=14, fontweight='bold')
    ax.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    output_file = output_dir / 'v2_energy_distribution.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"  ✓ Saved: {output_file}")
    
    # Plot 3: Summary Dashboard
    if len(df['nodes'].unique()) > 1:
        print("Generating scalability analysis...")
        fig, axes = plt.subplots(1, 2, figsize=(14, 5))
        
        # Energy vs Nodes
        ax = axes[0]
        for protocol in sorted(df['protocol'].unique()):
            data = df[df['protocol'] == protocol].groupby('nodes')['total_consumed_energy'].agg(['mean', 'std']).reset_index()
            ax.errorbar(data['nodes'], data['mean'], yerr=data['std'],
                       label=protocol, marker=PROTOCOL_MARKERS.get(protocol, 'o'),
                       color=PROTOCOL_COLORS.get(protocol), capsize=4, linewidth=2.5,
                       markersize=10, markeredgewidth=1.5, markeredgecolor='white')
        
        ax.set_xlabel('Number of Nodes', fontsize=12, fontweight='bold')
        ax.set_ylabel('Total Energy (J)', fontsize=12, fontweight='bold')
        ax.set_title('Energy Scalability', fontsize=13, fontweight='bold')
        ax.legend(title='Protocol', fontsize=11, title_fontsize=11)
        ax.grid(True, alpha=0.3, linestyle='--')
        
        # Residual Energy
        ax = axes[1]
        for protocol in sorted(df['protocol'].unique()):
            data = df[df['protocol'] == protocol].groupby('nodes')['total_residual_energy'].agg(['mean', 'std']).reset_index()
            ax.errorbar(data['nodes'], data['mean'], yerr=data['std'],
                       label=protocol, marker=PROTOCOL_MARKERS.get(protocol, 'o'),
                       color=PROTOCOL_COLORS.get(protocol), capsize=4, linewidth=2.5,
                       markersize=10, markeredgewidth=1.5, markeredgecolor='white')
        
        ax.set_xlabel('Number of Nodes', fontsize=12, fontweight='bold')
        ax.set_ylabel('Residual Energy (J)', fontsize=12, fontweight='bold')
        ax.set_title('Remaining Energy', fontsize=13, fontweight='bold')
        ax.legend(title='Protocol', fontsize=11, title_fontsize=11)
        ax.grid(True, alpha=0.3, linestyle='--')
        
        plt.suptitle('UWSN Protocol Comparison - Scalability Analysis', 
                     fontsize=16, fontweight='bold', y=1.02)
        plt.tight_layout()
        
        output_file = output_dir / 'v2_scalability_analysis.png'
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        plt.close()
        print(f"  ✓ Saved: {output_file}")
    
    print(f"\n✓ All plots saved to: {output_dir}\n")


def main():
    parser = argparse.ArgumentParser(
        description="UWSN Protocol Comparison V2 - Complete Workflow (Experiments + Analysis + Plots)"
    )
    parser.add_argument(
        "--runs", type=int, default=DEFAULT_RUNS,
        help=f"Number of runs per configuration (default: {DEFAULT_RUNS})"
    )
    parser.add_argument(
        "--protocols", nargs='+', default=["VBF", "DBR", "flooding"],
        help="Protocols to test (default: VBF DBR flooding)"
    )
    parser.add_argument(
        "--nodes", nargs='+', type=int, default=[100],
        help="Node counts to test (default: 100)"
    )
    parser.add_argument(
        "--simtime", type=int, default=DEFAULT_SIM_TIME,
        help=f"Simulation time in seconds (default: {DEFAULT_SIM_TIME})"
    )
    parser.add_argument(
        "--range", type=float, default=DEFAULT_RANGE,
        help=f"Communication range in meters (default: {DEFAULT_RANGE})"
    )
    parser.add_argument(
        "--min-depth", type=float, default=DEFAULT_MIN_DEPTH,
        help=f"Minimum depth in meters (default: {DEFAULT_MIN_DEPTH})"
    )
    parser.add_argument(
        "--max-depth", type=float, default=DEFAULT_MAX_DEPTH,
        help=f"Maximum depth in meters (default: {DEFAULT_MAX_DEPTH})"
    )
    parser.add_argument(
        "--vbf-width", type=float, default=DEFAULT_VBF_WIDTH,
        help=f"VBF routing pipe radius/width in meters (default: {DEFAULT_VBF_WIDTH})"
    )
    parser.add_argument(
        "--mobility", choices=["static", "random"], default=DEFAULT_MOBILITY,
        help=f"Mobility model (default: {DEFAULT_MOBILITY})"
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
    
    args = parser.parse_args()
    
    # Create directories
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)
    GRAPHS_DIR.mkdir(parents=True, exist_ok=True)
    
    # Generate output filename
    if args.output:
        csv_file = Path(args.output)
    else:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        csv_file = RESULTS_DIR / f"uwsn_v2_results_{timestamp}.csv"
    
    csv_file = str(csv_file.resolve())
    
    print(f"\n{'='*70}")
    print(f"UWSN PROTOCOL COMPARISON V2 - COMPLETE WORKFLOW")
    print(f"{'='*70}")
    print(f"Output: {csv_file}")
    print(f"Graphs: {GRAPHS_DIR}")
    print(f"{'='*70}\n")
    
    # Step 1: Run experiments
    success = run_experiments(args, csv_file)
    if not success:
        print("ERROR: No experiments completed successfully!")
        sys.exit(1)
    
    # Step 2: Analyze results
    if not args.skip_analysis:
        df = analyze_results(csv_file)
        if df is None:
            print("ERROR: Could not analyze results!")
            sys.exit(1)
    else:
        df = pd.read_csv(csv_file)
    
    # Step 3: Generate plots
    if not args.skip_plots:
        plot_results(df, GRAPHS_DIR)
    
    # Final summary
    print(f"\n{'='*70}")
    print(f"✅ COMPLETE WORKFLOW FINISHED!")
    print(f"{'='*70}")
    print(f"Results: {csv_file}")
    print(f"Graphs: {GRAPHS_DIR}")
    print(f"{'='*70}\n")


if __name__ == "__main__":
    main()


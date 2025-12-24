#!/usr/bin/env python3
"""
UWSN V2 - Comprehensive Plotting System
========================================
Y-axis (vertical): Parameters (nodes, depth, width, mobility, range)
X-axis (horizontal): Metrics (residual energy, energy eff %, PDR, delay, lifetime)

Each plot: Y-axis parameter values vs X-axis metric values
Grouped bars: All protocols compared for each Y-value

Usage:
    python3 plot_comprehensive.py results.csv --output-dir ./plots/
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import argparse
import sys
from pathlib import Path
from matplotlib.patches import Patch

# Color-blind friendly colors
PROTOCOL_COLORS = {
    'VBF': '#0072B2',       # Blue
    'DBR': '#D55E00',       # Vermillion/Orange
    'flooding': '#009E73'   # Bluish Green
}

PROTOCOL_HATCHES = {
    'VBF': '',
    'DBR': '///',
    'flooding': 'xxx'
}

plt.rcParams.update({
    'font.family': 'serif',
    'font.size': 11,
    'axes.labelsize': 12,
    'axes.titlesize': 14,
    'xtick.labelsize': 10,
    'ytick.labelsize': 10,
    'legend.fontsize': 10,
    'figure.dpi': 150,
    'savefig.dpi': 300,
    'savefig.bbox': 'tight',
})

# Y-AXIS (vertical): Parameters we vary
Y_AXIS_PARAMETERS = [
    ('nodes', 'Number of Nodes', None),
    ('depth', 'Depth Variation', None),
    ('vbfWidth', 'VBF Width (m)', 'VBF'),  # VBF only
    ('mobility', 'Mobility Model', None),
    ('range', 'Communication Range (m)', None)
]

# X-AXIS (horizontal): Metrics we measure
X_AXIS_METRICS = [
    ('total_residual_energy', 'Residual Energy (J)'),
    ('energy_efficiency_pct', 'Energy Efficiency (%)'),
    ('pdr_pct', 'PDR (%)'),
    ('avg_delay_ms', 'Average Delay (ms)'),
    ('network_lifetime_s', 'Network Lifetime (s)')
]


def load_and_prepare_data(csv_file):
    """Load CSV and compute ALL metrics including flooding fixes."""
    try:
        df = pd.read_csv(csv_file)
        print(f"Loaded {len(df)} rows from {csv_file}")
        print(f"Protocols: {list(df['protocol'].unique())}")
        
        # === FIX: PDR as percentage ===
        if 'pdr' in df.columns:
            df['pdr_pct'] = df['pdr'] * 100
        else:
            df['pdr_pct'] = 0
        
        # === FIX: Energy Efficiency for ALL protocols including flooding ===
        # Energy Efficiency = (Packets Received / Total Energy Consumed) * 100
        if 'sink_recv' in df.columns and 'total_consumed_energy' in df.columns:
            df['energy_efficiency_pct'] = np.where(
                df['total_consumed_energy'] > 0,
                (df['sink_recv'] / df['total_consumed_energy']) * 100,
                0
            )
        else:
            df['energy_efficiency_pct'] = 0
        
        # === FIX: ODR (Offered Data Rate) for flooding ===
        # ODR = (Packets Sent / Simulation Time) * Packet Size
        if 'sender_sent' in df.columns and 'simStop' in df.columns:
            # Assuming packet size ~40 bytes (from simulation)
            packet_size_bytes = 40
            df['odr'] = (df['sender_sent'] / df['simStop']) * packet_size_bytes * 8  # bits per second
        else:
            df['odr'] = 0
        
        # Ensure other metrics exist
        if 'avg_delay_ms' not in df.columns:
            df['avg_delay_ms'] = 0
        if 'network_lifetime_s' not in df.columns:
            df['network_lifetime_s'] = 0
        if 'total_residual_energy' not in df.columns:
            df['total_residual_energy'] = 0
        
        # Create depth label
        if 'minDepth' in df.columns and 'maxDepth' in df.columns:
            df['depth'] = df.apply(lambda r: f"{r['minDepth']:.0f}-{r['maxDepth']:.0f}m", axis=1)
        else:
            df['depth'] = 'default'
        
        # Print summary
        print("\nData Summary per Protocol:")
        for protocol in sorted(df['protocol'].unique()):
            pdata = df[df['protocol'] == protocol]
            print(f"  {protocol}:")
            print(f"    PDR: {pdata['pdr_pct'].mean():.2f}%")
            print(f"    Energy Efficiency: {pdata['energy_efficiency_pct'].mean():.4f}%")
            print(f"    ODR: {pdata['odr'].mean():.2f} bps")
            print(f"    Residual Energy: {pdata['total_residual_energy'].mean():.2f} J")
        
        return df
        
    except Exception as e:
        print(f"ERROR loading {csv_file}: {e}")
        sys.exit(1)


def create_plot(df, y_col, y_label, x_col, x_label, output_file, protocol_filter=None, csv_basename=""):
    """
    Create ONE plot: Y-axis parameter vs X-axis metric.
    
    Y-axis (vertical): Parameter values (e.g., nodes = 100, 150, 200) - ALL VALUES PLOTTED
    X-axis (horizontal): Metric values (sorted in ascending order)
    Bars: Protocols grouped side-by-side for each Y-value
    """
    # Get ALL Y-axis parameter values (e.g., nodes = 100, 150, 200)
    if y_col == 'depth':
        y_values = sorted(df['depth'].unique())
    elif y_col in df.columns:
        y_values = sorted(df[y_col].unique())
    else:
        print(f"  ⚠ Column '{y_col}' not found")
        return False
    
    if not y_values:
        print(f"  ⚠ No values for {y_col}")
        return False
    
    # Get protocols
    if protocol_filter:
        protocols = [protocol_filter] if protocol_filter in df['protocol'].unique() else []
        if not protocols:
            return False
    else:
        protocols = sorted(df['protocol'].unique())
    
    # For each Y-value, collect X-axis metric values
    # Group by Y-value and protocol, calculate means
    plot_data = {}
    for y_val in y_values:
        plot_data[y_val] = {}
        for protocol in protocols:
            if y_col == 'depth':
                mask = (df['depth'] == y_val) & (df['protocol'] == protocol)
            else:
                mask = (df[y_col] == y_val) & (df['protocol'] == protocol)
            subset = df[mask]
            
            if len(subset) > 0:
                mean_val = subset[x_col].mean()
                std_val = subset[x_col].std() if len(subset) > 1 else 0
                plot_data[y_val][protocol] = (mean_val, std_val)
            else:
                plot_data[y_val][protocol] = (0, 0)
    
    # Collect all X-axis values and sort them
    all_x_values = []
    for y_val in y_values:
        for protocol in protocols:
            if protocol in plot_data[y_val]:
                val, _ = plot_data[y_val][protocol]
                if val > 0:
                    all_x_values.append(val)
    
    if not all_x_values:
        print(f"  ⚠ No data for {y_label} vs {x_label}")
        return False
    
    # Sort X-axis values in ascending order
    sorted_x_positions = sorted(set(all_x_values))
    n_x_positions = len(sorted_x_positions)
    
    # Create figure - STANDARD VERTICAL bars
    # X-axis: Parameter values (sorted) - ALL VALUES PLOTTED
    # Y-axis: Metric values
    fig, ax = plt.subplots(figsize=(12, 8))
    
    # X-axis positions for parameter values (sorted)
    x_positions = np.arange(len(y_values))
    
    # For each parameter value, plot bars for each protocol
    n_protocols = len(protocols)
    bar_width = 0.8 / n_protocols
    
    # Plot vertical bars: for each Y-value, for each protocol
    for y_idx, y_val in enumerate(y_values):
        for p_idx, protocol in enumerate(protocols):
            if protocol not in plot_data[y_val]:
                continue
            
            mean_val, std_val = plot_data[y_val][protocol]
            
            # Calculate bar offset
            offset = (p_idx - (n_protocols - 1) / 2) * bar_width
            
            # Plot vertical bar
            bars = ax.bar(x_positions[y_idx] + offset, mean_val, bar_width,
                         yerr=std_val if std_val > 0 else None,
                         label=protocol if y_idx == 0 else "",
                         color=PROTOCOL_COLORS.get(protocol, 'gray'),
                         hatch=PROTOCOL_HATCHES.get(protocol, ''),
                         edgecolor='black', linewidth=1,
                         capsize=3, alpha=0.9)
    
    # Configure axes with SEPARATE labels (NO SHARED LABELS)
    ax.set_xlabel(y_label, fontsize=13, fontweight='bold', labelpad=10)
    ax.set_ylabel(x_label, fontsize=13, fontweight='bold', labelpad=10)
    
    # X-axis: parameter values (ALL VALUES, sorted)
    ax.set_xticks(x_positions)
    ax.set_xticklabels([str(v) for v in y_values], fontsize=11, fontweight='bold')
    
    # Y-axis: metric values (starts at 0)
    ax.set_ylim(bottom=0)
    
    # Title
    title = f"{y_label} vs {x_label}"
    if protocol_filter:
        title += f" [{protocol_filter} only]"
    ax.set_title(title, fontsize=14, fontweight='bold', pad=15)
    
    # Grid
    ax.grid(axis='y', alpha=0.3, linestyle='--')
    ax.set_axisbelow(True)
    
    # Legend (only show once)
    ax.legend(title='Protocol', loc='upper right', fontsize=10,
              frameon=True, fancybox=True, shadow=True)
    
    plt.tight_layout()
    
    # Save with proper naming: [csv_basename]_[y_var]_vs_[x_var].png
    plt.savefig(output_file, dpi=300, bbox_inches='tight', facecolor='white')
    plt.close()
    
    return True


def generate_all_plots(df, output_dir, csv_basename):
    """Generate all Y-axis vs X-axis plots (25 total)."""
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"\n{'='*70}")
    print(f"Generating All Plots")
    print(f"{'='*70}")
    print(f"Y-axis (Parameters): {[y[1] for y in Y_AXIS_PARAMETERS]}")
    print(f"X-axis (Metrics): {[x[1] for x in X_AXIS_METRICS]}")
    print(f"Total: {len(Y_AXIS_PARAMETERS)} × {len(X_AXIS_METRICS)} = {len(Y_AXIS_PARAMETERS) * len(X_AXIS_METRICS)} plots")
    print(f"Output: {output_dir}")
    print(f"{'='*70}\n")
    
    plot_count = 0
    success_count = 0
    
    for y_col, y_label, protocol_filter in Y_AXIS_PARAMETERS:
        for x_col, x_label in X_AXIS_METRICS:
            plot_count += 1
            
            # File naming: [csv_basename]_[y_var]_vs_[x_var].png
            y_name = y_col.lower().replace(' ', '_')
            x_name = x_col.lower().replace('_pct', '').replace('total_', '').replace('avg_', '')
            filename = f"{csv_basename}_{y_name}_vs_{x_name}.png"
            output_file = output_dir / filename
            
            print(f"[{plot_count}/25] {y_label} vs {x_label}")
            
            success = create_plot(df, y_col, y_label, x_col, x_label,
                                output_file, protocol_filter, csv_basename)
            
            if success:
                print(f"  ✓ Saved: {filename}")
                success_count += 1
            else:
                print(f"  ✗ Skipped")
    
    print(f"\n{'='*70}")
    print(f"✓ Generated {success_count}/{plot_count} plots")
    print(f"{'='*70}\n")
    
    return success_count


def main():
    parser = argparse.ArgumentParser(
        description="UWSN V2 - Comprehensive Plotting System"
    )
    parser.add_argument(
        "csv_file",
        help="Path to the results CSV file"
    )
    parser.add_argument(
        "--output-dir", "-o",
        default="./plots",
        help="Output directory for plots (default: ./plots)"
    )
    
    args = parser.parse_args()
    
    # Get CSV basename for file naming
    csv_basename = Path(args.csv_file).stem
    
    # Load data
    df = load_and_prepare_data(args.csv_file)
    
    # Generate plots
    generate_all_plots(df, args.output_dir, csv_basename)
    
    print(f"Done! All plots saved to: {args.output_dir}")
    print(f"Naming: {csv_basename}_[y_variable]_vs_[x_variable].png")


if __name__ == "__main__":
    main()

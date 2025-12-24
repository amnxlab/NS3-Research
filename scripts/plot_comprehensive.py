#!/usr/bin/env python3
"""
UWSN V2 - Comprehensive Visualization Matrix
=============================================
Generates 25 bar charts: 5 parameters (Y-axis) × 5 metrics (X-axis)

Each chart shows protocols side-by-side for easy comparison.

Usage:
    python3 plot_comprehensive.py results.csv --output-dir comprehensive_plots/
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import argparse
import sys
from pathlib import Path

# Protocol colors
PROTOCOL_COLORS = {
    'VBF': '#2ecc71',
    'DBR': '#3498db',
    'flooding': '#e74c3c'
}

# Set publication-quality defaults
plt.rcParams.update({
    'font.size': 11,
    'axes.labelsize': 12,
    'axes.titlesize': 13,
    'xtick.labelsize': 10,
    'ytick.labelsize': 10,
    'legend.fontsize': 10,
    'figure.dpi': 150,
    'savefig.dpi': 300,
    'savefig.bbox': 'tight',
})


def load_data(csv_file):
    """Load and validate CSV data."""
    try:
        df = pd.read_csv(csv_file)
        print(f"Loaded {len(df)} rows from {csv_file}")
        print(f"Protocols: {df['protocol'].unique()}")
        print(f"Parameters found:")
        for param in ['nodes', 'range', 'minDepth', 'maxDepth', 'vbfWidth', 'mobility']:
            if param in df.columns:
                print(f"  - {param}: {sorted(df[param].unique())}")
        return df
    except Exception as e:
        print(f"ERROR loading {csv_file}: {e}")
        sys.exit(1)


def plot_bar_chart(df, y_param, y_value, x_metric, output_file):
    """
    Create a bar chart comparing protocols for a specific parameter value and metric.
    
    Args:
        df: DataFrame with results
        y_param: Y-axis parameter name (e.g., 'nodes', 'range')
        y_value: Specific value of Y parameter (e.g., 100, 120)
        x_metric: X-axis metric (e.g., 'pdr', 'total_consumed_energy')
        output_file: Path to save the plot
    """
    # Filter data for specific parameter value
    if y_param == 'depth':
        # Special case: depth uses two columns
        data = df[(df['minDepth'] == y_value[0]) & (df['maxDepth'] == y_value[1])]
        y_label = f"Depth: [{y_value[0]:.0f}, {y_value[1]:.0f}]m"
    else:
        data = df[df[y_param] == y_value]
        if y_param == 'nodes':
            y_label = f"{y_value} Nodes"
        elif y_param == 'range':
            y_label = f"Range: {y_value}m"
        elif y_param == 'vbfWidth':
            y_label = f"VBF Width: {y_value}m"
        elif y_param == 'mobility':
            y_label = f"Mobility: {y_value}"
        else:
            y_label = f"{y_param}: {y_value}"
    
    if len(data) == 0:
        print(f"  ⚠ No data for {y_param}={y_value}, skipping...")
        return False
    
    # Group by protocol and calculate mean/std
    protocols = sorted(data['protocol'].unique())
    stats = data.groupby('protocol')[x_metric].agg(['mean', 'std']).reset_index()
    
    # Create bar chart
    fig, ax = plt.subplots(figsize=(8, 5))
    
    x_pos = np.arange(len(protocols))
    width = 0.6
    
    bars = ax.bar(x_pos, stats['mean'],
                  width=width,
                  yerr=stats['std'],
                  color=[PROTOCOL_COLORS.get(p, 'gray') for p in protocols],
                  capsize=5,
                  alpha=0.85,
                  edgecolor='black',
                  linewidth=1.5)
    
    # Labels and formatting
    ax.set_xlabel('Protocol', fontsize=12, fontweight='bold')
    
    # X-axis metric label
    metric_labels = {
        'pdr': 'Packet Delivery Ratio (%)',
        'total_consumed_energy': 'Total Energy Consumed (J)',
        'total_residual_energy': 'Residual Energy (J)',
        'avg_delay_ms': 'Average Delay (ms)',
        'network_lifetime_s': 'Network Lifetime (s)',
        'energy_efficiency': 'Energy Efficiency (pkts/J)'
    }
    
    ylabel = metric_labels.get(x_metric, x_metric)
    ax.set_ylabel(ylabel, fontsize=12, fontweight='bold')
    
    # Title
    ax.set_title(f'{ylabel} @ {y_label}', fontsize=13, fontweight='bold')
    
    # X-axis
    ax.set_xticks(x_pos)
    ax.set_xticklabels(protocols, fontsize=11)
    
    # Grid
    ax.grid(axis='y', alpha=0.3, linestyle='--')
    
    # Value labels on bars
    for i, bar in enumerate(bars):
        height = bar.get_height()
        if x_metric == 'pdr':
            label = f'{height*100:.1f}%'
        elif 'energy' in x_metric.lower():
            label = f'{height:.1f}J'
        elif 'delay' in x_metric.lower():
            label = f'{height:.1f}ms'
        elif 'lifetime' in x_metric.lower():
            label = f'{height:.0f}s'
        else:
            label = f'{height:.2f}'
        
        ax.text(bar.get_x() + bar.get_width()/2., height,
                label, ha='center', va='bottom', fontsize=9, fontweight='bold')
    
    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    
    return True


def generate_comprehensive_plots(df, output_dir):
    """
    Generate 25 bar charts: 5 Y-parameters × 5 X-metrics
    
    Y-axis parameters (what we vary):
    1. Number of nodes
    2. Communication range
    3. Depth variation
    4. VBF Width
    5. Mobility
    
    X-axis metrics (what we measure):
    1. PDR
    2. Residual Energy
    3. Energy Efficiency
    4. Average Delay (if available)
    5. Network Lifetime (if available)
    """
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"\n{'='*70}")
    print(f"Generating Comprehensive Visualization Matrix")
    print(f"{'='*70}\n")
    
    # Define Y-axis parameters and their unique values
    y_parameters = {
        'nodes': sorted(df['nodes'].unique()),
        'range': sorted(df['range'].unique()),
        'vbfWidth': sorted(df['vbfWidth'].unique()),
        'mobility': sorted(df['mobility'].unique()),
    }
    
    # Handle depth as special case (two columns)
    depth_combinations = df[['minDepth', 'maxDepth']].drop_duplicates().values.tolist()
    if len(depth_combinations) > 0:
        y_parameters['depth'] = depth_combinations
    
    # Define X-axis metrics
    x_metrics = []
    if 'pdr' in df.columns:
        x_metrics.append('pdr')
    if 'total_residual_energy' in df.columns:
        x_metrics.append('total_residual_energy')
    if 'total_consumed_energy' in df.columns:
        # Calculate energy efficiency
        df['energy_efficiency'] = df['sink_recv'] / df['total_consumed_energy']
        x_metrics.append('energy_efficiency')
    if 'avg_delay_ms' in df.columns:
        x_metrics.append('avg_delay_ms')
    if 'network_lifetime_s' in df.columns:
        x_metrics.append('network_lifetime_s')
    
    print(f"Y-axis parameters: {list(y_parameters.keys())}")
    print(f"X-axis metrics: {x_metrics}")
    print(f"Total charts to generate: {sum(len(v) for v in y_parameters.values()) * len(x_metrics)}\n")
    
    chart_count = 0
    total_charts = sum(len(v) for v in y_parameters.values()) * len(x_metrics)
    
    # Generate charts for each combination
    for y_param, y_values in y_parameters.items():
        for y_value in y_values:
            for x_metric in x_metrics:
                chart_count += 1
                
                # Generate filename
                if y_param == 'depth':
                    y_str = f"depth_{y_value[0]:.0f}_{y_value[1]:.0f}"
                elif y_param == 'mobility':
                    y_str = f"{y_param}_{y_value}"
                else:
                    y_str = f"{y_param}_{y_value:.0f}"
                
                filename = f"chart_{y_str}_vs_{x_metric}.png"
                output_file = output_dir / filename
                
                print(f"[{chart_count}/{total_charts}] Generating: {filename}")
                
                success = plot_bar_chart(df, y_param, y_value, x_metric, output_file)
                if success:
                    print(f"  ✓ Saved: {output_file}")
    
    print(f"\n{'='*70}")
    print(f"✓ Generated {chart_count} charts in {output_dir}/")
    print(f"{'='*70}\n")


def main():
    parser = argparse.ArgumentParser(
        description="UWSN V2 - Comprehensive Visualization Matrix"
    )
    parser.add_argument(
        "csv_file",
        help="Path to the results CSV file"
    )
    parser.add_argument(
        "--output-dir", "-o",
        default="./comprehensive_plots",
        help="Output directory for charts (default: ./comprehensive_plots)"
    )
    
    args = parser.parse_args()
    
    # Load data
    df = load_data(args.csv_file)
    
    # Generate comprehensive plots
    generate_comprehensive_plots(df, args.output_dir)
    
    print("Done! View charts in:", args.output_dir)


if __name__ == "__main__":
    main()


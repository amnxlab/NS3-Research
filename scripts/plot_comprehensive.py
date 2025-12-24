#!/usr/bin/env python3
"""
UWSN V2 - Comprehensive 5x5 Visualization Matrix
=================================================
Generates a 5x5 grid of bar charts:
- Rows (Y-axis): Parameters (Nodes, Depth, VBF Width, Mobility, Range)
- Columns (X-axis): Metrics (Residual Energy, Energy Efficiency %, PDR, Avg Delay, Network Lifetime)
- Bars: Protocols (VBF, DBR, Flooding) with color legend

Usage:
    python3 plot_comprehensive.py results.csv --output-dir ./comprehensive_plots/
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import argparse
import sys
from pathlib import Path

# Protocol colors (consistent across all charts)
PROTOCOL_COLORS = {
    'VBF': '#27ae60',       # Green
    'DBR': '#2980b9',       # Blue
    'flooding': '#c0392b'   # Red
}

# Set publication-quality defaults
plt.rcParams.update({
    'font.family': 'sans-serif',
    'font.size': 10,
    'axes.labelsize': 11,
    'axes.titlesize': 12,
    'xtick.labelsize': 9,
    'ytick.labelsize': 9,
    'legend.fontsize': 9,
    'figure.dpi': 150,
    'savefig.dpi': 300,
    'savefig.bbox': 'tight',
})

# Define the 5 metrics (X-axis / columns)
METRICS = [
    ('total_residual_energy', 'Residual Energy (J)'),
    ('energy_efficiency_pct', 'Energy Efficiency (%)'),
    ('pdr_pct', 'PDR (%)'),
    ('avg_delay_ms', 'Average Delay (ms)'),
    ('network_lifetime_s', 'Network Lifetime (s)')
]

# Define the 5 parameters (Y-axis / rows)
PARAMETERS = [
    ('nodes', 'Number of Nodes'),
    ('depth', 'Depth Variation'),
    ('vbfWidth', 'VBF Width (m)'),
    ('mobility', 'Mobility Model'),
    ('range', 'Communication Range (m)')
]


def load_and_prepare_data(csv_file):
    """Load CSV and compute derived metrics."""
    try:
        df = pd.read_csv(csv_file)
        print(f"Loaded {len(df)} rows from {csv_file}")
        print(f"Protocols: {list(df['protocol'].unique())}")
        
        # Compute derived metrics
        # PDR as percentage (0-100)
        if 'pdr' in df.columns:
            df['pdr_pct'] = df['pdr'] * 100
        else:
            df['pdr_pct'] = 0
        
        # Energy efficiency as percentage: (received/sent) / energy * 100
        if 'sender_sent' in df.columns and 'sink_recv' in df.columns and 'total_consumed_energy' in df.columns:
            # Avoid division by zero
            df['energy_efficiency_pct'] = np.where(
                df['total_consumed_energy'] > 0,
                (df['sink_recv'] / df['total_consumed_energy']) * 100,
                0
            )
        else:
            df['energy_efficiency_pct'] = 0
        
        # Ensure other metrics exist with defaults
        if 'avg_delay_ms' not in df.columns:
            df['avg_delay_ms'] = 0
        if 'network_lifetime_s' not in df.columns:
            df['network_lifetime_s'] = 0
        if 'total_residual_energy' not in df.columns:
            df['total_residual_energy'] = 0
            
        # Create depth label for grouping
        if 'minDepth' in df.columns and 'maxDepth' in df.columns:
            df['depth'] = df.apply(lambda r: f"{r['minDepth']:.0f}-{r['maxDepth']:.0f}m", axis=1)
        else:
            df['depth'] = 'default'
        
        return df
        
    except Exception as e:
        print(f"ERROR loading {csv_file}: {e}")
        sys.exit(1)


def create_bar_chart(ax, df, param_name, param_value, metric_col, metric_label, show_legend=False):
    """
    Create a single bar chart for one cell in the 5x5 grid.
    
    Args:
        ax: Matplotlib axis
        df: DataFrame with data
        param_name: Parameter column name (e.g., 'nodes', 'depth')
        param_value: Specific value to filter (e.g., 100, '0-400m')
        metric_col: Metric column name (e.g., 'pdr_pct', 'total_residual_energy')
        metric_label: Human-readable metric name
        show_legend: Whether to show legend on this chart
    """
    # Filter data for this parameter value
    if param_name == 'depth':
        data = df[df['depth'] == param_value]
    else:
        data = df[df[param_name] == param_value]
    
    if len(data) == 0:
        ax.text(0.5, 0.5, 'No Data', ha='center', va='center', transform=ax.transAxes)
        ax.set_title(f'{param_value}', fontsize=10)
        return
    
    # Get protocols and calculate means
    protocols = sorted(data['protocol'].unique())
    means = []
    colors = []
    
    for protocol in protocols:
        protocol_data = data[data['protocol'] == protocol]
        mean_val = protocol_data[metric_col].mean()
        means.append(mean_val)
        colors.append(PROTOCOL_COLORS.get(protocol, 'gray'))
    
    # Create bar chart
    x_pos = np.arange(len(protocols))
    width = 0.6
    
    bars = ax.bar(x_pos, means, width=width, color=colors, 
                  edgecolor='black', linewidth=0.8, alpha=0.85)
    
    # Configure axis
    ax.set_xticks(x_pos)
    ax.set_xticklabels(protocols, fontsize=8)
    ax.set_title(f'{param_value}', fontsize=10, fontweight='bold')
    ax.grid(axis='y', alpha=0.3, linestyle='--')
    
    # Add value labels on bars
    for bar, mean_val in zip(bars, means):
        height = bar.get_height()
        if height > 0:
            # Format based on metric
            if 'pct' in metric_col or 'PDR' in metric_label:
                label = f'{mean_val:.1f}%'
            elif 'energy' in metric_col.lower():
                label = f'{mean_val:.1f}'
            elif 'delay' in metric_col.lower():
                label = f'{mean_val:.1f}'
            elif 'lifetime' in metric_col.lower():
                label = f'{mean_val:.0f}'
            else:
                label = f'{mean_val:.1f}'
            
            ax.text(bar.get_x() + bar.get_width()/2., height,
                    label, ha='center', va='bottom', fontsize=7, fontweight='bold')
    
    # Add legend if requested
    if show_legend:
        from matplotlib.patches import Patch
        legend_elements = [Patch(facecolor=PROTOCOL_COLORS.get(p, 'gray'), 
                                  edgecolor='black', label=p) 
                          for p in ['VBF', 'DBR', 'flooding']]
        ax.legend(handles=legend_elements, loc='upper right', fontsize=7)


def generate_5x5_matrix(df, output_dir):
    """
    Generate the 5x5 visualization matrix.
    
    Rows (Y-axis): Parameters (Nodes, Depth, VBF Width, Mobility, Range)
    Columns (X-axis): Metrics (Residual Energy, Energy Eff %, PDR, Delay, Lifetime)
    """
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"\n{'='*70}")
    print(f"Generating 5x5 Visualization Matrix")
    print(f"{'='*70}")
    print(f"Rows: {[p[1] for p in PARAMETERS]}")
    print(f"Cols: {[m[1] for m in METRICS]}")
    print(f"{'='*70}\n")
    
    # Get unique values for each parameter
    param_values = {}
    for param_col, param_label in PARAMETERS:
        if param_col == 'depth':
            param_values[param_col] = sorted(df['depth'].unique())
        elif param_col in df.columns:
            param_values[param_col] = sorted(df[param_col].unique())
        else:
            param_values[param_col] = []
        print(f"  {param_label}: {param_values[param_col]}")
    
    # Generate one 5x5 figure for each parameter value combination
    # Actually, we need one figure PER parameter, showing all metrics
    
    for param_col, param_label in PARAMETERS:
        values = param_values.get(param_col, [])
        if not values:
            print(f"\n⚠ No data for {param_label}, skipping...")
            continue
        
        # Create figure with subplots: 1 row per value, 5 columns (metrics)
        n_values = len(values)
        fig, axes = plt.subplots(n_values, 5, figsize=(20, 4 * n_values))
        
        # Handle single row case
        if n_values == 1:
            axes = axes.reshape(1, -1)
        
        # Fill in the grid
        for row_idx, param_value in enumerate(values):
            for col_idx, (metric_col, metric_label) in enumerate(METRICS):
                ax = axes[row_idx, col_idx]
                
                # Add legend only on top-right cell
                show_legend = (row_idx == 0 and col_idx == 4)
                
                create_bar_chart(ax, df, param_col, param_value, 
                               metric_col, metric_label, show_legend)
                
                # Add column headers on first row
                if row_idx == 0:
                    ax.set_xlabel(metric_label, fontsize=10, fontweight='bold')
        
        # Add row labels on left side
        for row_idx, param_value in enumerate(values):
            axes[row_idx, 0].set_ylabel(f'{param_label}\n{param_value}', 
                                        fontsize=10, fontweight='bold', rotation=0, 
                                        ha='right', va='center', labelpad=50)
        
        # Main title
        fig.suptitle(f'Protocol Comparison by {param_label}', 
                     fontsize=16, fontweight='bold', y=1.02)
        
        plt.tight_layout()
        
        # Save figure
        output_file = output_dir / f'matrix_{param_col}.png'
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        plt.close()
        print(f"✓ Saved: {output_file}")
    
    # Also generate a combined summary figure
    generate_summary_figure(df, output_dir)
    
    print(f"\n{'='*70}")
    print(f"✓ All visualization matrices saved to {output_dir}/")
    print(f"{'='*70}\n")


def generate_summary_figure(df, output_dir):
    """Generate a single summary figure showing one value per parameter."""
    output_dir = Path(output_dir)
    
    # Create 5x5 grid (one row per parameter, one column per metric)
    fig, axes = plt.subplots(5, 5, figsize=(22, 18))
    
    # For each parameter, use the most common/default value
    default_values = {
        'nodes': df['nodes'].mode().iloc[0] if 'nodes' in df.columns else None,
        'depth': df['depth'].mode().iloc[0] if 'depth' in df.columns else None,
        'vbfWidth': df['vbfWidth'].mode().iloc[0] if 'vbfWidth' in df.columns else None,
        'mobility': df['mobility'].mode().iloc[0] if 'mobility' in df.columns else None,
        'range': df['range'].mode().iloc[0] if 'range' in df.columns else None,
    }
    
    print(f"\nGenerating summary figure with default values:")
    for k, v in default_values.items():
        print(f"  {k}: {v}")
    
    for row_idx, (param_col, param_label) in enumerate(PARAMETERS):
        param_value = default_values.get(param_col)
        if param_value is None:
            continue
            
        for col_idx, (metric_col, metric_label) in enumerate(METRICS):
            ax = axes[row_idx, col_idx]
            
            # Add legend only on top-right cell
            show_legend = (row_idx == 0 and col_idx == 4)
            
            create_bar_chart(ax, df, param_col, param_value,
                           metric_col, metric_label, show_legend)
            
            # Add column headers on first row
            if row_idx == 0:
                ax.set_title(metric_label, fontsize=11, fontweight='bold')
            
            # Add row labels on first column
            if col_idx == 0:
                ax.set_ylabel(f'{param_label}\n({param_value})', 
                             fontsize=10, fontweight='bold')
    
    # Main title
    fig.suptitle('UWSN Protocol Comparison - Summary Matrix\n(5 Parameters × 5 Metrics)', 
                 fontsize=18, fontweight='bold', y=1.01)
    
    # Add color legend at bottom
    from matplotlib.patches import Patch
    legend_elements = [Patch(facecolor=PROTOCOL_COLORS['VBF'], edgecolor='black', label='VBF'),
                       Patch(facecolor=PROTOCOL_COLORS['DBR'], edgecolor='black', label='DBR'),
                       Patch(facecolor=PROTOCOL_COLORS['flooding'], edgecolor='black', label='Flooding')]
    fig.legend(handles=legend_elements, loc='lower center', ncol=3, 
               fontsize=12, frameon=True, bbox_to_anchor=(0.5, -0.02))
    
    plt.tight_layout()
    
    output_file = output_dir / 'summary_5x5_matrix.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"✓ Saved: {output_file}")


def main():
    parser = argparse.ArgumentParser(
        description="UWSN V2 - 5x5 Visualization Matrix"
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
    
    # Load and prepare data
    df = load_and_prepare_data(args.csv_file)
    
    # Generate visualization matrix
    generate_5x5_matrix(df, args.output_dir)
    
    print("Done! View charts in:", args.output_dir)


if __name__ == "__main__":
    main()

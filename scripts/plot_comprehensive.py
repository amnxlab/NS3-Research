#!/usr/bin/env python3
"""
UWSN V2 - Comprehensive 5x5 Visualization Matrix
=================================================
Generates a 5x5 grid of bar charts:
- Rows (5): Parameters (Nodes, Depth, VBF Width, Mobility, Range)
- Each row shows ONE chart with 5 metrics on X-axis
- Bars: Protocols (VBF, DBR, Flooding) grouped for each metric

X-axis (always): Residual Energy, Energy Eff %, PDR %, Avg Delay, Network Lifetime
Y-axis (values): Numeric values for each metric

Usage:
    python3 plot_comprehensive.py results.csv --output-dir ./comprehensive_plots/
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import argparse
import sys
from pathlib import Path
from matplotlib.patches import Patch

# Protocol colors (consistent across all charts)
PROTOCOL_COLORS = {
    'VBF': '#8B0000',       # Dark Red
    'DBR': '#4169E1',       # Royal Blue  
    'flooding': '#228B22'   # Forest Green
}

# Protocol hatches for distinction
PROTOCOL_HATCHES = {
    'VBF': '',              # Solid
    'DBR': '///',           # Diagonal lines
    'flooding': 'xxx'       # Cross-hatch
}

# Set publication-quality defaults
plt.rcParams.update({
    'font.family': 'serif',
    'font.size': 10,
    'axes.labelsize': 11,
    'axes.titlesize': 12,
    'xtick.labelsize': 9,
    'ytick.labelsize': 9,
    'legend.fontsize': 10,
    'figure.dpi': 150,
    'savefig.dpi': 300,
    'savefig.bbox': 'tight',
})

# Define the 5 metrics (X-axis)
METRICS = [
    ('total_residual_energy', 'Residual\nEnergy (J)'),
    ('energy_efficiency_pct', 'Energy\nEfficiency (%)'),
    ('pdr_pct', 'PDR (%)'),
    ('avg_delay_ms', 'Average\nDelay (ms)'),
    ('network_lifetime_s', 'Network\nLifetime (s)')
]

# Define the 5 parameters (rows)  
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
        if 'pdr' in df.columns:
            df['pdr_pct'] = df['pdr'] * 100
        else:
            df['pdr_pct'] = 0
        
        if 'sender_sent' in df.columns and 'sink_recv' in df.columns and 'total_consumed_energy' in df.columns:
            df['energy_efficiency_pct'] = np.where(
                df['total_consumed_energy'] > 0,
                (df['sink_recv'] / df['total_consumed_energy']) * 100,
                0
            )
        else:
            df['energy_efficiency_pct'] = 0
        
        if 'avg_delay_ms' not in df.columns:
            df['avg_delay_ms'] = 0
        if 'network_lifetime_s' not in df.columns:
            df['network_lifetime_s'] = 0
        if 'total_residual_energy' not in df.columns:
            df['total_residual_energy'] = 0
            
        if 'minDepth' in df.columns and 'maxDepth' in df.columns:
            df['depth'] = df.apply(lambda r: f"{r['minDepth']:.0f}-{r['maxDepth']:.0f}m", axis=1)
        else:
            df['depth'] = 'default'
        
        return df
        
    except Exception as e:
        print(f"ERROR loading {csv_file}: {e}")
        sys.exit(1)


def format_value(val, metric_col):
    """Format value for display on bar."""
    if val == 0:
        return "0"
    if 'pct' in metric_col or 'pdr' in metric_col.lower():
        return f'{val:.1f}%'
    elif 'delay' in metric_col.lower():
        if val > 1000:
            return f'{val/1000:.1f}k'
        return f'{val:.0f}'
    elif 'lifetime' in metric_col.lower() or 'energy' in metric_col.lower():
        if val > 100000:
            return f'{val/1000:.0f}k'
        elif val > 1000:
            return f'{val/1000:.1f}k'
        return f'{val:.0f}'
    else:
        return f'{val:.1f}'


def create_grouped_bar_chart(ax, df, param_col, param_value, row_label):
    """
    Create a grouped bar chart with 5 metrics on X-axis.
    Each metric has bars for each protocol side by side.
    """
    # Filter data for this parameter value
    if param_col == 'depth':
        data = df[df['depth'] == param_value]
    else:
        data = df[df[param_col] == param_value]
    
    if len(data) == 0:
        ax.text(0.5, 0.5, 'No Data', ha='center', va='center', 
                transform=ax.transAxes, fontsize=12, style='italic')
        ax.set_ylabel(f'{row_label}\n({param_value})', fontsize=11, fontweight='bold')
        return
    
    protocols = sorted(data['protocol'].unique())
    n_protocols = len(protocols)
    n_metrics = len(METRICS)
    
    # X positions for metrics
    x = np.arange(n_metrics)
    width = 0.25  # Width of each bar
    
    # Calculate offsets for grouped bars
    offsets = np.linspace(-(n_protocols-1)*width/2, (n_protocols-1)*width/2, n_protocols)
    
    # For each protocol, get all metric values and plot
    all_bars = []
    max_val = 0
    
    for p_idx, protocol in enumerate(protocols):
        protocol_data = data[data['protocol'] == protocol]
        
        values = []
        for metric_col, metric_label in METRICS:
            val = protocol_data[metric_col].mean()
            values.append(val)
            if val > max_val:
                max_val = val
        
        bars = ax.bar(x + offsets[p_idx], values, width,
                      color=PROTOCOL_COLORS.get(protocol, 'gray'),
                      hatch=PROTOCOL_HATCHES.get(protocol, ''),
                      edgecolor='black', linewidth=0.8,
                      label=protocol, alpha=0.9)
        all_bars.append((protocol, bars, values))
    
    # Add value labels on bars
    for protocol, bars, values in all_bars:
        for bar, val, (metric_col, _) in zip(bars, values, METRICS):
            if val > 0:
                height = bar.get_height()
                label = format_value(val, metric_col)
                ax.text(bar.get_x() + bar.get_width()/2., height,
                        label, ha='center', va='bottom', fontsize=7,
                        fontweight='bold', rotation=45)
    
    # Configure axes
    ax.set_xticks(x)
    ax.set_xticklabels([m[1] for m in METRICS], fontsize=9)
    ax.set_ylabel(f'{row_label}\n({param_value})', fontsize=10, fontweight='bold')
    ax.grid(axis='y', alpha=0.3, linestyle='--')
    ax.set_ylim(bottom=0)
    
    # Adjust top margin for labels
    if max_val > 0:
        ax.set_ylim(top=max_val * 1.25)


def generate_5x5_matrix(df, output_dir):
    """
    Generate the visualization with 5 rows (parameters).
    Each row has 5 metrics on X-axis with protocols as grouped bars.
    """
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"\n{'='*70}")
    print(f"Generating Visualization Matrix")
    print(f"{'='*70}")
    print(f"Rows: {[p[1] for p in PARAMETERS]}")
    print(f"X-axis (Metrics): {[m[1].replace(chr(10), ' ') for m in METRICS]}")
    print(f"{'='*70}\n")
    
    # Get unique values for each parameter
    param_values = {}
    for param_col, param_label in PARAMETERS:
        if param_col == 'depth':
            values = sorted(df['depth'].unique())
        elif param_col in df.columns:
            values = sorted(df[param_col].unique())
        else:
            values = []
        param_values[param_col] = values[0] if values else None
        print(f"  {param_label}: {param_values[param_col]}")
    
    # Create figure with 5 rows (one per parameter)
    fig, axes = plt.subplots(5, 1, figsize=(14, 20))
    
    for row_idx, (param_col, param_label) in enumerate(PARAMETERS):
        param_value = param_values.get(param_col)
        if param_value is None:
            continue
        
        ax = axes[row_idx]
        create_grouped_bar_chart(ax, df, param_col, param_value, param_label)
    
    # Main title
    fig.suptitle('UWSN Protocol Comparison\nX-axis: 5 Metrics | Bars: Protocols (VBF, DBR, Flooding)', 
                 fontsize=16, fontweight='bold', y=0.99)
    
    # Legend at bottom
    legend_elements = [
        Patch(facecolor=PROTOCOL_COLORS['VBF'], edgecolor='black',
              hatch=PROTOCOL_HATCHES['VBF'], label='VBF'),
        Patch(facecolor=PROTOCOL_COLORS['DBR'], edgecolor='black',
              hatch=PROTOCOL_HATCHES['DBR'], label='DBR'),
        Patch(facecolor=PROTOCOL_COLORS['flooding'], edgecolor='black',
              hatch=PROTOCOL_HATCHES['flooding'], label='Flooding')
    ]
    
    fig.legend(handles=legend_elements, loc='lower center', ncol=3,
               fontsize=12, frameon=True, fancybox=True, shadow=True,
               bbox_to_anchor=(0.5, 0.01))
    
    plt.tight_layout(rect=[0, 0.04, 1, 0.97])
    
    output_file = output_dir / 'summary_5x5_matrix.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight', facecolor='white')
    plt.close()
    print(f"\n✓ Saved: {output_file}")
    
    # Generate individual charts for each parameter with all values
    generate_individual_charts(df, output_dir)
    
    print(f"\n{'='*70}")
    print(f"✓ All visualization saved to {output_dir}/")
    print(f"{'='*70}\n")


def generate_individual_charts(df, output_dir):
    """Generate individual charts for each parameter with all its values."""
    
    for param_col, param_label in PARAMETERS:
        if param_col == 'depth':
            all_values = sorted(df['depth'].unique())
        elif param_col in df.columns:
            all_values = sorted(df[param_col].unique())
        else:
            continue
        
        if not all_values:
            continue
        
        n_values = len(all_values)
        
        fig, axes = plt.subplots(n_values, 1, figsize=(14, 5 * n_values))
        
        if n_values == 1:
            axes = [axes]
        
        for row_idx, param_value in enumerate(all_values):
            ax = axes[row_idx]
            create_grouped_bar_chart(ax, df, param_col, param_value, param_label)
        
        fig.suptitle(f'Protocol Comparison by {param_label}\nX-axis: 5 Metrics | Bars: Protocols', 
                     fontsize=14, fontweight='bold', y=0.99)
        
        legend_elements = [
            Patch(facecolor=PROTOCOL_COLORS['VBF'], edgecolor='black',
                  hatch=PROTOCOL_HATCHES['VBF'], label='VBF'),
            Patch(facecolor=PROTOCOL_COLORS['DBR'], edgecolor='black',
                  hatch=PROTOCOL_HATCHES['DBR'], label='DBR'),
            Patch(facecolor=PROTOCOL_COLORS['flooding'], edgecolor='black',
                  hatch=PROTOCOL_HATCHES['flooding'], label='Flooding')
        ]
        
        fig.legend(handles=legend_elements, loc='lower center', ncol=3,
                   fontsize=11, frameon=True, bbox_to_anchor=(0.5, 0.01))
        
        plt.tight_layout(rect=[0, 0.05, 1, 0.97])
        
        output_file = output_dir / f'matrix_{param_col}.png'
        plt.savefig(output_file, dpi=300, bbox_inches='tight', facecolor='white')
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
    
    df = load_and_prepare_data(args.csv_file)
    generate_5x5_matrix(df, args.output_dir)
    
    print("Done! View charts in:", args.output_dir)


if __name__ == "__main__":
    main()

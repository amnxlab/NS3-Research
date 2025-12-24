#!/usr/bin/env python3
"""
UWSN V2 - Comprehensive 5x5 Visualization Matrix
=================================================
Generates a 5x5 grid of bar charts:
- Rows (5): Parameters (Nodes, Depth, VBF Width, Mobility, Range)
- Columns (5): Metrics (Residual Energy, Energy Efficiency %, PDR, Avg Delay, Network Lifetime)
- Bars: Protocols (VBF, DBR, Flooding) side-by-side with color legend OUTSIDE

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

# Protocol patterns for distinction
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

# Define the 5 metrics (columns)
METRICS = [
    ('total_residual_energy', 'Residual Energy (J)'),
    ('energy_efficiency_pct', 'Energy Efficiency (%)'),
    ('pdr_pct', 'PDR (%)'),
    ('avg_delay_ms', 'Average Delay (ms)'),
    ('network_lifetime_s', 'Network Lifetime (s)')
]

# Define the 5 parameters (rows)  
# Note: vbfWidth only applies to VBF protocol
PARAMETERS = [
    ('nodes', 'Number of Nodes', None),           # applies to all protocols
    ('depth', 'Depth Variation', None),           # applies to all protocols
    ('vbfWidth', 'VBF Width (m)', 'VBF'),         # VBF only
    ('mobility', 'Mobility Model', None),         # applies to all protocols
    ('range', 'Communication Range (m)', None)    # applies to all protocols
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
        
        # Energy efficiency as percentage
        if 'sender_sent' in df.columns and 'sink_recv' in df.columns and 'total_consumed_energy' in df.columns:
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


def format_xaxis_value(val, metric_col):
    """Format numeric value for X-axis display."""
    if val == 0:
        return "0"
    if 'pct' in metric_col or 'pdr' in metric_col.lower():
        return f'{val:.1f}'
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
        return f'{val:.0f}'


def create_bar_chart(ax, df, param_name, param_value, metric_col, metric_label, protocol_filter=None):
    """
    Create a single bar chart for one cell in the 5x5 grid.
    Protocols are shown as grouped bars.
    X-axis shows numeric values for each protocol.
    
    Args:
        protocol_filter: If set, only show this protocol (e.g., 'VBF' for vbfWidth)
    """
    # Filter data for this parameter value
    if param_name == 'depth':
        data = df[df['depth'] == param_value]
    else:
        data = df[df[param_name] == param_value]
    
    if len(data) == 0:
        ax.text(0.5, 0.5, 'No Data', ha='center', va='center', 
                transform=ax.transAxes, fontsize=10, style='italic')
        return
    
    # Get protocols (filter if needed)
    if protocol_filter:
        # Only show the specific protocol for this parameter
        protocols = [protocol_filter] if protocol_filter in data['protocol'].unique() else []
        if not protocols:
            ax.text(0.5, 0.5, f'VBF Only\n(N/A for others)', ha='center', va='center', 
                    transform=ax.transAxes, fontsize=9, style='italic')
            return
    else:
        protocols = sorted(data['protocol'].unique())
    
    means = []
    for protocol in protocols:
        protocol_data = data[data['protocol'] == protocol]
        mean_val = protocol_data[metric_col].mean()
        means.append(mean_val)
    
    # Create bar chart - protocols on X-axis
    x_pos = np.arange(len(protocols))
    width = 0.6
    
    bars = []
    for i, (protocol, mean_val) in enumerate(zip(protocols, means)):
        bar = ax.bar(x_pos[i], mean_val, width=width,
                     color=PROTOCOL_COLORS.get(protocol, 'gray'),
                     hatch=PROTOCOL_HATCHES.get(protocol, ''),
                     edgecolor='black', linewidth=1.2, alpha=0.9)
        bars.append(bar[0])
    
    # Configure X-axis with NUMERIC VALUES
    ax.set_xticks(x_pos)
    # Show numeric values on X-axis
    x_labels = [format_xaxis_value(m, metric_col) for m in means]
    ax.set_xticklabels(x_labels, fontsize=9, fontweight='bold')
    
    ax.grid(axis='y', alpha=0.3, linestyle='--')
    
    # Set Y-axis minimum to 0
    ax.set_ylim(bottom=0)
    
    # Adjust Y limit to make room
    max_val = max(means) if means else 1
    if means:
        ax.set_ylim(top=max_val * 1.1)


def generate_5x5_matrix(df, output_dir):
    """
    Generate the 5x5 visualization matrix.
    
    Rows: Parameters (Nodes, Depth, VBF Width, Mobility, Range)
    Columns: Metrics (Residual Energy, Energy Eff %, PDR, Delay, Lifetime)
    """
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"\n{'='*70}")
    print(f"Generating 5x5 Visualization Matrix")
    print(f"{'='*70}")
    print(f"Rows (Parameters): {[p[1] for p in PARAMETERS]}")
    print(f"Columns (Metrics): {[m[1] for m in METRICS]}")
    print(f"{'='*70}\n")
    
    # Get unique values for each parameter (use first/default value)
    param_values = {}
    for param_col, param_label, _ in PARAMETERS:
        if param_col == 'depth':
            values = sorted(df['depth'].unique())
        elif param_col in df.columns:
            values = sorted(df[param_col].unique())
        else:
            values = []
        param_values[param_col] = values[0] if values else None
        print(f"  {param_label}: {param_values[param_col]}")
    
    # Create the 5x5 figure
    fig, axes = plt.subplots(5, 5, figsize=(24, 20))
    
    # Fill in the grid
    for row_idx, (param_col, param_label, protocol_filter) in enumerate(PARAMETERS):
        param_value = param_values.get(param_col)
        if param_value is None:
            continue
            
        for col_idx, (metric_col, metric_label) in enumerate(METRICS):
            ax = axes[row_idx, col_idx]
            
            create_bar_chart(ax, df, param_col, param_value, metric_col, metric_label, protocol_filter)
            
            # Add column headers (metric names) on first row
            if row_idx == 0:
                ax.set_title(metric_label, fontsize=12, fontweight='bold', pad=10)
            
            # Add row labels (parameter names + values) on first column
            if col_idx == 0:
                label = f'{param_label}\n({param_value})'
                if protocol_filter:
                    label += f'\n[{protocol_filter} only]'
                ax.set_ylabel(label, fontsize=10, fontweight='bold')
    
    # Main title
    fig.suptitle('UWSN Protocol Comparison\n5 Parameters × 5 Metrics', 
                 fontsize=18, fontweight='bold', y=0.98)
    
    # Create legend OUTSIDE the plots (at the bottom)
    legend_elements = [
        Patch(facecolor=PROTOCOL_COLORS['VBF'], edgecolor='black', 
              hatch=PROTOCOL_HATCHES['VBF'], label='VBF'),
        Patch(facecolor=PROTOCOL_COLORS['DBR'], edgecolor='black', 
              hatch=PROTOCOL_HATCHES['DBR'], label='DBR'),
        Patch(facecolor=PROTOCOL_COLORS['flooding'], edgecolor='black', 
              hatch=PROTOCOL_HATCHES['flooding'], label='Flooding')
    ]
    
    fig.legend(handles=legend_elements, loc='lower center', ncol=3,
               fontsize=14, frameon=True, fancybox=True, shadow=True,
               bbox_to_anchor=(0.5, 0.01))
    
    plt.tight_layout(rect=[0, 0.05, 1, 0.96])
    
    # Save figure
    output_file = output_dir / 'summary_5x5_matrix.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight', facecolor='white')
    plt.close()
    print(f"\n✓ Saved: {output_file}")
    
    # Also generate individual parameter charts for detailed analysis
    generate_individual_charts(df, output_dir, param_values)
    
    print(f"\n{'='*70}")
    print(f"✓ All visualization saved to {output_dir}/")
    print(f"{'='*70}\n")


def generate_individual_charts(df, output_dir, param_values):
    """Generate individual charts for each parameter with all its values."""
    
    for param_col, param_label, protocol_filter in PARAMETERS:
        # Get all unique values for this parameter
        if param_col == 'depth':
            all_values = sorted(df['depth'].unique())
        elif param_col in df.columns:
            all_values = sorted(df[param_col].unique())
        else:
            continue
        
        if not all_values:
            continue
        
        n_values = len(all_values)
        
        # Create figure: n_values rows × 5 columns
        fig, axes = plt.subplots(n_values, 5, figsize=(24, 4 * n_values + 2))
        
        # Handle single row case
        if n_values == 1:
            axes = axes.reshape(1, -1)
        
        for row_idx, param_value in enumerate(all_values):
            for col_idx, (metric_col, metric_label) in enumerate(METRICS):
                ax = axes[row_idx, col_idx]
                
                create_bar_chart(ax, df, param_col, param_value, metric_col, metric_label, protocol_filter)
                
                # Column headers on first row
                if row_idx == 0:
                    ax.set_title(metric_label, fontsize=11, fontweight='bold', pad=10)
                
                # Row labels on first column
                if col_idx == 0:
                    label = f'{param_value}'
                    if protocol_filter:
                        label += f'\n[{protocol_filter} only]'
                    ax.set_ylabel(label, fontsize=10, fontweight='bold')
        
        # Main title
        fig.suptitle(f'Protocol Comparison by {param_label}', 
                     fontsize=16, fontweight='bold', y=0.99)
        
        # Legend outside
        legend_elements = [
            Patch(facecolor=PROTOCOL_COLORS['VBF'], edgecolor='black',
                  hatch=PROTOCOL_HATCHES['VBF'], label='VBF'),
            Patch(facecolor=PROTOCOL_COLORS['DBR'], edgecolor='black',
                  hatch=PROTOCOL_HATCHES['DBR'], label='DBR'),
            Patch(facecolor=PROTOCOL_COLORS['flooding'], edgecolor='black',
                  hatch=PROTOCOL_HATCHES['flooding'], label='Flooding')
        ]
        
        fig.legend(handles=legend_elements, loc='lower center', ncol=3,
                   fontsize=12, frameon=True, bbox_to_anchor=(0.5, 0.01))
        
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
    
    # Load and prepare data
    df = load_and_prepare_data(args.csv_file)
    
    # Generate visualization matrix
    generate_5x5_matrix(df, args.output_dir)
    
    print("Done! View charts in:", args.output_dir)


if __name__ == "__main__":
    main()

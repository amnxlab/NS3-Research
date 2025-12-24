#!/usr/bin/env python3
"""
UWSN V2 - Comprehensive Plotting System
========================================
Generates bar chart plots for all Y-axis vs X-axis combinations.

Y-axis variables (parameters):
- Number of nodes
- Depth variation
- Width (pipe) - VBF only
- Mobility
- Communication range

X-axis variables (metrics):
- Residual energy
- Energy efficiency (%)
- PDR (Packet Delivery Ratio)
- Average delay
- Network lifetime

Output: One plot per Y-axis vs X-axis combination = 25 plots
Naming: [original_filename]_[y_variable]_vs_[x_variable].png

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

# Color-blind friendly colors (distinguishable in grayscale)
PROTOCOL_COLORS = {
    'VBF': '#0072B2',       # Blue
    'DBR': '#D55E00',       # Vermillion/Orange
    'flooding': '#009E73'   # Bluish Green
}

# Hatching patterns for additional distinction
PROTOCOL_HATCHES = {
    'VBF': '',              # Solid
    'DBR': '///',           # Diagonal lines
    'flooding': 'xxx'       # Cross-hatch
}

# Set publication-quality defaults
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

# Y-axis variables (parameters) - what we vary
Y_AXIS_VARIABLES = [
    ('nodes', 'Number of Nodes', None),           # (column_name, display_label, protocol_filter)
    ('depth', 'Depth Variation', None),
    ('vbfWidth', 'VBF Width (m)', 'VBF'),         # VBF only
    ('mobility', 'Mobility Model', None),
    ('range', 'Communication Range (m)', None)
]

# X-axis variables (metrics) - what we measure
X_AXIS_VARIABLES = [
    ('total_residual_energy', 'Residual Energy (J)'),
    ('energy_efficiency_pct', 'Energy Efficiency (%)'),
    ('pdr_pct', 'PDR (%)'),
    ('avg_delay_ms', 'Average Delay (ms)'),
    ('network_lifetime_s', 'Network Lifetime (s)')
]


def load_and_prepare_data(csv_file):
    """Load CSV and compute derived metrics including fixes for flooding."""
    try:
        df = pd.read_csv(csv_file)
        print(f"Loaded {len(df)} rows from {csv_file}")
        print(f"Protocols: {list(df['protocol'].unique())}")
        
        # === FIX: Calculate PDR as percentage (0-100) ===
        if 'pdr' in df.columns:
            df['pdr_pct'] = df['pdr'] * 100
        else:
            df['pdr_pct'] = 0
        
        # === FIX: Calculate Energy Efficiency for ALL protocols including flooding ===
        # Energy Efficiency = (Packets Received / Total Energy Consumed) * 100
        if 'sink_recv' in df.columns and 'total_consumed_energy' in df.columns:
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
        
        # Print summary for verification
        print("\nData Summary per Protocol:")
        for protocol in sorted(df['protocol'].unique()):
            pdata = df[df['protocol'] == protocol]
            print(f"  {protocol}:")
            print(f"    PDR: {pdata['pdr_pct'].mean():.2f}%")
            print(f"    Energy Efficiency: {pdata['energy_efficiency_pct'].mean():.4f}%")
            print(f"    Residual Energy: {pdata['total_residual_energy'].mean():.2f} J")
        
        return df
        
    except Exception as e:
        print(f"ERROR loading {csv_file}: {e}")
        sys.exit(1)


def create_grouped_bar_plot(df, y_col, y_label, x_col, x_label, output_file, 
                            protocol_filter=None, csv_basename=""):
    """
    Create a grouped bar chart for one Y-axis variable vs one X-axis variable.
    
    Args:
        df: DataFrame with results
        y_col: Y-axis variable column name (e.g., 'nodes')
        y_label: Y-axis variable display label
        x_col: X-axis variable column name (e.g., 'pdr_pct')
        x_label: X-axis variable display label
        output_file: Path to save the plot
        protocol_filter: If set, only show this protocol (e.g., 'VBF' for vbfWidth)
        csv_basename: Base name of CSV file for title
    """
    # Get unique Y-axis values (e.g., nodes = 100, 150, 200)
    if y_col == 'depth':
        y_values = sorted(df['depth'].unique())
    elif y_col in df.columns:
        y_values = sorted(df[y_col].unique())
    else:
        print(f"  ⚠ Column '{y_col}' not found, skipping...")
        return False
    
    if not y_values:
        print(f"  ⚠ No values for {y_col}, skipping...")
        return False
    
    # Get protocols to plot
    if protocol_filter:
        protocols = [protocol_filter] if protocol_filter in df['protocol'].unique() else []
        if not protocols:
            print(f"  ⚠ Protocol {protocol_filter} not found for {y_col}, skipping...")
            return False
    else:
        protocols = sorted(df['protocol'].unique())
    
    # Create figure
    fig, ax = plt.subplots(figsize=(12, 7))
    
    # Calculate bar positions
    n_groups = len(y_values)
    n_bars = len(protocols)
    bar_width = 0.8 / n_bars
    
    # X positions for each group
    x_positions = np.arange(n_groups)
    
    # Plot bars for each protocol
    for p_idx, protocol in enumerate(protocols):
        means = []
        stds = []
        
        for y_val in y_values:
            # Filter data
            if y_col == 'depth':
                mask = (df['depth'] == y_val) & (df['protocol'] == protocol)
            else:
                mask = (df[y_col] == y_val) & (df['protocol'] == protocol)
            
            subset = df[mask]
            
            if len(subset) > 0:
                means.append(subset[x_col].mean())
                stds.append(subset[x_col].std() if len(subset) > 1 else 0)
            else:
                means.append(0)
                stds.append(0)
        
        # Calculate bar position offset
        offset = (p_idx - (n_bars - 1) / 2) * bar_width
        
        # Plot bars with error bars
        bars = ax.bar(x_positions + offset, means, bar_width,
                      yerr=stds if any(s > 0 for s in stds) else None,
                      label=protocol,
                      color=PROTOCOL_COLORS.get(protocol, 'gray'),
                      hatch=PROTOCOL_HATCHES.get(protocol, ''),
                      edgecolor='black', linewidth=1,
                      capsize=3, alpha=0.9)
    
    # Configure axes with SEPARATE, CLEAR labels
    ax.set_xlabel(y_label, fontsize=13, fontweight='bold', labelpad=10)
    ax.set_ylabel(x_label, fontsize=13, fontweight='bold', labelpad=10)
    
    # Set X-axis ticks to Y-variable values (sorted)
    ax.set_xticks(x_positions)
    ax.set_xticklabels([str(v) for v in y_values], fontsize=11, fontweight='bold')
    
    # Title
    title = f"{y_label} vs {x_label}"
    if protocol_filter:
        title += f" [{protocol_filter} only]"
    ax.set_title(title, fontsize=14, fontweight='bold', pad=15)
    
    # Grid
    ax.grid(axis='y', alpha=0.3, linestyle='--')
    ax.set_axisbelow(True)
    
    # Y-axis starts at 0
    ax.set_ylim(bottom=0)
    
    # Legend outside the plot (top right)
    ax.legend(title='Protocol', loc='upper right', fontsize=10,
              frameon=True, fancybox=True, shadow=True)
    
    plt.tight_layout()
    
    # Save
    plt.savefig(output_file, dpi=300, bbox_inches='tight', facecolor='white')
    plt.close()
    
    return True


def generate_all_plots(df, output_dir, csv_basename):
    """
    Generate all Y-axis vs X-axis combination plots.
    
    Total plots: 5 Y-variables × 5 X-variables = 25 plots
    """
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"\n{'='*70}")
    print(f"Generating All Plots")
    print(f"{'='*70}")
    print(f"Y-axis variables: {[y[1] for y in Y_AXIS_VARIABLES]}")
    print(f"X-axis variables: {[x[1] for x in X_AXIS_VARIABLES]}")
    print(f"Total plots to generate: {len(Y_AXIS_VARIABLES) * len(X_AXIS_VARIABLES)}")
    print(f"Output directory: {output_dir}")
    print(f"{'='*70}\n")
    
    plot_count = 0
    success_count = 0
    
    # Generate one plot for each Y vs X combination
    for y_col, y_label, protocol_filter in Y_AXIS_VARIABLES:
        for x_col, x_label in X_AXIS_VARIABLES:
            plot_count += 1
            
            # Generate filename: [csv_basename]_[y_var]_vs_[x_var].png
            y_name = y_col.lower().replace(' ', '_')
            x_name = x_col.lower().replace('_pct', '').replace('total_', '')
            filename = f"{csv_basename}_{y_name}_vs_{x_name}.png"
            output_file = output_dir / filename
            
            print(f"[{plot_count}/25] {y_label} vs {x_label}")
            
            success = create_grouped_bar_plot(
                df, y_col, y_label, x_col, x_label,
                output_file, protocol_filter, csv_basename
            )
            
            if success:
                print(f"  ✓ Saved: {filename}")
                success_count += 1
            else:
                print(f"  ✗ Skipped")
    
    print(f"\n{'='*70}")
    print(f"✓ Generated {success_count}/{plot_count} plots in {output_dir}/")
    print(f"{'='*70}\n")
    
    # Also generate summary 5x5 matrix
    generate_summary_matrix(df, output_dir, csv_basename)
    
    return success_count


def generate_summary_matrix(df, output_dir, csv_basename):
    """Generate a single 5x5 summary matrix figure."""
    
    print("Generating 5x5 summary matrix...")
    
    fig, axes = plt.subplots(5, 5, figsize=(28, 24))
    
    for row_idx, (y_col, y_label, protocol_filter) in enumerate(Y_AXIS_VARIABLES):
        # Get Y values
        if y_col == 'depth':
            y_values = sorted(df['depth'].unique())
        elif y_col in df.columns:
            y_values = sorted(df[y_col].unique())
        else:
            y_values = []
        
        for col_idx, (x_col, x_label) in enumerate(X_AXIS_VARIABLES):
            ax = axes[row_idx, col_idx]
            
            if not y_values:
                ax.text(0.5, 0.5, 'No Data', ha='center', va='center',
                        transform=ax.transAxes, fontsize=10)
                continue
            
            # Get protocols
            if protocol_filter:
                protocols = [protocol_filter] if protocol_filter in df['protocol'].unique() else []
            else:
                protocols = sorted(df['protocol'].unique())
            
            if not protocols:
                ax.text(0.5, 0.5, 'N/A', ha='center', va='center',
                        transform=ax.transAxes, fontsize=10)
                continue
            
            # Plot grouped bars
            n_groups = len(y_values)
            n_bars = len(protocols)
            bar_width = 0.8 / n_bars
            x_positions = np.arange(n_groups)
            
            for p_idx, protocol in enumerate(protocols):
                means = []
                for y_val in y_values:
                    if y_col == 'depth':
                        mask = (df['depth'] == y_val) & (df['protocol'] == protocol)
                    else:
                        mask = (df[y_col] == y_val) & (df['protocol'] == protocol)
                    subset = df[mask]
                    means.append(subset[x_col].mean() if len(subset) > 0 else 0)
                
                offset = (p_idx - (n_bars - 1) / 2) * bar_width
                ax.bar(x_positions + offset, means, bar_width,
                       color=PROTOCOL_COLORS.get(protocol, 'gray'),
                       hatch=PROTOCOL_HATCHES.get(protocol, ''),
                       edgecolor='black', linewidth=0.5, alpha=0.9)
            
            # Axis labels
            ax.set_xticks(x_positions)
            ax.set_xticklabels([str(v) for v in y_values], fontsize=8)
            ax.grid(axis='y', alpha=0.3, linestyle='--')
            ax.set_ylim(bottom=0)
            
            # Column header (X-axis metric) - first row only
            if row_idx == 0:
                ax.set_title(x_label, fontsize=11, fontweight='bold', pad=8)
            
            # Row header (Y-axis parameter) - first column only
            if col_idx == 0:
                label = y_label
                if protocol_filter:
                    label += f'\n[{protocol_filter}]'
                ax.set_ylabel(label, fontsize=10, fontweight='bold')
    
    # Main title
    fig.suptitle(f'UWSN Protocol Comparison Summary\n{csv_basename}',
                 fontsize=16, fontweight='bold', y=0.98)
    
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
               fontsize=12, frameon=True, bbox_to_anchor=(0.5, 0.01))
    
    plt.tight_layout(rect=[0, 0.04, 1, 0.96])
    
    output_file = output_dir / f"{csv_basename}_summary_5x5_matrix.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight', facecolor='white')
    plt.close()
    print(f"✓ Saved: {output_file}")


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
    
    # Load and prepare data
    df = load_and_prepare_data(args.csv_file)
    
    # Generate all plots
    generate_all_plots(df, args.output_dir, csv_basename)
    
    print(f"\nDone! All plots saved to: {args.output_dir}")
    print(f"Naming convention: {csv_basename}_[y_variable]_vs_[x_variable].png")


if __name__ == "__main__":
    main()

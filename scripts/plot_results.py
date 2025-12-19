#!/usr/bin/env python3
"""
UWSN Protocol Comparison - Visualization
=========================================
Generates publication-quality graphs from simulation results.

Usage:
    python plot_results.py results.csv [--output-dir graphs/]
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from scipy import stats
import argparse
from pathlib import Path
import sys

# Set publication-quality defaults
plt.rcParams.update({
    'font.size': 12,
    'axes.labelsize': 14,
    'axes.titlesize': 14,
    'xtick.labelsize': 12,
    'ytick.labelsize': 12,
    'legend.fontsize': 11,
    'figure.figsize': (8, 6),
    'figure.dpi': 150,
    'savefig.dpi': 300,
    'savefig.bbox': 'tight',
    'axes.grid': True,
    'grid.alpha': 0.3,
    'lines.linewidth': 2,
    'lines.markersize': 8,
})

# Color palette for protocols
PROTOCOL_COLORS = {
    'VBF': '#2ecc71',       # Green
    'DBR': '#3498db',       # Blue
    'flooding': '#e74c3c',  # Red
}

PROTOCOL_MARKERS = {
    'VBF': 'o',
    'DBR': 's',
    'flooding': '^',
}


def load_data(csv_path):
    """Load simulation results."""
    if not Path(csv_path).exists():
        print(f"ERROR: File not found: {csv_path}")
        sys.exit(1)
    
    df = pd.read_csv(csv_path)
    print(f"Loaded {len(df)} records")
    return df


def compute_stats(df, group_cols, metric):
    """Compute statistics for plotting."""
    stats_df = df.groupby(group_cols)[metric].agg([
        'mean', 'std', 'count'
    ]).reset_index()
    
    # 95% confidence interval
    stats_df['ci'] = 1.96 * stats_df['std'] / np.sqrt(stats_df['count'])
    
    return stats_df


def plot_bar_comparison(df, metric, ylabel, title, output_path, figsize=(10, 6)):
    """Create bar chart comparing protocols with error bars."""
    fig, ax = plt.subplots(figsize=figsize)
    
    protocols = df['protocol'].unique()
    nodes = sorted(df['nodes'].unique())
    x = np.arange(len(nodes))
    width = 0.25
    
    for i, protocol in enumerate(protocols):
        proto_df = df[df['protocol'] == protocol]
        stats = compute_stats(proto_df, ['nodes'], metric)
        
        means = []
        errors = []
        for n in nodes:
            row = stats[stats['nodes'] == n]
            if len(row) > 0:
                means.append(row['mean'].values[0])
                errors.append(row['ci'].values[0])
            else:
                means.append(0)
                errors.append(0)
        
        bars = ax.bar(
            x + i * width, 
            means, 
            width,
            yerr=errors,
            label=protocol,
            color=PROTOCOL_COLORS.get(protocol, f'C{i}'),
            capsize=4,
            error_kw={'linewidth': 1.5}
        )
    
    ax.set_xlabel('Number of Nodes')
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.set_xticks(x + width)
    ax.set_xticklabels(nodes)
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(output_path)
    plt.close()
    print(f"Saved: {output_path}")


def plot_line_comparison(df, metric, ylabel, title, output_path, figsize=(10, 6)):
    """Create line graph showing metric vs nodes."""
    fig, ax = plt.subplots(figsize=figsize)
    
    protocols = df['protocol'].unique()
    
    for protocol in protocols:
        proto_df = df[df['protocol'] == protocol]
        stats = compute_stats(proto_df, ['nodes'], metric)
        stats = stats.sort_values('nodes')
        
        ax.errorbar(
            stats['nodes'],
            stats['mean'],
            yerr=stats['ci'],
            label=protocol,
            color=PROTOCOL_COLORS.get(protocol),
            marker=PROTOCOL_MARKERS.get(protocol, 'o'),
            markersize=8,
            capsize=4,
            linewidth=2
        )
    
    ax.set_xlabel('Number of Nodes')
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(output_path)
    plt.close()
    print(f"Saved: {output_path}")


def plot_boxplot(df, metric, ylabel, title, output_path, figsize=(10, 6)):
    """Create box plot showing distribution across runs."""
    fig, ax = plt.subplots(figsize=figsize)
    
    # Prepare data for seaborn
    protocols = df['protocol'].unique()
    palette = {p: PROTOCOL_COLORS.get(p, f'C{i}') for i, p in enumerate(protocols)}
    
    sns.boxplot(
        data=df,
        x='nodes',
        y=metric,
        hue='protocol',
        palette=palette,
        ax=ax
    )
    
    ax.set_xlabel('Number of Nodes')
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.legend(title='Protocol')
    ax.grid(True, alpha=0.3, axis='y')
    
    plt.tight_layout()
    plt.savefig(output_path)
    plt.close()
    print(f"Saved: {output_path}")


def plot_cdf(df, metric, xlabel, title, output_path, figsize=(10, 6)):
    """Create CDF plot for delay distribution."""
    fig, ax = plt.subplots(figsize=figsize)
    
    protocols = df['protocol'].unique()
    
    for protocol in protocols:
        proto_df = df[df['protocol'] == protocol]
        values = proto_df[metric].dropna().sort_values()
        
        if len(values) > 0:
            cdf = np.arange(1, len(values) + 1) / len(values)
            ax.plot(
                values, cdf,
                label=protocol,
                color=PROTOCOL_COLORS.get(protocol),
                linewidth=2
            )
    
    ax.set_xlabel(xlabel)
    ax.set_ylabel('CDF')
    ax.set_title(title)
    ax.legend()
    ax.grid(True, alpha=0.3)
    ax.set_ylim(0, 1)
    
    plt.tight_layout()
    plt.savefig(output_path)
    plt.close()
    print(f"Saved: {output_path}")


def plot_energy_breakdown(df, output_path, figsize=(12, 5)):
    """Create energy consumption breakdown chart."""
    fig, axes = plt.subplots(1, 2, figsize=figsize)
    
    protocols = df['protocol'].unique()
    
    # Total energy consumed
    ax1 = axes[0]
    stats = compute_stats(df, ['protocol'], 'total_consumed_energy')
    colors = [PROTOCOL_COLORS.get(p, f'C{i}') for i, p in enumerate(stats['protocol'])]
    
    bars = ax1.bar(
        stats['protocol'],
        stats['mean'],
        yerr=stats['ci'],
        color=colors,
        capsize=5
    )
    ax1.set_ylabel('Total Energy Consumed (J)')
    ax1.set_title('Total Energy Consumption')
    ax1.grid(True, alpha=0.3, axis='y')
    
    # Energy efficiency
    ax2 = axes[1]
    if 'energy_efficiency_pkts_per_j' in df.columns:
        stats = compute_stats(df, ['protocol'], 'energy_efficiency_pkts_per_j')
        colors = [PROTOCOL_COLORS.get(p, f'C{i}') for i, p in enumerate(stats['protocol'])]
        
        bars = ax2.bar(
            stats['protocol'],
            stats['mean'],
            yerr=stats['ci'],
            color=colors,
            capsize=5
        )
        ax2.set_ylabel('Packets per Joule')
        ax2.set_title('Energy Efficiency')
        ax2.grid(True, alpha=0.3, axis='y')
    
    plt.tight_layout()
    plt.savefig(output_path)
    plt.close()
    print(f"Saved: {output_path}")


def plot_network_lifetime(df, output_path, figsize=(10, 6)):
    """Create network lifetime comparison chart."""
    fig, ax = plt.subplots(figsize=figsize)
    
    if 'network_lifetime_s' not in df.columns:
        print("Warning: network_lifetime_s not in data")
        return
    
    protocols = df['protocol'].unique()
    nodes = sorted(df['nodes'].unique())
    x = np.arange(len(nodes))
    width = 0.25
    
    for i, protocol in enumerate(protocols):
        proto_df = df[df['protocol'] == protocol]
        stats = compute_stats(proto_df, ['nodes'], 'network_lifetime_s')
        
        means = []
        errors = []
        for n in nodes:
            row = stats[stats['nodes'] == n]
            if len(row) > 0:
                means.append(row['mean'].values[0])
                errors.append(row['ci'].values[0])
            else:
                means.append(0)
                errors.append(0)
        
        bars = ax.bar(
            x + i * width,
            means,
            width,
            yerr=errors,
            label=protocol,
            color=PROTOCOL_COLORS.get(protocol, f'C{i}'),
            capsize=4
        )
    
    ax.set_xlabel('Number of Nodes')
    ax.set_ylabel('Network Lifetime (seconds)')
    ax.set_title('Network Lifetime Comparison')
    ax.set_xticks(x + width)
    ax.set_xticklabels(nodes)
    ax.legend()
    ax.grid(True, alpha=0.3, axis='y')
    
    plt.tight_layout()
    plt.savefig(output_path)
    plt.close()
    print(f"Saved: {output_path}")


def plot_heatmap(df, metric, title, output_path, figsize=(10, 8)):
    """Create heatmap showing metric across protocols and node counts."""
    fig, ax = plt.subplots(figsize=figsize)
    
    pivot_df = df.pivot_table(
        values=metric,
        index='nodes',
        columns='protocol',
        aggfunc='mean'
    )
    
    sns.heatmap(
        pivot_df,
        annot=True,
        fmt='.3f',
        cmap='YlOrRd',
        ax=ax,
        cbar_kws={'label': metric}
    )
    
    ax.set_title(title)
    ax.set_xlabel('Protocol')
    ax.set_ylabel('Number of Nodes')
    
    plt.tight_layout()
    plt.savefig(output_path)
    plt.close()
    print(f"Saved: {output_path}")


def plot_summary_dashboard(df, output_path, figsize=(16, 12)):
    """Create a summary dashboard with multiple metrics."""
    fig, axes = plt.subplots(2, 2, figsize=figsize)
    
    protocols = df['protocol'].unique()
    nodes = sorted(df['nodes'].unique())
    
    # PDR
    ax = axes[0, 0]
    for protocol in protocols:
        proto_df = df[df['protocol'] == protocol]
        stats = compute_stats(proto_df, ['nodes'], 'pdr')
        stats = stats.sort_values('nodes')
        ax.errorbar(
            stats['nodes'], stats['mean'] * 100,
            yerr=stats['ci'] * 100,
            label=protocol,
            color=PROTOCOL_COLORS.get(protocol),
            marker=PROTOCOL_MARKERS.get(protocol),
            capsize=3
        )
    ax.set_xlabel('Number of Nodes')
    ax.set_ylabel('PDR (%)')
    ax.set_title('Packet Delivery Ratio')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    # Delay
    ax = axes[0, 1]
    if 'avg_delay_ms' in df.columns:
        for protocol in protocols:
            proto_df = df[df['protocol'] == protocol]
            stats = compute_stats(proto_df, ['nodes'], 'avg_delay_ms')
            stats = stats.sort_values('nodes')
            ax.errorbar(
                stats['nodes'], stats['mean'],
                yerr=stats['ci'],
                label=protocol,
                color=PROTOCOL_COLORS.get(protocol),
                marker=PROTOCOL_MARKERS.get(protocol),
                capsize=3
            )
    ax.set_xlabel('Number of Nodes')
    ax.set_ylabel('Average Delay (ms)')
    ax.set_title('End-to-End Delay')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    # Energy
    ax = axes[1, 0]
    for protocol in protocols:
        proto_df = df[df['protocol'] == protocol]
        stats = compute_stats(proto_df, ['nodes'], 'total_consumed_energy')
        stats = stats.sort_values('nodes')
        ax.errorbar(
            stats['nodes'], stats['mean'],
            yerr=stats['ci'],
            label=protocol,
            color=PROTOCOL_COLORS.get(protocol),
            marker=PROTOCOL_MARKERS.get(protocol),
            capsize=3
        )
    ax.set_xlabel('Number of Nodes')
    ax.set_ylabel('Energy Consumed (J)')
    ax.set_title('Total Energy Consumption')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    # Throughput
    ax = axes[1, 1]
    if 'throughput_bps' in df.columns:
        for protocol in protocols:
            proto_df = df[df['protocol'] == protocol]
            stats = compute_stats(proto_df, ['nodes'], 'throughput_bps')
            stats = stats.sort_values('nodes')
            ax.errorbar(
                stats['nodes'], stats['mean'] / 1000,  # Convert to kbps
                yerr=stats['ci'] / 1000,
                label=protocol,
                color=PROTOCOL_COLORS.get(protocol),
                marker=PROTOCOL_MARKERS.get(protocol),
                capsize=3
            )
    ax.set_xlabel('Number of Nodes')
    ax.set_ylabel('Throughput (kbps)')
    ax.set_title('Network Throughput')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    plt.suptitle('UWSN Protocol Comparison Summary', fontsize=16, fontweight='bold')
    plt.tight_layout()
    plt.savefig(output_path)
    plt.close()
    print(f"Saved: {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description="UWSN Protocol Comparison - Visualization"
    )
    parser.add_argument(
        "input", type=str,
        help="Input CSV file with simulation results"
    )
    parser.add_argument(
        "--output-dir", type=str, default="graphs",
        help="Output directory for graphs (default: graphs/)"
    )
    parser.add_argument(
        "--format", type=str, default="pdf",
        choices=['pdf', 'png', 'svg'],
        help="Output format (default: pdf)"
    )
    
    args = parser.parse_args()
    
    # Create output directory
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Load data
    df = load_data(args.input)
    
    fmt = args.format
    
    print("\nGenerating visualizations...")
    print("=" * 60)
    
    # Summary dashboard
    plot_summary_dashboard(
        df,
        output_dir / f"summary_dashboard.{fmt}"
    )
    
    # PDR comparison
    plot_bar_comparison(
        df, 'pdr', 'Packet Delivery Ratio',
        'PDR Comparison by Protocol and Node Count',
        output_dir / f"pdr_bar.{fmt}"
    )
    
    plot_line_comparison(
        df, 'pdr', 'Packet Delivery Ratio',
        'PDR vs Number of Nodes',
        output_dir / f"pdr_line.{fmt}"
    )
    
    plot_boxplot(
        df, 'pdr', 'Packet Delivery Ratio',
        'PDR Distribution',
        output_dir / f"pdr_boxplot.{fmt}"
    )
    
    # Delay
    if 'avg_delay_ms' in df.columns:
        plot_line_comparison(
            df, 'avg_delay_ms', 'Average Delay (ms)',
            'End-to-End Delay vs Number of Nodes',
            output_dir / f"delay_line.{fmt}"
        )
        
        plot_boxplot(
            df, 'avg_delay_ms', 'Average Delay (ms)',
            'Delay Distribution',
            output_dir / f"delay_boxplot.{fmt}"
        )
        
        plot_cdf(
            df, 'avg_delay_ms', 'Delay (ms)',
            'CDF of End-to-End Delay',
            output_dir / f"delay_cdf.{fmt}"
        )
    
    # Energy
    plot_energy_breakdown(
        df,
        output_dir / f"energy_breakdown.{fmt}"
    )
    
    plot_line_comparison(
        df, 'total_consumed_energy', 'Energy Consumed (J)',
        'Energy Consumption vs Number of Nodes',
        output_dir / f"energy_line.{fmt}"
    )
    
    plot_boxplot(
        df, 'total_consumed_energy', 'Energy Consumed (J)',
        'Energy Consumption Distribution',
        output_dir / f"energy_boxplot.{fmt}"
    )
    
    # Network lifetime
    plot_network_lifetime(
        df,
        output_dir / f"network_lifetime.{fmt}"
    )
    
    # Throughput
    if 'throughput_bps' in df.columns:
        plot_line_comparison(
            df, 'throughput_bps', 'Throughput (bps)',
            'Throughput vs Number of Nodes',
            output_dir / f"throughput_line.{fmt}"
        )
    
    # Heatmap
    plot_heatmap(
        df, 'pdr',
        'PDR Heatmap',
        output_dir / f"pdr_heatmap.{fmt}"
    )
    
    print("\n" + "=" * 60)
    print("  VISUALIZATION COMPLETE")
    print("=" * 60)
    print(f"All graphs saved to: {output_dir}/")
    print(f"\nGenerated files:")
    for f in sorted(output_dir.glob(f"*.{fmt}")):
        print(f"  - {f.name}")


if __name__ == "__main__":
    main()


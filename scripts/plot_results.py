#!/usr/bin/env python3
"""
UWSN Protocol Comparison - Results Plotter
==========================================
Generates publication-quality graphs from simulation results.

Usage:
    python plot_results.py results.csv
    python plot_results.py results.csv --format png --output-dir ./graphs
"""

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import argparse
import sys
from pathlib import Path

# Set style for publication-quality figures
plt.style.use('seaborn-v0_8-whitegrid')
sns.set_palette("husl")

# Color scheme for protocols
PROTOCOL_COLORS = {
    'VBF': '#2ecc71',      # Green
    'DBR': '#3498db',      # Blue
    'flooding': '#e74c3c'  # Red
}

# Markers for line plots
PROTOCOL_MARKERS = {
    'VBF': 'o',
    'DBR': 's',
    'flooding': '^'
}


def load_data(csv_file):
    """Load and validate CSV data."""
    try:
        df = pd.read_csv(csv_file)
        print(f"Loaded {len(df)} rows from {csv_file}")
        print(f"Columns: {list(df.columns)}")
        print(f"Protocols: {df['protocol'].unique()}")
        print(f"Node counts: {sorted(df['nodes'].unique())}")
        return df
    except Exception as e:
        print(f"ERROR loading {csv_file}: {e}")
        sys.exit(1)


def plot_pdr_bar(df, output_dir, fmt):
    """Bar chart comparing PDR across protocols."""
    fig, ax = plt.subplots(figsize=(10, 6))
    
    # Group by protocol and nodes
    grouped = df.groupby(['protocol', 'nodes'])['pdr'].agg(['mean', 'std']).reset_index()
    
    protocols = df['protocol'].unique()
    nodes_list = sorted(df['nodes'].unique())
    x = range(len(nodes_list))
    width = 0.25
    
    for i, protocol in enumerate(protocols):
        data = grouped[grouped['protocol'] == protocol]
        means = [data[data['nodes'] == n]['mean'].values[0] * 100 if len(data[data['nodes'] == n]) > 0 else 0 
                 for n in nodes_list]
        stds = [data[data['nodes'] == n]['std'].values[0] * 100 if len(data[data['nodes'] == n]) > 0 else 0 
                for n in nodes_list]
        
        bars = ax.bar([xi + i * width for xi in x], means, width, 
                      label=protocol, yerr=stds, capsize=3,
                      color=PROTOCOL_COLORS.get(protocol, f'C{i}'))
    
    ax.set_xlabel('Number of Nodes', fontsize=12)
    ax.set_ylabel('Packet Delivery Ratio (%)', fontsize=12)
    ax.set_title('PDR Comparison Across Protocols', fontsize=14, fontweight='bold')
    ax.set_xticks([xi + width for xi in x])
    ax.set_xticklabels(nodes_list)
    ax.legend(title='Protocol')
    ax.set_ylim(0, 100)
    
    plt.tight_layout()
    output_file = output_dir / f'pdr_bar.{fmt}'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Saved: {output_file}")


def plot_pdr_line(df, output_dir, fmt):
    """Line chart showing PDR vs node count."""
    fig, ax = plt.subplots(figsize=(10, 6))
    
    for protocol in df['protocol'].unique():
        data = df[df['protocol'] == protocol].groupby('nodes')['pdr'].agg(['mean', 'std']).reset_index()
        ax.errorbar(data['nodes'], data['mean'] * 100, yerr=data['std'] * 100,
                   label=protocol, marker=PROTOCOL_MARKERS.get(protocol, 'o'),
                   color=PROTOCOL_COLORS.get(protocol), capsize=3, linewidth=2, markersize=8)
    
    ax.set_xlabel('Number of Nodes', fontsize=12)
    ax.set_ylabel('Packet Delivery Ratio (%)', fontsize=12)
    ax.set_title('PDR Scalability Analysis', fontsize=14, fontweight='bold')
    ax.legend(title='Protocol')
    ax.set_ylim(0, 100)
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    output_file = output_dir / f'pdr_line.{fmt}'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Saved: {output_file}")


def plot_pdr_boxplot(df, output_dir, fmt):
    """Box plot showing PDR distribution."""
    fig, ax = plt.subplots(figsize=(12, 6))
    
    df_plot = df.copy()
    df_plot['pdr_percent'] = df_plot['pdr'] * 100
    
    sns.boxplot(data=df_plot, x='nodes', y='pdr_percent', hue='protocol', ax=ax,
                palette=PROTOCOL_COLORS)
    
    ax.set_xlabel('Number of Nodes', fontsize=12)
    ax.set_ylabel('Packet Delivery Ratio (%)', fontsize=12)
    ax.set_title('PDR Distribution by Protocol and Node Count', fontsize=14, fontweight='bold')
    ax.legend(title='Protocol')
    
    plt.tight_layout()
    output_file = output_dir / f'pdr_boxplot.{fmt}'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Saved: {output_file}")


def plot_energy_line(df, output_dir, fmt):
    """Line chart showing energy consumption vs node count."""
    fig, ax = plt.subplots(figsize=(10, 6))
    
    for protocol in df['protocol'].unique():
        data = df[df['protocol'] == protocol].groupby('nodes')['avg_energy_per_node'].agg(['mean', 'std']).reset_index()
        ax.errorbar(data['nodes'], data['mean'], yerr=data['std'],
                   label=protocol, marker=PROTOCOL_MARKERS.get(protocol, 'o'),
                   color=PROTOCOL_COLORS.get(protocol), capsize=3, linewidth=2, markersize=8)
    
    ax.set_xlabel('Number of Nodes', fontsize=12)
    ax.set_ylabel('Average Energy per Node (J)', fontsize=12)
    ax.set_title('Energy Consumption Scalability', fontsize=14, fontweight='bold')
    ax.legend(title='Protocol')
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    output_file = output_dir / f'energy_line.{fmt}'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Saved: {output_file}")


def plot_energy_boxplot(df, output_dir, fmt):
    """Box plot showing energy distribution."""
    fig, ax = plt.subplots(figsize=(12, 6))
    
    sns.boxplot(data=df, x='nodes', y='avg_energy_per_node', hue='protocol', ax=ax,
                palette=PROTOCOL_COLORS)
    
    ax.set_xlabel('Number of Nodes', fontsize=12)
    ax.set_ylabel('Average Energy per Node (J)', fontsize=12)
    ax.set_title('Energy Consumption Distribution', fontsize=14, fontweight='bold')
    ax.legend(title='Protocol')
    
    plt.tight_layout()
    output_file = output_dir / f'energy_boxplot.{fmt}'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Saved: {output_file}")


def plot_network_lifetime(df, output_dir, fmt):
    """Bar chart comparing network lifetime."""
    fig, ax = plt.subplots(figsize=(10, 6))
    
    grouped = df.groupby(['protocol', 'nodes'])['network_lifetime_s'].agg(['mean', 'std']).reset_index()
    
    protocols = df['protocol'].unique()
    nodes_list = sorted(df['nodes'].unique())
    x = range(len(nodes_list))
    width = 0.25
    
    for i, protocol in enumerate(protocols):
        data = grouped[grouped['protocol'] == protocol]
        means = [data[data['nodes'] == n]['mean'].values[0] if len(data[data['nodes'] == n]) > 0 else 0 
                 for n in nodes_list]
        stds = [data[data['nodes'] == n]['std'].values[0] if len(data[data['nodes'] == n]) > 0 else 0 
                for n in nodes_list]
        
        ax.bar([xi + i * width for xi in x], means, width,
               label=protocol, yerr=stds, capsize=3,
               color=PROTOCOL_COLORS.get(protocol, f'C{i}'))
    
    ax.set_xlabel('Number of Nodes', fontsize=12)
    ax.set_ylabel('Network Lifetime (s)', fontsize=12)
    ax.set_title('Network Lifetime Comparison', fontsize=14, fontweight='bold')
    ax.set_xticks([xi + width for xi in x])
    ax.set_xticklabels(nodes_list)
    ax.legend(title='Protocol')
    
    plt.tight_layout()
    output_file = output_dir / f'network_lifetime.{fmt}'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Saved: {output_file}")


def plot_dead_nodes(df, output_dir, fmt):
    """Bar chart showing dead nodes at simulation end."""
    fig, ax = plt.subplots(figsize=(10, 6))
    
    grouped = df.groupby(['protocol', 'nodes'])['dead_nodes'].agg(['mean', 'std']).reset_index()
    
    protocols = df['protocol'].unique()
    nodes_list = sorted(df['nodes'].unique())
    x = range(len(nodes_list))
    width = 0.25
    
    for i, protocol in enumerate(protocols):
        data = grouped[grouped['protocol'] == protocol]
        means = [data[data['nodes'] == n]['mean'].values[0] if len(data[data['nodes'] == n]) > 0 else 0 
                 for n in nodes_list]
        stds = [data[data['nodes'] == n]['std'].values[0] if len(data[data['nodes'] == n]) > 0 else 0 
                for n in nodes_list]
        
        ax.bar([xi + i * width for xi in x], means, width,
               label=protocol, yerr=stds, capsize=3,
               color=PROTOCOL_COLORS.get(protocol, f'C{i}'))
    
    ax.set_xlabel('Number of Nodes', fontsize=12)
    ax.set_ylabel('Dead Nodes', fontsize=12)
    ax.set_title('Node Failures by Protocol', fontsize=14, fontweight='bold')
    ax.set_xticks([xi + width for xi in x])
    ax.set_xticklabels(nodes_list)
    ax.legend(title='Protocol')
    
    plt.tight_layout()
    output_file = output_dir / f'dead_nodes.{fmt}'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Saved: {output_file}")


def plot_total_energy(df, output_dir, fmt):
    """Line chart showing total energy consumption."""
    fig, ax = plt.subplots(figsize=(10, 6))
    
    for protocol in df['protocol'].unique():
        data = df[df['protocol'] == protocol].groupby('nodes')['total_consumed_energy'].agg(['mean', 'std']).reset_index()
        ax.errorbar(data['nodes'], data['mean'], yerr=data['std'],
                   label=protocol, marker=PROTOCOL_MARKERS.get(protocol, 'o'),
                   color=PROTOCOL_COLORS.get(protocol), capsize=3, linewidth=2, markersize=8)
    
    ax.set_xlabel('Number of Nodes', fontsize=12)
    ax.set_ylabel('Total Energy Consumed (J)', fontsize=12)
    ax.set_title('Total Network Energy Consumption', fontsize=14, fontweight='bold')
    ax.legend(title='Protocol')
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    output_file = output_dir / f'total_energy.{fmt}'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Saved: {output_file}")


def plot_sent_vs_received(df, output_dir, fmt):
    """Scatter plot showing sent vs received packets."""
    fig, ax = plt.subplots(figsize=(10, 6))
    
    for protocol in df['protocol'].unique():
        data = df[df['protocol'] == protocol]
        ax.scatter(data['sent_pkts'], data['recv_pkts'], 
                   label=protocol, alpha=0.6, s=50,
                   color=PROTOCOL_COLORS.get(protocol),
                   marker=PROTOCOL_MARKERS.get(protocol, 'o'))
    
    # Add diagonal line for reference (100% PDR)
    max_val = max(df['sent_pkts'].max(), df['recv_pkts'].max())
    ax.plot([0, max_val], [0, max_val], 'k--', alpha=0.3, label='100% PDR')
    
    ax.set_xlabel('Packets Sent', fontsize=12)
    ax.set_ylabel('Packets Received', fontsize=12)
    ax.set_title('Packet Delivery Analysis', fontsize=14, fontweight='bold')
    ax.legend(title='Protocol')
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    output_file = output_dir / f'sent_vs_received.{fmt}'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Saved: {output_file}")


def plot_summary_dashboard(df, output_dir, fmt):
    """Create a 2x2 summary dashboard."""
    fig, axes = plt.subplots(2, 2, figsize=(14, 12))
    
    # PDR comparison
    ax = axes[0, 0]
    for protocol in df['protocol'].unique():
        data = df[df['protocol'] == protocol].groupby('nodes')['pdr'].mean().reset_index()
        ax.plot(data['nodes'], data['pdr'] * 100, 
                marker=PROTOCOL_MARKERS.get(protocol, 'o'),
                color=PROTOCOL_COLORS.get(protocol),
                label=protocol, linewidth=2, markersize=8)
    ax.set_xlabel('Number of Nodes')
    ax.set_ylabel('PDR (%)')
    ax.set_title('Packet Delivery Ratio', fontweight='bold')
    ax.legend()
    ax.grid(True, alpha=0.3)
    ax.set_ylim(0, 100)
    
    # Energy consumption
    ax = axes[0, 1]
    for protocol in df['protocol'].unique():
        data = df[df['protocol'] == protocol].groupby('nodes')['avg_energy_per_node'].mean().reset_index()
        ax.plot(data['nodes'], data['mean'] if 'mean' in data.columns else data['avg_energy_per_node'],
                marker=PROTOCOL_MARKERS.get(protocol, 'o'),
                color=PROTOCOL_COLORS.get(protocol),
                label=protocol, linewidth=2, markersize=8)
    ax.set_xlabel('Number of Nodes')
    ax.set_ylabel('Energy per Node (J)')
    ax.set_title('Average Energy Consumption', fontweight='bold')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    # Network lifetime
    ax = axes[1, 0]
    grouped = df.groupby(['protocol', 'nodes'])['network_lifetime_s'].mean().unstack(level=0)
    grouped.plot(kind='bar', ax=ax, color=[PROTOCOL_COLORS.get(p, f'C{i}') for i, p in enumerate(grouped.columns)])
    ax.set_xlabel('Number of Nodes')
    ax.set_ylabel('Network Lifetime (s)')
    ax.set_title('Network Lifetime', fontweight='bold')
    ax.legend(title='Protocol')
    ax.tick_params(axis='x', rotation=0)
    
    # Dead nodes
    ax = axes[1, 1]
    grouped = df.groupby(['protocol', 'nodes'])['dead_nodes'].mean().unstack(level=0)
    grouped.plot(kind='bar', ax=ax, color=[PROTOCOL_COLORS.get(p, f'C{i}') for i, p in enumerate(grouped.columns)])
    ax.set_xlabel('Number of Nodes')
    ax.set_ylabel('Dead Nodes')
    ax.set_title('Node Failures', fontweight='bold')
    ax.legend(title='Protocol')
    ax.tick_params(axis='x', rotation=0)
    
    plt.suptitle('UWSN Protocol Comparison Summary', fontsize=16, fontweight='bold', y=1.02)
    plt.tight_layout()
    
    output_file = output_dir / f'summary_dashboard.{fmt}'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Saved: {output_file}")


def main():
    parser = argparse.ArgumentParser(
        description="UWSN Protocol Comparison - Results Plotter"
    )
    parser.add_argument(
        "csv_file",
        help="Path to the results CSV file"
    )
    parser.add_argument(
        "--output-dir", "-o",
        default="./graphs",
        help="Output directory for graphs (default: ./graphs)"
    )
    parser.add_argument(
        "--format", "-f",
        choices=["png", "pdf", "svg"],
        default="png",
        help="Output format (default: png)"
    )
    
    args = parser.parse_args()
    
    # Create output directory
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Load data
    df = load_data(args.csv_file)
    
    print(f"\nGenerating {args.format.upper()} graphs in {output_dir}/")
    print("=" * 50)
    
    # Generate all plots
    plot_pdr_bar(df, output_dir, args.format)
    plot_pdr_line(df, output_dir, args.format)
    plot_pdr_boxplot(df, output_dir, args.format)
    plot_energy_line(df, output_dir, args.format)
    plot_energy_boxplot(df, output_dir, args.format)
    plot_network_lifetime(df, output_dir, args.format)
    plot_dead_nodes(df, output_dir, args.format)
    plot_total_energy(df, output_dir, args.format)
    plot_sent_vs_received(df, output_dir, args.format)
    plot_summary_dashboard(df, output_dir, args.format)
    
    print("=" * 50)
    print(f"Generated 10 graphs in {output_dir}/")


if __name__ == "__main__":
    main()

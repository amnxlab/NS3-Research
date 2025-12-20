#!/usr/bin/env python3
"""
UWSN Protocol Comparison - Results Plotter
==========================================
Generates meaningful, publication-quality graphs from simulation results.

Usage:
    python3 plot_results.py results.csv
    python3 plot_results.py results.csv --format png --output-dir ./graphs
"""

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
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
        print(f"Protocols: {df['protocol'].unique()}")
        print(f"Node counts: {sorted(df['nodes'].unique())}")
        print(f"Runs per config: {df.groupby(['protocol', 'nodes']).size().min()}")
        return df
    except Exception as e:
        print(f"ERROR loading {csv_file}: {e}")
        sys.exit(1)


def plot_protocol_comparison(df, output_dir, fmt):
    """
    MOST IMPORTANT: Bar chart comparing protocols across all metrics.
    Shows PDR, Energy, and Throughput side-by-side.
    """
    fig, axes = plt.subplots(1, 3, figsize=(16, 5))
    
    protocols = sorted(df['protocol'].unique())
    
    # 1. PDR Comparison
    ax = axes[0]
    pdr_data = df.groupby('protocol')['pdr'].agg(['mean', 'std']).reset_index()
    x_pos = np.arange(len(protocols))
    
    bars = ax.bar(x_pos, pdr_data['mean'] * 100, 
                  yerr=pdr_data['std'] * 100,
                  color=[PROTOCOL_COLORS.get(p, 'gray') for p in protocols],
                  capsize=5, alpha=0.8, edgecolor='black', linewidth=1.2)
    
    ax.set_xlabel('Protocol', fontsize=12, fontweight='bold')
    ax.set_ylabel('Packet Delivery Ratio (%)', fontsize=12, fontweight='bold')
    ax.set_title('PDR Performance', fontsize=13, fontweight='bold')
    ax.set_xticks(x_pos)
    ax.set_xticklabels(protocols, fontsize=11)
    ax.set_ylim(0, 110)
    ax.grid(axis='y', alpha=0.3)
    
    # Add value labels on bars
    for i, bar in enumerate(bars):
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height + 2,
                f'{height:.1f}%', ha='center', va='bottom', fontweight='bold')
    
    # 2. Energy Efficiency
    ax = axes[1]
    energy_data = df.groupby('protocol')['avg_energy_per_node'].agg(['mean', 'std']).reset_index()
    
    bars = ax.bar(x_pos, energy_data['mean'],
                  yerr=energy_data['std'],
                  color=[PROTOCOL_COLORS.get(p, 'gray') for p in protocols],
                  capsize=5, alpha=0.8, edgecolor='black', linewidth=1.2)
    
    ax.set_xlabel('Protocol', fontsize=12, fontweight='bold')
    ax.set_ylabel('Energy per Node (J)', fontsize=12, fontweight='bold')
    ax.set_title('Energy Efficiency', fontsize=13, fontweight='bold')
    ax.set_xticks(x_pos)
    ax.set_xticklabels(protocols, fontsize=11)
    ax.grid(axis='y', alpha=0.3)
    
    # Add value labels
    for i, bar in enumerate(bars):
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.2f}J', ha='center', va='bottom', fontweight='bold')
    
    # 3. Throughput (packets received)
    ax = axes[2]
    throughput_data = df.groupby('protocol')['sink_recv'].agg(['mean', 'std']).reset_index()
    
    bars = ax.bar(x_pos, throughput_data['mean'],
                  yerr=throughput_data['std'],
                  color=[PROTOCOL_COLORS.get(p, 'gray') for p in protocols],
                  capsize=5, alpha=0.8, edgecolor='black', linewidth=1.2)
    
    ax.set_xlabel('Protocol', fontsize=12, fontweight='bold')
    ax.set_ylabel('Packets Received', fontsize=12, fontweight='bold')
    ax.set_title('Throughput', fontsize=13, fontweight='bold')
    ax.set_xticks(x_pos)
    ax.set_xticklabels(protocols, fontsize=11)
    ax.grid(axis='y', alpha=0.3)
    
    # Add value labels
    for i, bar in enumerate(bars):
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
                f'{int(height)}', ha='center', va='bottom', fontweight='bold')
    
    plt.suptitle('Protocol Performance Comparison', fontsize=16, fontweight='bold', y=1.02)
    plt.tight_layout()
    
    output_file = output_dir / f'1_protocol_comparison.{fmt}'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"✓ Saved: {output_file}")


def plot_scalability_analysis(df, output_dir, fmt):
    """
    SCALABILITY: How protocols perform as network size increases.
    Shows PDR and Energy vs Node Count.
    """
    fig, axes = plt.subplots(1, 2, figsize=(14, 5))
    
    # 1. PDR Scalability
    ax = axes[0]
    for protocol in sorted(df['protocol'].unique()):
        data = df[df['protocol'] == protocol].groupby('nodes')['pdr'].agg(['mean', 'std']).reset_index()
        ax.errorbar(data['nodes'], data['mean'] * 100, yerr=data['std'] * 100,
                   label=protocol, marker=PROTOCOL_MARKERS.get(protocol, 'o'),
                   color=PROTOCOL_COLORS.get(protocol), capsize=4, linewidth=2.5, 
                   markersize=10, markeredgewidth=1.5, markeredgecolor='white')
    
    ax.set_xlabel('Number of Nodes', fontsize=12, fontweight='bold')
    ax.set_ylabel('Packet Delivery Ratio (%)', fontsize=12, fontweight='bold')
    ax.set_title('PDR vs Network Size', fontsize=13, fontweight='bold')
    ax.legend(title='Protocol', fontsize=11, title_fontsize=11)
    ax.set_ylim(0, 110)
    ax.grid(True, alpha=0.3, linestyle='--')
    
    # 2. Energy Scalability
    ax = axes[1]
    for protocol in sorted(df['protocol'].unique()):
        data = df[df['protocol'] == protocol].groupby('nodes')['avg_energy_per_node'].agg(['mean', 'std']).reset_index()
        ax.errorbar(data['nodes'], data['mean'], yerr=data['std'],
                   label=protocol, marker=PROTOCOL_MARKERS.get(protocol, 'o'),
                   color=PROTOCOL_COLORS.get(protocol), capsize=4, linewidth=2.5,
                   markersize=10, markeredgewidth=1.5, markeredgecolor='white')
    
    ax.set_xlabel('Number of Nodes', fontsize=12, fontweight='bold')
    ax.set_ylabel('Energy per Node (J)', fontsize=12, fontweight='bold')
    ax.set_title('Energy Efficiency vs Network Size', fontsize=13, fontweight='bold')
    ax.legend(title='Protocol', fontsize=11, title_fontsize=11)
    ax.grid(True, alpha=0.3, linestyle='--')
    
    plt.suptitle('Scalability Analysis', fontsize=16, fontweight='bold', y=1.02)
    plt.tight_layout()
    
    output_file = output_dir / f'2_scalability_analysis.{fmt}'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"✓ Saved: {output_file}")


def plot_reliability_analysis(df, output_dir, fmt):
    """
    RELIABILITY: Box plots showing consistency and variance.
    """
    fig, axes = plt.subplots(1, 2, figsize=(14, 5))
    
    # 1. PDR Distribution
    ax = axes[0]
    df_plot = df.copy()
    df_plot['pdr_percent'] = df_plot['pdr'] * 100
    
    sns.boxplot(data=df_plot, x='protocol', y='pdr_percent', ax=ax,
                palette=PROTOCOL_COLORS, linewidth=1.5)
    
    ax.set_xlabel('Protocol', fontsize=12, fontweight='bold')
    ax.set_ylabel('Packet Delivery Ratio (%)', fontsize=12, fontweight='bold')
    ax.set_title('PDR Consistency', fontsize=13, fontweight='bold')
    ax.set_ylim(0, 110)
    ax.grid(axis='y', alpha=0.3)
    
    # 2. Energy Distribution
    ax = axes[1]
    sns.boxplot(data=df, x='protocol', y='avg_energy_per_node', ax=ax,
                palette=PROTOCOL_COLORS, linewidth=1.5)
    
    ax.set_xlabel('Protocol', fontsize=12, fontweight='bold')
    ax.set_ylabel('Energy per Node (J)', fontsize=12, fontweight='bold')
    ax.set_title('Energy Consistency', fontsize=13, fontweight='bold')
    ax.grid(axis='y', alpha=0.3)
    
    plt.suptitle('Reliability & Consistency Analysis', fontsize=16, fontweight='bold', y=1.02)
    plt.tight_layout()
    
    output_file = output_dir / f'3_reliability_analysis.{fmt}'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"✓ Saved: {output_file}")


def plot_energy_pdr_tradeoff(df, output_dir, fmt):
    """
    TRADE-OFF: Scatter plot showing Energy vs PDR trade-off.
    Ideal protocols are in top-left (high PDR, low energy).
    """
    fig, ax = plt.subplots(figsize=(10, 7))
    
    for protocol in sorted(df['protocol'].unique()):
        data = df[df['protocol'] == protocol]
        
        # Calculate mean for annotation
        mean_pdr = data['pdr'].mean() * 100
        mean_energy = data['avg_energy_per_node'].mean()
        
        ax.scatter(data['avg_energy_per_node'], data['pdr'] * 100,
                   label=protocol, alpha=0.6, s=100,
                   color=PROTOCOL_COLORS.get(protocol),
                   marker=PROTOCOL_MARKERS.get(protocol, 'o'),
                   edgecolors='black', linewidth=1)
        
        # Add protocol label at mean position
        ax.annotate(protocol, (mean_energy, mean_pdr),
                   fontsize=12, fontweight='bold',
                   xytext=(10, 10), textcoords='offset points',
                   bbox=dict(boxstyle='round,pad=0.5', facecolor=PROTOCOL_COLORS.get(protocol), alpha=0.3))
    
    ax.set_xlabel('Energy per Node (J)', fontsize=12, fontweight='bold')
    ax.set_ylabel('Packet Delivery Ratio (%)', fontsize=12, fontweight='bold')
    ax.set_title('Energy-PDR Trade-off Analysis', fontsize=14, fontweight='bold')
    ax.legend(title='Protocol', fontsize=11, title_fontsize=11)
    ax.grid(True, alpha=0.3, linestyle='--')
    ax.set_ylim(0, 110)
    
    # Add ideal region annotation
    ax.axhline(y=80, color='green', linestyle='--', alpha=0.3, linewidth=1)
    ax.text(ax.get_xlim()[1] * 0.95, 82, 'High PDR', 
            ha='right', va='bottom', fontsize=10, color='green', style='italic')
    
    plt.tight_layout()
    
    output_file = output_dir / f'4_energy_pdr_tradeoff.{fmt}'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"✓ Saved: {output_file}")


def plot_comprehensive_summary(df, output_dir, fmt):
    """
    COMPREHENSIVE DASHBOARD: 2x2 grid with key metrics.
    Perfect for presentations and papers.
    """
    fig = plt.figure(figsize=(16, 12))
    gs = fig.add_gridspec(2, 2, hspace=0.3, wspace=0.3)
    
    protocols = sorted(df['protocol'].unique())
    
    # 1. Protocol Ranking Table (Top Left)
    ax = fig.add_subplot(gs[0, 0])
    ax.axis('off')
    
    # Calculate rankings
    rankings = []
    for protocol in protocols:
        data = df[df['protocol'] == protocol]
        rankings.append({
            'Protocol': protocol,
            'PDR (%)': f"{data['pdr'].mean() * 100:.1f} ± {data['pdr'].std() * 100:.1f}",
            'Energy (J)': f"{data['avg_energy_per_node'].mean():.2f} ± {data['avg_energy_per_node'].std():.2f}",
            'Throughput': f"{data['sink_recv'].mean():.0f} ± {data['sink_recv'].std():.0f}"
        })
    
    ranking_df = pd.DataFrame(rankings)
    table = ax.table(cellText=ranking_df.values, colLabels=ranking_df.columns,
                     cellLoc='center', loc='center',
                     colColours=['lightgray']*4)
    table.auto_set_font_size(False)
    table.set_fontsize(11)
    table.scale(1, 2.5)
    
    # Color code by protocol
    for i, protocol in enumerate(protocols):
        table[(i+1, 0)].set_facecolor(PROTOCOL_COLORS.get(protocol, 'white'))
        table[(i+1, 0)].set_alpha(0.3)
    
    ax.set_title('Performance Summary', fontsize=14, fontweight='bold', pad=20)
    
    # 2. PDR Comparison (Top Right)
    ax = fig.add_subplot(gs[0, 1])
    pdr_data = df.groupby('protocol')['pdr'].agg(['mean', 'std']).reset_index()
    x_pos = np.arange(len(protocols))
    
    bars = ax.bar(x_pos, pdr_data['mean'] * 100,
                  yerr=pdr_data['std'] * 100,
                  color=[PROTOCOL_COLORS.get(p, 'gray') for p in protocols],
                  capsize=5, alpha=0.8, edgecolor='black', linewidth=1.2)
    
    ax.set_ylabel('PDR (%)', fontsize=11, fontweight='bold')
    ax.set_title('Packet Delivery Ratio', fontsize=13, fontweight='bold')
    ax.set_xticks(x_pos)
    ax.set_xticklabels(protocols)
    ax.set_ylim(0, 110)
    ax.grid(axis='y', alpha=0.3)
    
    for bar in bars:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height + 2,
                f'{height:.1f}%', ha='center', va='bottom', fontsize=10, fontweight='bold')
    
    # 3. Scalability (Bottom Left)
    ax = fig.add_subplot(gs[1, 0])
    for protocol in protocols:
        data = df[df['protocol'] == protocol].groupby('nodes')['pdr'].mean().reset_index()
        ax.plot(data['nodes'], data['pdr'] * 100,
                marker=PROTOCOL_MARKERS.get(protocol, 'o'),
                color=PROTOCOL_COLORS.get(protocol),
                label=protocol, linewidth=2.5, markersize=10)
    
    ax.set_xlabel('Number of Nodes', fontsize=11, fontweight='bold')
    ax.set_ylabel('PDR (%)', fontsize=11, fontweight='bold')
    ax.set_title('Scalability', fontsize=13, fontweight='bold')
    ax.legend(fontsize=10)
    ax.grid(True, alpha=0.3)
    ax.set_ylim(0, 110)
    
    # 4. Energy Efficiency (Bottom Right)
    ax = fig.add_subplot(gs[1, 1])
    energy_data = df.groupby('protocol')['avg_energy_per_node'].agg(['mean', 'std']).reset_index()
    
    bars = ax.bar(x_pos, energy_data['mean'],
                  yerr=energy_data['std'],
                  color=[PROTOCOL_COLORS.get(p, 'gray') for p in protocols],
                  capsize=5, alpha=0.8, edgecolor='black', linewidth=1.2)
    
    ax.set_ylabel('Energy per Node (J)', fontsize=11, fontweight='bold')
    ax.set_title('Energy Efficiency', fontsize=13, fontweight='bold')
    ax.set_xticks(x_pos)
    ax.set_xticklabels(protocols)
    ax.grid(axis='y', alpha=0.3)
    
    for bar in bars:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.2f}J', ha='center', va='bottom', fontsize=10, fontweight='bold')
    
    plt.suptitle('UWSN Protocol Comparison - Comprehensive Summary', 
                 fontsize=18, fontweight='bold', y=0.98)
    
    output_file = output_dir / f'5_comprehensive_summary.{fmt}'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"✓ Saved: {output_file}")


def main():
    parser = argparse.ArgumentParser(
        description="UWSN Protocol Comparison - Results Plotter (Streamlined)"
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
    
    print(f"\n{'='*60}")
    print(f"Generating {args.format.upper()} graphs in {output_dir}/")
    print(f"{'='*60}\n")
    
    # Generate meaningful plots only
    plot_protocol_comparison(df, output_dir, args.format)
    plot_scalability_analysis(df, output_dir, args.format)
    plot_reliability_analysis(df, output_dir, args.format)
    plot_energy_pdr_tradeoff(df, output_dir, args.format)
    plot_comprehensive_summary(df, output_dir, args.format)
    
    print(f"\n{'='*60}")
    print(f"✓ Generated 5 meaningful graphs in {output_dir}/")
    print(f"{'='*60}\n")
    
    print("Graph descriptions:")
    print("  1. Protocol Comparison - Overall performance metrics")
    print("  2. Scalability Analysis - Performance vs network size")
    print("  3. Reliability Analysis - Consistency and variance")
    print("  4. Energy-PDR Trade-off - Efficiency analysis")
    print("  5. Comprehensive Summary - Complete dashboard")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
UWSN Protocol Comparison - Results Analyzer
============================================
Calculates statistics from simulation results.

Usage:
    python analyze_results.py results.csv
"""

import pandas as pd
import numpy as np
from scipy import stats
import argparse
import sys
from pathlib import Path
# update this 
# import my actuall path

def load_data(csv_file):
    """Load and validate CSV data."""
    try:
        df = pd.read_csv(csv_file)
        print(f"Loaded {len(df)} rows from {csv_file}")
        return df
    except Exception as e:
        print(f"ERROR: File not found: {csv_file}")
        sys.exit(1)


def calculate_confidence_interval(data, confidence=0.95):
    """Calculate confidence interval for a data series."""
    n = len(data)
    if n < 2:
        return 0, 0
    mean = data.mean()
    se = stats.sem(data)
    ci = se * stats.t.ppf((1 + confidence) / 2, n - 1)
    return mean - ci, mean + ci


def analyze_by_protocol(df):
    """Analyze metrics grouped by protocol."""
    print("\n" + "=" * 70)
    print("   ANALYSIS BY PROTOCOL")
    print("=" * 70)
    
    metrics = ['pdr', 'avg_energy_per_node', 'network_lifetime_s', 'dead_nodes']
    
    for protocol in sorted(df['protocol'].unique()):
        data = df[df['protocol'] == protocol]
        n_runs = len(data)
        
        print(f"\n{protocol} (n={n_runs} runs)")
        print("-" * 40)
        
        for metric in metrics:
            if metric not in data.columns:
                continue
            values = data[metric].dropna()
            if len(values) == 0:
                continue
            
            mean = values.mean()
            std = values.std()
            ci_low, ci_high = calculate_confidence_interval(values)
            
            # Format based on metric type
            if metric == 'pdr':
                print(f"  PDR: {mean*100:.2f}% ± {std*100:.2f}% (95% CI: [{ci_low*100:.2f}%, {ci_high*100:.2f}%])")
            elif metric == 'network_lifetime_s':
                print(f"  Lifetime: {mean:.1f}s ± {std:.1f}s (95% CI: [{ci_low:.1f}s, {ci_high:.1f}s])")
            elif metric == 'dead_nodes':
                print(f"  Dead Nodes: {mean:.1f} ± {std:.1f}")
            else:
                print(f"  {metric}: {mean:.2f} ± {std:.2f}")


def analyze_by_nodes(df):
    """Analyze metrics grouped by node count."""
    print("\n" + "=" * 70)
    print("   ANALYSIS BY NODE COUNT")
    print("=" * 70)
    
    for nodes in sorted(df['nodes'].unique()):
        print(f"\n{nodes} Nodes")
        print("-" * 40)
        
        for protocol in sorted(df['protocol'].unique()):
            data = df[(df['nodes'] == nodes) & (df['protocol'] == protocol)]
            if len(data) == 0:
                continue
            
            pdr_mean = data['pdr'].mean() * 100
            pdr_std = data['pdr'].std() * 100
            energy_mean = data['avg_energy_per_node'].mean()
            
            print(f"  {protocol:10} PDR: {pdr_mean:5.1f}% ± {pdr_std:4.1f}%  |  Energy: {energy_mean:.2f} J/node")


def compare_protocols(df):
    """Statistical comparison between protocols."""
    print("\n" + "=" * 70)
    print("   PROTOCOL COMPARISON (T-TEST)")
    print("=" * 70)
    
    protocols = sorted(df['protocol'].unique())
    
    if len(protocols) < 2:
        print("Need at least 2 protocols for comparison.")
        return
    
    metrics = ['pdr', 'avg_energy_per_node']
    
    for metric in metrics:
        print(f"\n{metric.upper()}")
        print("-" * 40)
        
        for i, p1 in enumerate(protocols):
            for p2 in protocols[i+1:]:
                data1 = df[df['protocol'] == p1][metric].dropna()
                data2 = df[df['protocol'] == p2][metric].dropna()
                
                if len(data1) < 2 or len(data2) < 2:
                    continue
                
                t_stat, p_value = stats.ttest_ind(data1, data2)
                
                significance = ""
                if p_value < 0.001:
                    significance = "***"
                elif p_value < 0.01:
                    significance = "**"
                elif p_value < 0.05:
                    significance = "*"
                
                print(f"  {p1} vs {p2}: t={t_stat:.3f}, p={p_value:.4f} {significance}")


def generate_summary_table(df):
    """Generate summary table for paper."""
    print("\n" + "=" * 70)
    print("   SUMMARY TABLE (for paper)")
    print("=" * 70)
    
    summary_data = []
    
    for protocol in sorted(df['protocol'].unique()):
        for nodes in sorted(df['nodes'].unique()):
            data = df[(df['protocol'] == protocol) & (df['nodes'] == nodes)]
            if len(data) == 0:
                continue
            
            row = {
                'Protocol': protocol,
                'Nodes': nodes,
                'PDR (%)': f"{data['pdr'].mean()*100:.2f} ± {data['pdr'].std()*100:.2f}",
                'Energy (J)': f"{data['avg_energy_per_node'].mean():.2f} ± {data['avg_energy_per_node'].std():.2f}",
                'Lifetime (s)': f"{data['network_lifetime_s'].mean():.1f} ± {data['network_lifetime_s'].std():.1f}",
                'Sent Pkts': f"{data['sent_pkts'].mean():.0f}",
                'Recv Pkts': f"{data['recv_pkts'].mean():.0f}"
            }
            summary_data.append(row)
    
    summary_df = pd.DataFrame(summary_data)
    print("\n" + summary_df.to_string(index=False))
    
    return summary_df


def save_summary(df, output_file):
    """Save summary statistics to CSV."""
    summary_data = []
    
    for protocol in sorted(df['protocol'].unique()):
        for nodes in sorted(df['nodes'].unique()):
            data = df[(df['protocol'] == protocol) & (df['nodes'] == nodes)]
            if len(data) == 0:
                continue
            
            pdr_ci = calculate_confidence_interval(data['pdr'])
            energy_ci = calculate_confidence_interval(data['avg_energy_per_node'])
            lifetime_ci = calculate_confidence_interval(data['network_lifetime_s'])
            
            row = {
                'protocol': protocol,
                'nodes': nodes,
                'runs': len(data),
                'pdr_mean': data['pdr'].mean(),
                'pdr_std': data['pdr'].std(),
                'pdr_ci_low': pdr_ci[0],
                'pdr_ci_high': pdr_ci[1],
                'energy_mean': data['avg_energy_per_node'].mean(),
                'energy_std': data['avg_energy_per_node'].std(),
                'energy_ci_low': energy_ci[0],
                'energy_ci_high': energy_ci[1],
                'lifetime_mean': data['network_lifetime_s'].mean(),
                'lifetime_std': data['network_lifetime_s'].std(),
                'lifetime_ci_low': lifetime_ci[0],
                'lifetime_ci_high': lifetime_ci[1],
                'dead_nodes_mean': data['dead_nodes'].mean(),
                'sent_pkts_mean': data['sent_pkts'].mean(),
                'recv_pkts_mean': data['recv_pkts'].mean()
            }
            summary_data.append(row)
    
    summary_df = pd.DataFrame(summary_data)
    summary_df.to_csv(output_file, index=False)
    print(f"\nSummary saved to: {output_file}")


def main():
    parser = argparse.ArgumentParser(
        description="UWSN Protocol Comparison - Results Analyzer"
    )
    parser.add_argument(
        "csv_file",
        help="Path to the results CSV file"
    )
    parser.add_argument(
        "--output", "-o",
        default=None,
        help="Output file for summary (default: input_summary.csv)"
    )
    
    args = parser.parse_args()
    
    # Load data
    df = load_data(args.csv_file)
    
    # Run analyses
    analyze_by_protocol(df)
    analyze_by_nodes(df)
    compare_protocols(df)
    summary = generate_summary_table(df)
    
    # Save summary
    if args.output:
        output_file = args.output
    else:
        input_path = Path(args.csv_file)
        output_file = str(input_path.parent / f"{input_path.stem}_summary.csv")
    
    save_summary(df, output_file)


if __name__ == "__main__":
    main()

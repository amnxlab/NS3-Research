#!/usr/bin/env python3
"""
UWSN Protocol Comparison - Statistical Analysis
================================================
Analyzes simulation results and generates statistical summaries.

Usage:
    python analyze_results.py results.csv [--output summary.csv]
"""

import pandas as pd
import numpy as np
from scipy import stats
import argparse
from pathlib import Path
import sys


def load_results(csv_path):
    """Load and validate results CSV."""
    if not Path(csv_path).exists():
        print(f"ERROR: File not found: {csv_path}")
        sys.exit(1)
    
    df = pd.read_csv(csv_path)
    print(f"Loaded {len(df)} records from {csv_path}")
    print(f"Protocols: {df['protocol'].unique().tolist()}")
    print(f"Node counts: {sorted(df['nodes'].unique().tolist())}")
    print(f"Runs per config: {df.groupby(['protocol', 'nodes']).size().iloc[0]}")
    
    return df


def compute_statistics(df, group_cols, metric_cols):
    """Compute mean, std, CI for each metric grouped by columns."""
    results = []
    
    for name, group in df.groupby(group_cols):
        if isinstance(name, tuple):
            row = dict(zip(group_cols, name))
        else:
            row = {group_cols[0]: name}
        
        row['n_samples'] = len(group)
        
        for metric in metric_cols:
            values = group[metric].dropna()
            
            if len(values) > 0:
                mean = values.mean()
                std = values.std()
                sem = stats.sem(values)
                
                # 95% confidence interval
                if len(values) > 1:
                    ci = stats.t.interval(
                        0.95, 
                        len(values) - 1, 
                        loc=mean, 
                        scale=sem
                    )
                    ci_lower, ci_upper = ci
                else:
                    ci_lower, ci_upper = mean, mean
                
                row[f'{metric}_mean'] = mean
                row[f'{metric}_std'] = std
                row[f'{metric}_ci_lower'] = ci_lower
                row[f'{metric}_ci_upper'] = ci_upper
                row[f'{metric}_min'] = values.min()
                row[f'{metric}_max'] = values.max()
            else:
                row[f'{metric}_mean'] = np.nan
                row[f'{metric}_std'] = np.nan
                row[f'{metric}_ci_lower'] = np.nan
                row[f'{metric}_ci_upper'] = np.nan
                row[f'{metric}_min'] = np.nan
                row[f'{metric}_max'] = np.nan
        
        results.append(row)
    
    return pd.DataFrame(results)


def print_summary_table(df, title, metric, group_by='protocol'):
    """Print a formatted summary table."""
    print(f"\n{'='*60}")
    print(f"  {title}")
    print(f"{'='*60}")
    
    mean_col = f'{metric}_mean'
    std_col = f'{metric}_std'
    ci_lower = f'{metric}_ci_lower'
    ci_upper = f'{metric}_ci_upper'
    
    if group_by == 'protocol':
        for _, row in df.iterrows():
            print(f"{row['protocol']:12s}: "
                  f"{row[mean_col]:.4f} ± {row[std_col]:.4f} "
                  f"(95% CI: [{row[ci_lower]:.4f}, {row[ci_upper]:.4f}])")
    else:
        # Group by protocol and nodes
        protocols = df['protocol'].unique()
        nodes = sorted(df['nodes'].unique())
        
        # Header
        header = f"{'Nodes':>8}"
        for proto in protocols:
            header += f" | {proto:^20}"
        print(header)
        print("-" * len(header))
        
        # Data rows
        for n in nodes:
            row_str = f"{n:>8}"
            for proto in protocols:
                row = df[(df['protocol'] == proto) & (df['nodes'] == n)]
                if len(row) > 0:
                    row = row.iloc[0]
                    row_str += f" | {row[mean_col]:8.4f} ± {row[std_col]:6.4f}"
                else:
                    row_str += f" | {'N/A':^20}"
            print(row_str)


def generate_latex_table(df, metrics, output_path):
    """Generate LaTeX-formatted tables for papers."""
    latex_content = []
    
    for metric in metrics:
        mean_col = f'{metric}_mean'
        std_col = f'{metric}_std'
        
        latex_content.append(f"% Table for {metric}")
        latex_content.append(r"\begin{table}[htbp]")
        latex_content.append(r"\centering")
        latex_content.append(f"\\caption{{{metric.replace('_', ' ').title()} Comparison}}")
        latex_content.append(r"\begin{tabular}{l" + "c" * len(df['protocol'].unique()) + "}")
        latex_content.append(r"\toprule")
        
        # Header
        protocols = df['protocol'].unique()
        header = "Nodes & " + " & ".join(protocols) + r" \\"
        latex_content.append(header)
        latex_content.append(r"\midrule")
        
        # Data rows
        for nodes in sorted(df['nodes'].unique()):
            row_data = [str(int(nodes))]
            for proto in protocols:
                row = df[(df['protocol'] == proto) & (df['nodes'] == nodes)]
                if len(row) > 0:
                    row = row.iloc[0]
                    row_data.append(f"${row[mean_col]:.3f} \\pm {row[std_col]:.3f}$")
                else:
                    row_data.append("N/A")
            latex_content.append(" & ".join(row_data) + r" \\")
        
        latex_content.append(r"\bottomrule")
        latex_content.append(r"\end{tabular}")
        latex_content.append(r"\end{table}")
        latex_content.append("")
    
    latex_path = output_path.replace('.csv', '_latex.tex')
    with open(latex_path, 'w') as f:
        f.write('\n'.join(latex_content))
    
    print(f"LaTeX tables saved to: {latex_path}")


def analyze_protocol_performance(df):
    """Perform statistical tests comparing protocols."""
    print("\n" + "=" * 60)
    print("  STATISTICAL SIGNIFICANCE TESTS")
    print("=" * 60)
    
    protocols = df['protocol'].unique()
    metrics = ['pdr', 'avg_delay_ms', 'total_consumed_energy', 'throughput_bps']
    
    for metric in metrics:
        if metric not in df.columns:
            continue
            
        print(f"\n--- {metric.upper()} ---")
        
        # Kruskal-Wallis test (non-parametric)
        groups = [df[df['protocol'] == p][metric].dropna() for p in protocols]
        groups = [g for g in groups if len(g) > 0]
        
        if len(groups) >= 2:
            stat, p_value = stats.kruskal(*groups)
            print(f"Kruskal-Wallis test: H={stat:.4f}, p={p_value:.6f}")
            
            if p_value < 0.05:
                print("  -> Significant difference between protocols (p < 0.05)")
                
                # Pairwise Mann-Whitney U tests
                print("  Pairwise comparisons (Mann-Whitney U):")
                for i, p1 in enumerate(protocols):
                    for p2 in protocols[i+1:]:
                        g1 = df[df['protocol'] == p1][metric].dropna()
                        g2 = df[df['protocol'] == p2][metric].dropna()
                        if len(g1) > 0 and len(g2) > 0:
                            stat, p = stats.mannwhitneyu(g1, g2, alternative='two-sided')
                            sig = "*" if p < 0.05 else ""
                            print(f"    {p1} vs {p2}: U={stat:.2f}, p={p:.6f} {sig}")
            else:
                print("  -> No significant difference (p >= 0.05)")


def main():
    parser = argparse.ArgumentParser(
        description="UWSN Protocol Comparison - Statistical Analysis"
    )
    parser.add_argument(
        "input", type=str,
        help="Input CSV file with simulation results"
    )
    parser.add_argument(
        "--output", type=str, default=None,
        help="Output CSV file for summary statistics"
    )
    parser.add_argument(
        "--latex", action="store_true",
        help="Generate LaTeX tables"
    )
    
    args = parser.parse_args()
    
    # Load data
    df = load_results(args.input)
    
    # Define metrics to analyze
    metrics = [
        'pdr',
        'throughput_bps',
        'avg_delay_ms',
        'network_lifetime_s',
        'total_consumed_energy',
        'avg_energy_per_node',
        'energy_efficiency_pkts_per_j'
    ]
    
    # Filter to available metrics
    available_metrics = [m for m in metrics if m in df.columns]
    
    print(f"\nAnalyzing metrics: {available_metrics}")
    
    # Compute statistics by protocol
    print("\n" + "=" * 60)
    print("  SUMMARY BY PROTOCOL")
    print("=" * 60)
    
    summary_protocol = compute_statistics(df, ['protocol'], available_metrics)
    print(summary_protocol.to_string())
    
    # Compute statistics by protocol and nodes
    print("\n" + "=" * 60)
    print("  SUMMARY BY PROTOCOL AND NODE COUNT")
    print("=" * 60)
    
    summary_full = compute_statistics(df, ['protocol', 'nodes'], available_metrics)
    
    # Print formatted tables for key metrics
    for metric in ['pdr', 'avg_delay_ms', 'total_consumed_energy']:
        if metric in available_metrics:
            print_summary_table(
                summary_full, 
                f"{metric.upper().replace('_', ' ')}", 
                metric,
                group_by='protocol_nodes'
            )
    
    # Statistical significance tests
    analyze_protocol_performance(df)
    
    # Save output
    if args.output:
        output_path = args.output
    else:
        output_path = args.input.replace('.csv', '_summary.csv')
    
    summary_full.to_csv(output_path, index=False)
    print(f"\nSummary statistics saved to: {output_path}")
    
    # Generate LaTeX tables if requested
    if args.latex:
        generate_latex_table(summary_full, available_metrics[:4], output_path)
    
    print("\n" + "=" * 60)
    print("  ANALYSIS COMPLETE")
    print("=" * 60)
    print(f"\nNext step: Generate visualizations")
    print(f"  python plot_results.py {args.input}")


if __name__ == "__main__":
    main()


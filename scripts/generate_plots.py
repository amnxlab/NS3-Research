
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys
import os
import glob
from pathlib import Path

# Constants
PACKET_SIZE_BYTES = 40
PACKET_SIZE_BITS = PACKET_SIZE_BYTES * 8

def get_latest_results_file(results_dir):
    """Finds the latest CSV file in the results directory."""
    files = glob.glob(os.path.join(results_dir, "uwsn_v2_results_*.csv"))
    if not files:
        # Fallback to any csv
        files = glob.glob(os.path.join(results_dir, "*.csv"))
    
    if not files:
        print(f"No CSV files found in {results_dir}")
        sys.exit(1)
        
    # specific fallback if the known file exists
    known_file = os.path.join(results_dir, "uwsn_v2_results_20251224_215446.csv")
    if known_file in files:
        return known_file
        
    return max(files, key=os.path.getmtime)

def calculate_metrics(df):
    """Calculates additional metrics required for plotting."""
    
    # Ensure necessary columns exist
    required_cols = ['sender_sent', 'simStop', 'total_residual_energy', 'total_initial_energy']
    for col in required_cols:
        if col not in df.columns:
            print(f"Warning: Column {col} missing. Metrics might be incomplete.")
            
    # 1. ODR (Offered Data Rate) in kbps
    # ODR = (Generated Packets * Packet Size * 8) / (Simulation Time * 1000)
    if 'sender_sent' in df.columns and 'simStop' in df.columns:
        df['odr_kbps'] = (df['sender_sent'] * PACKET_SIZE_BITS) / (df['simStop'] * 1000)
    
    # 2. Energy Efficiency (%)
    # Interpretation: Remaining Energy Percentage = (Residual / Initial) * 100
    if 'total_residual_energy' in df.columns and 'total_initial_energy' in df.columns:
        df['energy_efficiency_percent'] = (df['total_residual_energy'] / df['total_initial_energy']) * 100
    
    # 3. PDR Percentage (if not already scaled 0-100, usually pdr is 0-1)
    if 'pdr' in df.columns:
        # Check if PDR is 0-1 or 0-100. Inspect max value.
        if df['pdr'].max() <= 1.0:
            df['pdr_percent'] = df['pdr'] * 100
        else:
            df['pdr_percent'] = df['pdr']
            
    return df

def generate_plot(df, y_var, x_var, output_prefix, result_filename_stem):
    """
    Generates a horizontal grouped bar chart.
    y_var: The categorical parameter (plotted on Y-axis). e.g., 'nodes'
    x_var: The metric (plotted on X-axis). e.g., 'pdr_percent'
    """
    
    # Mapping for labels
    label_map = {
        'nodes': 'Number of Nodes',
        'maxDepth': 'Depth Variation (m)',
        'vbfWidth': 'Pipe Width',
        'mobility': 'Mobility Model',
        'range': 'Communication Range (m)',
        'total_residual_energy': 'Residual Energy (J)',
        'energy_efficiency_percent': 'Energy Efficiency (%)',
        'pdr_percent': 'Packet Delivery Ratio (%)',
        'avg_delay_ms': 'Average Delay (ms)',
        'network_lifetime_s': 'Network Lifetime (s)',
        'odr_kbps': 'Offered Data Rate (kbps)'
    }
    
    y_label = label_map.get(y_var, y_var)
    x_label = label_map.get(x_var, x_var)
    
    # Group data
    # We want to group by y_var (e.g. Nodes: 100, 150, 200)
    # Inside each group, we show bars for each Protocol.
    
    # Get sorted unique values for the Y-variable (Categories)
    if y_var not in df.columns:
        return
        
    categories = sorted(df[y_var].unique())
    protocols = sorted(df['protocol'].unique())
    
    # Setup plot
    fig, ax = plt.subplots(figsize=(10, len(categories) * 2 + 1))
    
    # Bar configuration
    bar_height = 0.8 / len(protocols)
    y_indices = np.arange(len(categories))
    
    colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd'] # Colorblind friendly-ish standard set
    
    for i, protocol in enumerate(protocols):
        # Extract data for this protocol across all categories
        means = []
        stds = []
        
        for cat in categories:
            subset = df[(df['protocol'] == protocol) & (df[y_var] == cat)]
            if x_var in subset.columns and not subset.empty:
                means.append(subset[x_var].mean())
                stds.append(subset[x_var].std())
            else:
                means.append(0)
                stds.append(0)
        
        # Plot bars
        # Note: Horizontal bars. Y is positions.
        # We want grouped bars centered around y_indices
        # Offset calculation:
        # Total group height = 0.8.
        # Start = y - 0.4.
        
        offset = (i - len(protocols)/2) * bar_height + bar_height/2
        
        ax.barh(y_indices + offset, means, height=bar_height, label=protocol, xerr=stds, capsize=3)

    # Styling
    ax.set_yticks(y_indices)
    ax.set_yticklabels(categories)
    ax.set_ylabel(y_label, fontweight='bold')
    ax.set_xlabel(x_label, fontweight='bold')
    ax.set_title(f"{y_label} vs {x_label}")
    ax.legend(title="Protocol")
    ax.grid(axis='x', linestyle='--', alpha=0.7)
    
    # Output filename
    # [original_results_filename]_[plot_description].png
    # Description: nodes_vs_pdr
    desc = f"{y_var}_vs_{x_var}".replace("percent", "").replace("kbps", "").strip("_")
    filename = f"{result_filename_stem}_{desc}.png"
    output_path = os.path.join(os.path.dirname(output_prefix), filename)
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=300)
    plt.close()
    print(f"Generated: {output_path}")

def main():
    results_dir = "/home/ahmed/NS3-Research/results"
    
    # 1. Load Data
    csv_file = get_latest_results_file(results_dir)
    print(f"Processing: {csv_file}")
    
    df = pd.read_csv(csv_file)
    
    # 2. Calculate Metrics
    df = calculate_metrics(df)
    
    # 3. Define Analysis Matrix
    # Y-axis variables (Categorical/Input)
    y_vars = ['nodes', 'maxDepth', 'vbfWidth', 'mobility', 'range']
    
    # X-axis variables (Metrics/Output)
    x_vars = ['total_residual_energy', 'energy_efficiency_percent', 'pdr_percent', 
              'avg_delay_ms', 'network_lifetime_s', 'odr_kbps']
    
    # Identify relevant y-vars based on variation
    # If a variable has only 1 unique value, it's not a useful Y-axis category for comparison
    active_y_vars = []
    for var in y_vars:
        if var in df.columns:
            if df[var].nunique() > 1:
                active_y_vars.append(var)
    
    # If nodes is effectively the only changing var in this specific CSV, we use it.
    # If the user requested plots but the data doesn't vary, we might just plot result for that single value
    # But usually simulation results cover a range.
    # We will force plotting for 'nodes' even if unique=1 if it's the requested example.
    if 'nodes' not in active_y_vars and 'nodes' in df.columns:
        active_y_vars.append('nodes') # Force it for consistency with request
        
    print(f"Active Categorical Variables (Y-axis): {active_y_vars}")
    
    # Generate Plots
    result_filename_stem = Path(csv_file).stem
    
    for y in active_y_vars:
        for x in x_vars:
            if x in df.columns:
                generate_plot(df, y, x, results_dir, result_filename_stem)

if __name__ == "__main__":
    main()

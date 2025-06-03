import sys
import matplotlib.pyplot as plt
import seaborn as sns
from scipy import stats
import numpy as np

"""
This script takes as parameters:
1. A path to a file containing on each line a value corresponding to SaaKM performances
2. A path to a file containing on each line a value corresponding to SCX performances
3. A title

Based on that, it will read the values in each file and generate a boxplot with enhanced visualization.
"""

def read_values(file_path):
        with open(file_path, 'r') as file:
                return [float(line.strip()) for line in file]

def print_statistics(saakm_values, scx_values):
        print("Statistics between SaaKM and SCX:")
        print(f"SaaKM - Mean: {np.mean(saakm_values):.2f}, Median: {np.median(saakm_values):.2f}, Std: {np.std(saakm_values):.2f}")
        print(f"SCX - Mean: {np.mean(scx_values):.2f}, Median: {np.median(scx_values):.2f}, Std: {np.std(scx_values):.2f}")
        
        # Additional statistics
        print(f"SaaKM - Min: {np.min(saakm_values):.2f}, Max: {np.max(saakm_values):.2f}")
        print(f"SCX - Min: {np.min(scx_values):.2f}, Max: {np.max(scx_values):.2f}")
        
        # Calculate gain or loss in percentage
        mean_saakm = np.mean(saakm_values)
        mean_scx = np.mean(scx_values)
        percentage_change = ((mean_saakm - mean_scx) / mean_scx) * 100
        print(f"Percentage change (SaaKM vs SCX): {percentage_change:.2f}%")

def generate_boxplot(saakm_values, scx_values, title, output_path):
        data = [saakm_values, scx_values]
        labels = ['SaaKM', 'SCX']
        
        # Create a figure
        plt.figure(figsize=(6, 4))  # Adjusted figure size for a tighter plot
        
        # Create a boxplot
        sns.boxplot(data=data, width=0.4, palette="Set2", showmeans=True, meanline=True)
        
        # Overlay a swarmplot for better distribution visualization
        sns.swarmplot(data=data, color=".25", size=4)
        
        # Add labels and title
        plt.xticks(ticks=[0, 1], labels=labels, fontsize=10)
        plt.title(title, fontsize=12)
        plt.ylabel('Performance', fontsize=10)
        plt.grid(axis='y', linestyle='--', alpha=0.7)
        
        # Adjust layout to make it tighter
        plt.tight_layout(pad=1.0)
        # plt.savefig("/tmp/temp.png")  # Save the figure with high resolution
        # plt.show()
        plt.savefig(output_path, dpi=300)  # Save the figure with high resolution

if __name__ == "__main__":
        if len(sys.argv) != 5:
                print("Usage: python boxplot.py <saakm_file> <scx_file> <title>")
                sys.exit(1)

        saakm_file = sys.argv[1]
        scx_file = sys.argv[2]
        title = sys.argv[3]
        output_path = sys.argv[4] 
        
        saakm_values = read_values(saakm_file)
        scx_values = read_values(scx_file)

        print_statistics(saakm_values, scx_values)
        generate_boxplot(saakm_values, scx_values, title, output_path)

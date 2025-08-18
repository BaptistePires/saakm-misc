import sys
import os
import numpy as np
import scipy.stats as stats

scheds = ["ext", "ipanema"]

benchmarks_description = {
        "hackbench": {
                "metric" : "Time (s)",
                "reading": "Lower is better",
                "title": "haakcbench",
        },
        "x265": {
                "metric" : "Frame per second (fps)",
                "reading": "Higher is better",
                "title": "x265 - Bosphorus 1080p"
        },
        "build-linux-kernel" : {
                "metric" : "Time (s)",
                "reading": "Lower is better",
                "title": "Linux Kernel Build (defconfig)"
        },
        "ffmpeg": {
                "metric" : "Frame per second (fps)",
                "reading": "Higher is better",
                "title": "ffmpeg - Encoder libx264 - Live"
        },
        "stockfish": {
                "metric" : "Nodes per second (nps)",
                "reading": "Higher is better",
                "title": "Stockfish - Chess Engine"
        },
        "rbenchmark": {
                "metric" : "Time (s)",
                "reading": "Lower is better",
                "title": "rbenchmark"
        },
        "dav1d": {
                "metric" : "Frame per second (fps)",
                "reading": "Higher is better",
                "title": "dav1d - Video 4K"
        },
        "npb" : {
                "metric": "Mop/s",
                "reading": "Higher is better",
                "title": "NPB - Block Tri-diagonal solver (BT.C)"
        },
        "stress-ng": {
                "metric" : "Bogo Ops/s",
                "reading": "Higher is better",
                "title": "stress-ng - Pthread test"
        },
        "blender":  {
                "metric" : "Time (s)",
                "reading": "Lower is better",
                "title": "Blender - CPU-Only - BMW27"
        },
        "cloverleaf" : {
                "metric" : "Time (s)",
                "reading": "Lower is better",
                "title": "Cloverleaf - clover_bm"
        }

}

benchmarks_results = {}

if len(sys.argv) < 2:
        print("Usage: python boxplot-apps.py <input_directory>")
        sys.exit(1)

input_dir = sys.argv[1]

if not os.path.isdir(input_dir):
        print(f"Error: '{input_dir}' is not a valid directory.")
        sys.exit(1)

for benchmark in os.listdir(input_dir):
        entry_path = os.path.join(input_dir, benchmark)
        if os.path.isdir(entry_path):
                if benchmark == "compress-7zip": continue

                benchmarks_results[benchmark] = {
                        "ext" : [],
                        "ipanema" : []
                }

                for sched in scheds:
                        parsed_file = os.path.join(entry_path, sched, "parsed.txt")
                        
                        with open(parsed_file, "r") as f:
                                benchmarks_results[benchmark][sched] = [float(l.strip()) for l in f.readlines() if l.strip()]

import matplotlib.pyplot as plt
for benchmark, results in benchmarks_results.items():
        data = [results[sched] for sched in scheds]
        plt.figure()
        plt.boxplot(data, tick_labels=scheds)
        plt.title(f'{benchmarks_description[benchmark]["title"]}')
        plt.ylabel(f"{benchmarks_description[benchmark]["metric"]} - {benchmarks_description[benchmark]["reading"]}")
        plt.xlabel('Scheduler')
        plt.grid()
        plt.savefig(f'{benchmark}_boxplot.png')
        plt.close()


        percent_diffs = []
        benchmarks = []

for benchmark, results in benchmarks_results.items():
        
        ext_mean = np.mean(results["ext"])
        ipanema_mean = np.mean(results["ipanema"])
        if ext_mean != 0:
                percent_diff = ((ipanema_mean - ext_mean) / ((ext_mean + ipanema_mean) / 2)) * 100
                print(benchmark, percent_diff)
                percent_diffs.append(percent_diff)
                benchmarks.append(benchmark)

plt.figure(figsize=(10, 6))
bars = plt.bar(benchmarks, percent_diffs, color='skyblue')
plt.axhline(0, color='gray', linewidth=0.8, linestyle='--')
plt.ylabel('Percentage Difference (%)')
plt.xlabel('Benchmark')
plt.title('Percentage Difference between ipanema and ext (base) per Benchmark')
plt.xticks(rotation=45, ha='right')
plt.tight_layout()
plt.savefig('percentage_difference_barplot.png')
plt.close()
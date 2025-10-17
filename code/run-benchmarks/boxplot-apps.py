import sys
import os
import numpy as np
import scipy.stats as stats

scheds = ["ext", "ipanema"]

benchmarks_description = {
        "hackbench": {"metrics" : [
                {
                "metric" : "Time (s)",
                "reading": "Lower is better",
                "title": "haakcbench",
                "parsed_filename" : "parsed.txt"
                }
        ]},
        "x265": {
                "metrics": [
                        {
                "metric" : "Frame per second (fps)",
                "reading": "Higher is better",
                "title": "x265 - Bosphorus 1080p",
                "parsed_filename" : "parsed.txt"
        }
                ]
        },
        "build-linux-kernel" : {
                "metrics": [
                {
                "metric" : "Time (s)",
                "reading": "Lower is better",
                "title": "Linux Kernel Build (defconfig)",
                "parsed_filename" : "parsed.txt"
        }]
        },
        "ffmpeg": {
                "metrics": [
                        {
                "metric" : "Frame per second (fps)",
                "reading": "Higher is better",
                "title": "ffmpeg - Encoder libx264 - Live",
                "parsed_filename" : "parsed.txt"
        }
                ]
        },
        "stockfish": {
                "metrics": [
                        {
                "metric" : "Nodes per second (nps)",
                "reading": "Higher is better",
                "title": "Stockfish - Chess Engine",
                "parsed_filename" : "parsed.txt"
        }
                ]
        },
        "rbenchmark": {
                "metrics" : [
                        {
                "metric" : "Time (s)",
                "reading": "Lower is better",
                "title": "rbenchmark",
                "parsed_filename" : "parsed.txt"
        }
                ]
        },
        "dav1d": {
                "metrics" : [
                        {
                "metric" : "Frame per second (fps)",
                "reading": "Higher is better",
                "title": "dav1d - Video 4K",
                "parsed_filename" : "parsed.txt"
        }
                ]
        },
        "npb" : {
                "metrics" : [
                        {
                "metric": "Mop/s",
                "reading": "Higher is better",
                "title": "NPB - Block Tri-diagonal solver (BT.C)",
                "parsed_filename" : "parsed.txt"
        }
                ]
        },
        "stress-ng": {
                "metrics" : [
                        {
                "metric" : "Bogo Ops/s",
                "reading": "Higher is better",
                "title": "stress-ng - Pthread test",
                "parsed_filename" : "parsed.txt"
        }
                ]
        },
        "blender":  {
                "metrics" : [
                        {
                "metric" : "Time (s)",
                "reading": "Lower is better",
                "title": "Blender - CPU-Only - BMW27",
                "parsed_filename" : "parsed.txt"
        }
                ]
        },
        "cloverleaf" : {
                "metrics" : [
                        {
                "metric" : "Time (s)",
                "reading": "Lower is better",
                "title": "Cloverleaf - clover_bm",
                "parsed_filename" : "parsed.txt"
        }]
        },
        "cassandra" : {
                "metrics" : [
                        {
                "metric" : "Operations per second (ops/s)",
                "reading": "Higher is better",
                "title": "Cassandra - Writes",
                "parsed_filename" : "parsed.txt"
        }
                ]
        },
        "svt-av1" : {
                "metrics" : [
                        {
                "metric" : "Frames per second (fps)",
                "reading": "Higher is better",
                "title": "Encoder preset 13 - Input: Bosphorus 4K",
                "parsed_filename" : "parsed.txt"
        }
                ]
        },
        "namd" : {
                "metrics" : [
                        {
                "metric" : "ns/day",
                "reading": "Higher is better",
                "title": "ATParse with 327,506 Atoms",
                "parsed_filename" : "parsed.txt"
        }
                ]
        },
        "byte" : {
                "metrics" : [
                        {
                "metric" : "LPS",
                "reading": "Higher is better",
                "title": "Dhrystone 2",
                "parsed_filename" : "parsed.txt"
        }
                ]
        },
        "onednn" : {
                "metrics" : [
                        {
                "metric" : "Time (ms)",
                "reading": "Lower is better",
                "title": "Convolution Batch Shapes Auto - CPU",
                "parsed_filename" : "parsed.txt"
        }
                ]
        },
        "compress-7zip" :  {
                "multiple": True,
                "metrics" : [
                        {       "name" : "compress",
                                "metric" : "Compression ratio",
                                "reading": "Higher is better",
                                "title": "7zip - Compression ratio",
                                "parsed_filename": "parsed.txt-compress.txt"
                        },
                        {
                                "name": "decompress",
                                "metric" : "Decompresion",
                                "reading": "Higher is better",
                                "title": "7zip - Decompression ratio",
                                "parsed_filename": "parsed.txt-decompress.txt"
                        },
                ]
        },
        "clickhouse" : {
                "multiple": True,
                "metrics" : [
                        {
                                "name": "cold-firstrun",
                                "metric" : "Queries per minute",
                                "reading": "Higher is better",
                                "title": "Clickhouse firstrun",
                                "parsed_filename" : "parsed.txt-firstrun.txt"
                        },
                        {
                                "name": "second-run",
                                "metric" : "Queries per minute",
                                "reading": "Higher is better",
                                "title": "Clickhouse firstrun",
                                "parsed_filename" : "parsed.txt-secondrun.txt"
                        },
                                                {
                                "name": "third-run",
                                "metric" : "Queries per minute",
                                "reading": "Higher is better",
                                "title": "Clickhouse firstrun",
                                "parsed_filename" : "parsed.txt-thirdrun.txt"
                        },
                ]
        },
}


HSB = "Higher is better"
LSB = "Lower is better"

ignored_benchmarks = ["stress-ng"]
benchmarks_results = {}

if len(sys.argv) < 2:
        print("Usage: python boxplot-apps.py <input_directory>")
        sys.exit(1)

input_dir = sys.argv[1]

if not os.path.isdir(input_dir):
        print(f"Error: '{input_dir}' is not a valid directory.")
        sys.exit(1)

for benchmark_name in os.listdir(input_dir):
        entry_path = os.path.join(input_dir, benchmark_name)
        if os.path.isdir(entry_path):
                if benchmark_name in ignored_benchmarks: continue

                current_benchmark_desc = benchmarks_description.get(benchmark_name, None)
                multiple = True if "multiple" in current_benchmark_desc else False

                for metric in current_benchmark_desc["metrics"]:
                        
                        benchmark_key = f"{benchmark_name}_{metric['name']}" if multiple else benchmark_name

                        benchmarks_results[benchmark_key] = {
                                "ext" : [],
                                "ipanema" : [],
                                "metric": metric["metric"],
                                "reading": metric["reading"],
                                "title": metric["title"]
                        }

                        for sched in scheds:
                                parsed_file = os.path.join(entry_path, sched, metric["parsed_filename"])
                                
                                with open(parsed_file, "r") as f:
                                        benchmarks_results[benchmark_key][sched] = [float(l.strip()) for l in f.readlines() if l.strip()]

import matplotlib.pyplot as plt
for benchmark_name, results in benchmarks_results.items():
        data = [results[sched] for sched in scheds]
        plt.figure()
        plt.boxplot(data, tick_labels=scheds)
        plt.title(f'{results["title"]}')
        plt.ylabel(f"{results["metric"]} - {results["reading"]}")
        plt.xlabel('Scheduler')
        plt.grid()
        plt.savefig(f'{benchmark_name}_boxplot.png')
        plt.close()


        percent_diffs = []
        benchmarks = []

for benchmark_name in sorted(benchmarks_results.keys()):
        results = benchmarks_results[benchmark_name]
        ext_mean = np.mean(results["ext"])
        ipanema_mean = np.mean(results["ipanema"])
        if ext_mean != 0:
                percent_diff = ((ipanema_mean - ext_mean) / ((ext_mean + ipanema_mean) / 2)) * 100
                percent_diff_2 = ((ext_mean - ipanema_mean) / ext_mean) * 100
                
                if results["reading"] == LSB:
                        percent_diff = -percent_diff
                        percent_diff_2 = -percent_diff_2
                        
                print(benchmark_name, round(percent_diff,2), round(percent_diff_2,2))        
                ext_std = np.std(results["ext"], ddof=1)
                ipanema_std = np.std(results["ipanema"], ddof=1)
                ext_n = len(results["ext"])
                ipanema_n = len(results["ipanema"])

                # 95% confidence interval for the mean
                ext_ci = 1.95 * ext_std / np.sqrt(ext_n) if ext_n > 1 else 0
                ipanema_ci = 1.95 * ipanema_std / np.sqrt(ipanema_n) if ipanema_n > 1 else 0

                metric_label = "\\lsb" if results["reading"] == LSB else "\\hsb"
                # print(f'{benchmark_name.replace("_", "\\_")} - {results["metric"]} & {metric_label} & {round(np.mean(results["ext"]), 2)} $\\pm$ {round(ext_ci, 2)} & {round(np.mean(results["ipanema"]), 2)} $\\pm$ {round(ipanema_ci, 2)} & {round(percent_diff, 2)}\\% \\\\')
                percent_diffs.append(percent_diff)
                benchmarks.append(benchmark_name)

print(np.mean(percent_diffs), stats.tstd(percent_diffs))
plt.figure(figsize=(10, 6))
bars = plt.bar(benchmarks, percent_diffs, color='skyblue')
plt.axhline(0, color='gray', linewidth=0.8, linestyle='--')
plt.ylabel('Percentage Difference (%)')
plt.xlabel('Benchmark')
plt.title('Percentage Difference between ipanema and ext (base) per Benchmark')
plt.xticks(rotation=45, ha='right')
plt.ylim(-5,5)
plt.tight_layout()
plt.savefig('percentage_difference_barplot.png')
plt.close()


# for benchmark_name, results in benchmarks_results.items():
        
#         ext_mean = np.mean(results["ext"])
#         ipanema_mean = np.mean(results["ipanema"])
#         if ext_mean != 0:
#                 percent_diff = ((ipanema_mean - ext_mean) / ((ext_mean + ipanema_mean) / 2)) * 100

#                 if results["reading"] == LSB:
#                         percent_diff = -percent_diff
                        
#                 print(benchmark_name, percent_diff)
#                 percent_diffs.append(percent_diff)
#                 benchmarks.append(benchmark_name)


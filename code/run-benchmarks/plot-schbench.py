import numpy as np
import json
from sys import argv
import os
if len(argv) < 2:
        print("Usage: python plot-schbench.py <path_to_schbench_output>")
        exit(1)

input_dir = argv[1]


results_dirs = [f"schbench_{i}" for i in range(1, 100)]

benchmarks_results = {
        "ext" : [],
        "ipanema" : [],
        "nr_workers": []
}

for i in range(1, 100):
        current_dir = input_dir + "/" + f"schbench_{i}"

        for sched in ["ext", "ipanema"]:
                current_file = f"{current_dir}/{sched}/1.json"

                if not os.path.isfile(current_file):
                        continue
                with open(current_file, "r") as f:
                        data = json.load(f)
                        benchmarks_results[sched].append(data["int"]["rps_pct99.0"])
        benchmarks_results["nr_workers"].append(i)
        
import matplotlib.pyplot as plt

plt.plot(benchmarks_results["nr_workers"], benchmarks_results["ext"], label="ext")
plt.plot(benchmarks_results["nr_workers"], benchmarks_results["ipanema"], label="ipanema")
plt.xlabel("Number of workers")
plt.ylabel("99.9th percentile request latency")
plt.title("Schbench Results")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()
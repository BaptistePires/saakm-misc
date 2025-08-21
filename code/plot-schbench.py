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
                        benchmarks_results[sched].append(data["results"]["request_latency_pct99.9"])
        benchmarks_results["nr_workers"].append(i)
        
print(benchmarks_results)
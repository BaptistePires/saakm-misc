from sys import argv

fin=argv[1]
benchmarks = {}
with open(fin, "r") as f:
        for l in f.readlines():
                l = l.strip()
                bench, file, sched, mean, CI_low, CI_high = l.split()
                
                file_metric= file.split("-")
                if len(file_metric) == 2:
                        bench += "-" + file_metric[1].split(".")[0]

                if bench not in benchmarks:
                        benchmarks[bench] = {}
                if sched not in benchmarks[bench]:
                        benchmarks[bench][sched] = {
                                "mean": float(mean),
                                "CI_low": float(CI_low),
                                "CI_high": float(CI_high)
                        }

for benchmark in benchmarks:
        ext_ci_low = benchmarks[benchmark]["ext"]["CI_low"]
        ipanema_ci_low = benchmarks[benchmark]["ipanema"]["CI_low"]
        ext_ci_high = benchmarks[benchmark]["ext"]["CI_high"]
        ipanema_ci_high = benchmarks[benchmark]["ipanema"]["CI_high"]

        ext_ci = (ext_ci_low, ext_ci_high)
        ipanema_ci = (ipanema_ci_low, ipanema_ci_high)

        if ext_ci[0] > ipanema_ci[0] and ext_ci[0] < ipanema_ci[1]:
                print("intersection")
        
        if ext_ci[1] > ipanema_ci[0] and ext_ci[1] < ipanema_ci[1]:
                print(benchmark, "intersection")        
                ext_mean = benchmarks[benchmark]["ext"]["mean"]
                ipanema_mean = benchmarks[benchmark]["ipanema"]["mean"]
                # print(f"{benchmark} & {ext_mean:.2f} ({ext_ci_high - ext_ci_low:.2f}) & {ipanema_mean:.2f} ({ipanema_ci_high - ipanema_ci_low:.2f}) \\\\")
for benchmark in benchmarks:
        ext_ci_low = benchmarks[benchmark]["ext"]["CI_low"]
        ipanema_ci_low = benchmarks[benchmark]["ipanema"]["CI_low"]
        ext_ci_high = benchmarks[benchmark]["ext"]["CI_high"]
        ipanema_ci_high = benchmarks[benchmark]["ipanema"]["CI_high"]

        ext_ci = (ext_ci_low, ext_ci_high)
        ipanema_ci = (ipanema_ci_low, ipanema_ci_high)
        ext_mean = benchmarks[benchmark]["ext"]["mean"]
        ipanema_mean = benchmarks[benchmark]["ipanema"]["mean"]
        percent_diff = ((ipanema_mean - ext_mean) / ((ext_mean + ipanema_mean) / 2)) * 100
  
        ext_mean = benchmarks[benchmark]["ext"]["mean"]
        ipanema_mean = benchmarks[benchmark]["ipanema"]["mean"]
        print(f"{benchmark} & {ext_mean:.2f} ($\\pm$ {ext_ci_high - ext_mean:.2f}) & {ipanema_mean:.2f} ($\\pm$ {ipanema_ci_high - ipanema_mean:.2f}) & {percent_diff:.2f} \\\\")

usage() {
        echo "Usage : $0 <scheduler_framework> <benchmark_csv_file> <output_directory>"
        exit 1
}

install_benchmarks() {
        while read line; do
                if test $(echo $line | grep -c phoronix ) -ne 0; then
                        phoronix-test-suite install "$line"
                fi
        done < $1
}

check_if_schedstart_exists() {
    if ! command -v $1 &> /dev/null; then
        echo "Error: $1 not found in PATH."
        exit 1
   fi
}

clear_caches() {
        echo 3 > /proc/sys/vm/drop_caches
        sync
}
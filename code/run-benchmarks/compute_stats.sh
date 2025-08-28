#/bin/bash
IC_moyenne=/home/baptiste/code/statutils/IC_moyenne

input_dir="$1"

if [ ! -d "$input_dir" ]; then
	echo "Error: Directory '$input_dir' does not exist."
	exit 1
fi


outfile="./stats.txt"
> $outfile

for benchmark in $input_dir/*; do
        if [ "$(basename $benchmark)" = "logs.txt" ]; then
                continue
        fi
        for sched in ext ipanema; do
                current_wd="${benchmark}/${sched}"
                if [ ! -d "$current_wd" ]; then
                        echo "Warning: Directory '$current_wd' does not exist. Skipping."
                        exit
                fi
                
                parsed_files=$(ls $current_wd/parsed*.txt 2>/dev/null | tr '\n' ' ')

                for pf in $parsed_files; do
                        stas=$(cat $pf | tr '\n' ' ' | $IC_moyenne -c 0.95)
                        printf "%s %s %s %s\n" $(basename $benchmark) $(basename $pf) $sched "$stas" >> $outfile
                done
                echo "Processing $current_wd with files: $parsed_files"
                
                
        done
done

python analyse_CI.py $outfile
#!/bin/bash


for i in $(seq 1 10); do

        python child.py $i &

done

wait

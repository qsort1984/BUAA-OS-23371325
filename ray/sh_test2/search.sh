#!/bin/bash
#First you can use grep (-n) to find the number of lines of string.
#Then you can use awk to separate the answer.
input=$1
string=$2
output=$3

grep -n "$string" "$input" | cut -d: -f1 > "output"

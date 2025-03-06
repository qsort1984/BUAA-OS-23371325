#!/bin/bash
input=$1
output=$2
{
	sed -n '8p' "$input"
	sed -n '32p' "$input"
	sed -n '128p' "$input"
	sed -n '512p' "$input"
	sed -n '1024p' "$input"
} > $output


#!/bin/bash

ldbc_snb_home="$1"
output_path="$2"

python3 stream.py -i "$ldbc_snb_home" -o "$output_path"

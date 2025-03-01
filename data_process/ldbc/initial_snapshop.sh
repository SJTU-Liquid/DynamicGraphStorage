#!/bin/bash

ldbc_snb_home="$1"
tmp_dir="$2"
output_path="$3"

python3 sparse2dense.py -i "$ldbc_snb_home" -o "$tmp_dir"
python3 syn.py -i "$tmp_dir" -o "$output_path"

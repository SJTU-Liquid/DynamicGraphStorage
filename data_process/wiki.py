import bz2
from tqdm import tqdm

def process_file(input_file, output_file):
    with bz2.open(input_file, 'rt') as file:
        # Estimate the number of lines
        num_lines = 119544281
        
        with open(output_file, 'w') as out_file:
            article_id = None
            rev_id = None
            for line in tqdm(file, total=num_lines, desc="Processing"):
                line = line.strip()
                if line.startswith("REVISION"):
                    parts = line.split()
                    article_id = parts[1]
                    rev_id = parts[-1]
                    print(line)
                    out_file.write(f"{article_id} {rev_id}\n")

input_file = '/data/datasets/wiki/enwiki-20080103.main.bz2'
output_file = '/data/results/test/log.log'

process_file(input_file, output_file)

print("Processing complete. Output saved to", output_file)

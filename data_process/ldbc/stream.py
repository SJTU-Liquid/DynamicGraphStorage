from utils import *
import argparse
import os
import re
import pandas as pd
import time

parser = argparse.ArgumentParser(description='parser')
parser.add_argument('-i', '--input_path', type=str, help='input_path')
parser.add_argument('-o', '--output_path', type=str, help='output_path')

def find_files(pattern, directory):
    matched_files = []
    regex = re.compile(pattern)

    for file in os.listdir(directory):
        if regex.search(file):
            matched_files.append(file)

    return matched_files


def person(row, edge_stream, vertex_stream):
    ins1(row, edge_stream, vertex_stream)

def forum(row, edge_stream, vertex_stream):
    function_list = [ins1, ins2, ins3, ins4, ins5, ins6, ins7, ins8]
    function_list[int(row[2]) - 1](row, edge_stream, vertex_stream)

if __name__ == '__main__':
    arg = parser.parse_args()
    input_path = arg.input_path
    output_path = arg.output_path

    pattern = r'^updateStream.*\.csv$'
    matched_files = find_files(pattern, input_path)

    # edge_stream = pd.DataFrame(columns=['src', 'dst', 'label_src', 'label_dst', 'value', 'timestamp'])
    # vertex_stream = pd.DataFrame(columns=['id', 'label', 'timestamp'])
    edge_stream_list = []
    vertex_stream_list = []

    for file in matched_files:
        if file.endswith('forum.csv'): 
            time_start = time.time()
            print('processing file: ' + file)
            stream_file = pd.DataFrame(columns=range(15))
            with open(input_path + file, 'r') as file_open:
                for line in file_open:
                    row = line.split('|')
                    forum(row, edge_stream_list, vertex_stream_list)

            # stream_file = pd.read_csv(input_path + file, sep='|', header=None)
            # print(stream_file.index)
            
            time_end = time.time()
            print('time cost: ' + str(time_end - time_start) + 's')
            
        elif file.endswith('person.csv'):
            print('processing file: ' + file)
            time_start = time.time()
            stream_file = pd.read_csv(input_path + file, sep='|', dtype=str, header=None)
            stream_file.apply(person, axis=1, args=(edge_stream_list, vertex_stream_list,))
            time_end = time.time()
            print('time cost: ' + str(time_end - time_start) + 's')
        
        else:
            print('error: ' + file)
            exit(1)
    
    
    edge_stream = pd.DataFrame(edge_stream_list, columns=['src', 'dst', 'label_src', 'label_dst', 'value', 'timestamp'])
    vertex_stream = pd.DataFrame(vertex_stream_list, columns=['id', 'label', 'timestamp'])

    vertex_stream.to_csv(output_path + 'vertex_stream.csv', index=False, sep='|')
    edge_stream.to_csv(output_path + 'edge_stream.csv', index=False, sep='|')
    print(vertex_stream.index)
    print(edge_stream.index)

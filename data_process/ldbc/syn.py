import os
import pandas as pd
import argparse
from utils import *

parser = argparse.ArgumentParser(description='parser')
parser.add_argument('-i', '--input_path', type=str, help='input_path')
parser.add_argument('-o', '--output_path', type=str, help='output_path')


def syn_vertex(social_network_path, output_path):
    vertex_list = []
    
    for file in os.listdir(social_network_path + 'vertex/'):
        if not file.startswith('.'):
            file_name = file.split(sep='.')[0]
            tokens = file_name.split(sep='-')
            src = label2header[tokens[0]]

            df = pd.read_csv(social_network_path + 'vertex/' + file, sep='|', dtype=str, usecols=[src])
            df.insert(1, 'label', label2prefix[file.split(sep='.')[0]])
            vertex_list.append(df)
            print('processing file: ' + file)
    vertex = pd.concat(vertex_list, axis=0, ignore_index=True)
    vertex.to_csv(output_path + 'vertex.csv', index=False, sep=' ', header = False)

def syn_edges(social_network_path, output_path):
    edge_list_static = []
    edge_list_dynamic = []
    
    for file in os.listdir(social_network_path + 'edges/static/'):
        print('processing file: ' + file)
        file_name = file.split(sep='.')[0]
        tokens = file_name.split(sep='-')
        src = label2header[tokens[0]]
        dst = label2header[tokens[2]]
        if src == dst:
            dst = src + '.1'
        if not file.startswith('.'):
            df = pd.read_csv(social_network_path + 'edges/static/' + file, sep='|', dtype=str, usecols=[src, dst, 'label'])
            df.rename(columns={src: 'src', dst: 'dst'}, inplace=True)
            edge_list_static.append(df)

    for file in os.listdir(social_network_path + 'edges/dynamic/'):
        print('processing file: ' + file)
        file_name = file.split(sep='.')[0]
        tokens = file_name.split(sep='-')
        src = label2header[tokens[0]]
        dst = label2header[tokens[2]]
        if src == dst:
            dst = src + '.1'
        if not file.startswith('.'):
            df = pd.read_csv(social_network_path + 'edges/dynamic/' + file, sep='|', dtype=str, usecols=[src, dst, 'label', 'creationDate'])
            df.rename(columns={src: 'src', dst: 'dst'}, inplace=True)
            df = df[['src', 'dst', 'label', 'creationDate']]
            edge_list_dynamic.append(df)
            
    edge_static = pd.concat(edge_list_static, axis=0, ignore_index=True)
    edge_static = edge_static[['src', 'dst']]
    edge_static.to_csv(output_path + 'static_edges.csv', index=False, sep=' ', header=False)


    edge_dynamic = pd.concat(edge_list_dynamic, axis=0, ignore_index=True)
    edge_dynamic.sort_values(by=edge_dynamic.columns[3], ascending=True, inplace=True)
    edge_dynamic = edge_dynamic[['src', 'dst']]
    edge_dynamic.to_csv(output_path + 'dynamic_edges.csv', index=False, sep=' ', header=False)

if __name__ == '__main__':
    # social_network_path = '/root/datagen-graphs-regen/social_network-sf0.1-CsvComposite-LongDateFormatter/transformed/'
    # output_path = '/root/datagen-graphs-regen/social_network-sf0.1-CsvComposite-LongDateFormatter/plain/'
    
    args = parser.parse_args()
    social_network_path = args.input_path
    output_path = args.output_path
    if not os.path.exists(output_path): os.makedirs(output_path)

    syn_vertex(social_network_path, output_path)
    syn_edges(social_network_path, output_path)


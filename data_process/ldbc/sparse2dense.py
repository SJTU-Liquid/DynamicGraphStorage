from utils import *
import pandas as pd
import os
from multiprocessing import Process
import time
import hashlib
import random
import argparse
import json

parser = argparse.ArgumentParser(description='parser')
parser.add_argument('-i', '--input_path', type=str, help='input_path')
parser.add_argument('-o', '--output_path', type=str, help='output_path')
dic_sparse2dense = {}
list_of_vertex = {}

def vertex_transform(label, input_path, output_path, sf=0.1, hash_flag=False):
    prefix = label2prefix[label]
    print('processing vertex with label [' + label + ']')

    data_csv = pd.read_csv(input_path, sep='|', dtype=str)
    data_csv.dropna(axis=0, how='all', inplace=True)
    data_csv.insert(1, 'label', label)
    data_csv['id'] = data_csv['id'].apply(lambda x: id2non_repeat(prefix, x, num_bits=sf2num_bits[sf], dic=dic_sparse2dense, hash_flag=hash_flag))

    data_csv.rename(columns={'id': label2header[label]}, inplace=True)
    list_of_vertex[label] = data_csv
    data_csv.to_csv(output_path + label + '.csv', index=False, sep='|')
    print('finish processing vertex with label [' + label + ']')

def edge_transform(label_src, label_edge, label_dst, input_path, output_path, sf=0.1, hash_flag=False, dynamic=False):
    prefix_src = label2prefix[label_src]
    prefix_dst = label2prefix[label_dst]
    label_join = '-'.join([label_src, label_edge, label_dst])
    print('processing edge with label [' + label_join + ']')

    data_csv = pd.read_csv(input_path, sep='|', dtype=str)
    data_csv.dropna(axis=0, how='all', inplace=True)
    data_csv.insert(2, 'label', label_edge)
    id_header_src = label2header[label_src]
    data_csv[id_header_src] = data_csv[id_header_src].apply(lambda x: id2non_repeat(prefix_src, x, num_bits=sf2num_bits[sf], hash_flag=hash_flag))
    data_csv['label'] = data_csv['label'].apply(lambda x: relation2prefix[x])
    if label_src == label_dst:
        id_header_dst = id_header_src + '.1'
    else:
        id_header_dst = label2header[label_dst]
    data_csv[id_header_dst] = data_csv[id_header_dst].apply(lambda x: id2non_repeat(prefix_dst, x, num_bits=sf2num_bits[sf], hash_flag=hash_flag))

    data_csv.rename(columns={'joinDate': 'creationDate'}, inplace=True)
    

    if dynamic:
        if label_src == 'forum' and label_dst == 'post':
            data_csv = pd.merge(data_csv, list_of_vertex[label_dst], on = label2header[label_dst], how='left')
        else:
            data_csv = pd.merge(data_csv, list_of_vertex[label_src], on = label2header[label_src], how='left')
    
    data_csv.rename(columns={'creationDate_x': 'creationDate'}, inplace=True)
    data_csv.rename(columns={'label_x': 'label'}, inplace=True)

    data_csv.to_csv(output_path + label_join + '.csv', index=False, sep='|')

    # if label_edge == 'knows':
    #     dst = data_csv[id_header_dst]
    #     data_csv.drop(labels=[id_header_dst], axis=1, inplace=True)
    #     data_csv.insert(0, id_header_dst, dst)
    #     data_csv.to_csv(output_path + label_join + '-reverse.csv', index=False, sep='|')

    print('finish processing edge with label [' + label_join + ']')

def file_transform(file, input_path, output_path, sf=0.1, hash_flag=False, dynamic=False):
    if not os.path.exists(output_path + 'vertex/'): os.makedirs(output_path + 'vertex/')
    if not os.path.exists(output_path + 'edges/'): os.makedirs(output_path + 'edges/')
    if not os.path.exists(output_path + 'edges/static'): os.makedirs(output_path + 'edges/static')
    if not os.path.exists(output_path + 'edges/dynamic'): os.makedirs(output_path + 'edges/dynamic')


    prefix = file.split(sep='_0_0')[0]
    
    if not file.startswith('.'):
        print('prefix: ' + prefix)
        if prefix in label2prefix.keys():
            label = file.split(sep='_')[0]
            vertex_transform(label, input_path + file, output_path + 'vertex/', sf=0.1, hash_flag=hash_flag)

        else:
            label_src = file.split(sep='_')[0]
            label_edge = file.split(sep='_')[1]
            label_dst = file.split(sep='_')[2]
            if label_src not in label2prefix.keys() or label_dst not in label2prefix.keys(): return
            if (label_src, label_edge) in static_set:
                output_path = output_path + 'edges/static/'
            else: output_path = output_path + 'edges/dynamic/'
            
            edge_transform(label_src, label_edge, label_dst, input_path + file, output_path, sf=0.1, hash_flag=hash_flag, dynamic=dynamic) 

if __name__ == '__main__':
    time_start = time.time()

    args = parser.parse_args()
    social_network_path = args.input_path
    output_path = args.output_path

    for file in os.listdir(social_network_path + 'static/'):
        file_transform(file, social_network_path + 'static/', output_path, sf=get_sf(social_network_path), hash_flag=False, dynamic=False)

    for file in os.listdir(social_network_path + 'dynamic/'):
        prefix = file.split(sep='_0_0')[0]
        if prefix in label2prefix.keys():
            file_transform(file, social_network_path + 'dynamic/', output_path, sf=get_sf(social_network_path), hash_flag=False, dynamic=False)
    for file in os.listdir(social_network_path + 'dynamic/'):
        prefix = file.split(sep='_0_0')[0]
        if prefix not in label2prefix.keys():
            file_transform(file, social_network_path + 'dynamic/', output_path, sf=get_sf(social_network_path), hash_flag=False, dynamic=True)
    
    print('time_cost: ' + str(time.time() - time_start) + 's')
    json_file = json.dumps(dic_sparse2dense)
    with open(output_path + 'dic.json', 'w') as file:
        file.write(json_file)
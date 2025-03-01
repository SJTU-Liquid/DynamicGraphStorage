import hashlib
import random
import pandas as pd

label2prefix = {'comment': '1',
                'forum': '2',
                'person': '3',
                'post': '4',
                'place': '5',
                'tag': '6',
                'tagclass': '7',
                'organisation': '8'}

label2header = {'comment': 'Comment.id',
                'forum': 'Forum.id',
                'person': 'Person.id',
                'post': 'Post.id',
                'place': 'Place.id',
                'tag': 'Tag.id',
                 'tagclass': 'TagClass.id',
                 'organisation': 'Organisation.id'}      

labels = ['comment', 'forum', 'person', 'post', 'place', 'tag', 'tagclass', 'organisation']

static_set = {('forum', 'hasModerator'), ('organisation', 'isLocatedIn'), ('place', 'isPartOf'), ('tagclass', 'isSubclassOf'), ('tag', 'hasType')}

sf2num_bits = {0.1: 64}

relation2prefix = {'hasType': 1, 'isLocatedIn': 2, 'isSubclassOf': 3, 'isPartOf': 4, 'studyAt': 5, 'containerOf': 6, 'hasCreator': 7, 'hasTag': 8, 'hasModerator': 9, 'replyOf': 10, 'hasMember': 11, 'hasInterest': 12, 'likes': 13, 'workAt': 14, 'knows': 15}

def get_prefix(label): return label2prefix[label]
def get_header(label): return label2header[label]

def get_sf(file):
    return 0.1

def id2non_repeat(prefix, idx, num_bits=64, hash_flag=False, dic=None):
    zero_num = 17 - len(str(idx))

    if zero_num < 0:    
        return idx
    raw = prefix + '0' * zero_num + idx
    
    if hash_flag:
        ans =  sparse2dense(raw, num_bits)
    else:         
        ans = int(raw)

    if dic is not None:     
        dic[str(idx)] = ans

    return ans

def sparse2dense(raw, num_bits=64):
    dense_set = set()
    table = {}
    reverse_table = {}
    while True:
        hash_value = hashlib.md5(str(raw).encode()).digest()

        dense_value = int.from_bytes(hash_value, byteorder='big') % (2**num_bits - 1)

        if dense_value not in dense_set:
            dense_set.add(dense_value)
            break

        random_suffix = random.randint(0, 9999) 
        raw += str(random_suffix)
    table[raw] = dense_value
    reverse_table[dense_value] = raw

    return dense_value

def ins1(row, edge_stream, vertex_stream):
    vertex_stream.append([id2non_repeat(get_prefix('person'), row[2]), get_prefix('person'), row[0]])

    row[0] = row[0].replace('\n', '')
    update(edge_stream, row[2], 'person', str(row[11]), 'place', row[0])
    update(edge_stream, row[2], 'person', str(row[14]), 'tag', row[0])
    update(edge_stream, row[2], 'person', str(row[15]), 'organisation', row[0])
    update(edge_stream, row[2], 'person', str(row[16]), 'organisation', row[0])

def ins2(row, edge_stream, vertex_stream):
    row[5] = row[5].replace('\n', '')
    update(edge_stream, row[3], 'person', str(row[4]), 'post', row[5])

def ins3(row, edge_stream, vertex_stream):
    row[5] = row[5].replace('\n', '')
    update(edge_stream, row[3], 'person', str(row[4]), 'comment', row[5])

def ins4(row, edge_stream, vertex_stream):
    row[5] = row[5].replace('\n', '')
    vertex_stream.append([id2non_repeat(get_prefix('forum'), row[3]), get_prefix('forum'), row[5]])

    update(edge_stream, row[3], 'forum', str(row[6]), 'person', row[5])
    update(edge_stream, row[3], 'forum', str(row[7]), 'tag', row[5])

    return edge_stream, vertex_stream

def ins5(row, edge_stream, vertex_stream):
    row[5] = row[5].replace('\n', '')
    update(edge_stream, row[4], 'forum', str(row[3]), 'post', row[5])
    

def ins6(row, edge_stream, vertex_stream):
    row[5] = row[5].replace('\n', '')
    vertex_stream.append([id2non_repeat(get_prefix('post'), row[3]), get_prefix('post'), row[5]])

    update(edge_stream, row[3], 'post', str(row[13]), 'place', row[5])
    update(edge_stream, row[3], 'post', str(row[14]), 'tag', row[5])
    update(edge_stream, row[3], 'post', str(row[11]), 'person', row[5])
    update(edge_stream, str(row[12]), 'forum', row[3], 'post', row[5])

def ins7(row, edge_stream, vertex_stream):
    row[5] = row[5].replace('\n', '')
    vertex_stream.append([id2non_repeat(get_prefix('comment'), row[3]), get_prefix('comment'), row[5]])

    update(edge_stream, row[3], 'comment', str(row[9]), 'person', row[4])
    update(edge_stream, row[3], 'comment', str(row[10]), 'place', row[4])
    update(edge_stream, row[3], 'comment', str(row[11]), 'post', row[4])
    update(edge_stream, row[3], 'comment', str(row[12]), 'comment', row[4])
    update(edge_stream, row[3], 'comment', str(row[13]), 'tag', row[4])


def ins8(row, edge_stream, vertex_stream):
    row[5] = row[5].replace('\n', '')
    update(edge_stream, row[3], 'person', str(row[4]), 'person', row[5])
    update(edge_stream, row[4], 'person', str(row[3]), 'person', row[5])

def parse(str):
    tokens = str.split(sep=';')
    return [token.split(sep=',')[0] for token in tokens]

def get_id_list(label, id_str):
    return [id2non_repeat(get_prefix(label), id) for id in parse(id_str)]

def update(edge_stream, src_id, src_label, dst_id_str, dst_label, time_stamp, value=0):
    if(dst_id_str == 'nan' or dst_id_str == '-1'):
        return 
    dst_id_str = dst_id_str.replace('\n', '')
    src_id =id2non_repeat(get_prefix(src_label), src_id)
    id_list = get_id_list(dst_label, dst_id_str)
    for dst_id in id_list:
        edge_stream.append([src_id, dst_id, get_prefix(src_label), get_prefix(dst_label), value, time_stamp])

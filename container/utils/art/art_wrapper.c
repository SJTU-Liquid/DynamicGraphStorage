#include <malloc.h>
#include <stdbool.h>
#include <assert.h>
#include "art_wrapper.h"
#include "art.h"

/// Auxiliary type
struct M_Vector {
    void** data;
    int capacity;
    int size;
};

struct M_Vector* new_vec(void** data, int capacity) {
    struct M_Vector* vec = malloc(sizeof(struct M_Vector));
    vec->data = data;
    vec->capacity = capacity;
    vec->size = 0;
    return vec;
}

void free_vec(struct M_Vector* vec) {
    free(vec);
}

void* get_vec_data(struct M_Vector* vec) {
    return vec->data;
}

bool grow_to(struct M_Vector* vec, int target) {
    assert(target > vec->capacity);

    void* new_data = realloc(vec->data, target * sizeof(void**));
    if (new_data == NULL) {
        return false;
    }
    vec->data = new_data;
    vec->size = target;
    return true;
}

bool push_back(struct M_Vector* vec, void* value) {
    if (vec->size == vec->capacity) {
        if(grow_to(vec, vec->capacity * 2) == false) {
            printf("Error: failed to grow vector\n");
            return false;
        }
    }
    vec->data[vec->size] = value;
    vec->size++;
    return true;
}

/// Wrapper

void art_destroy_wrapper(void* ptr) {
    art_tree_destroy(ptr);
}

void* art_insert_wrapper(void* ptr, void* key, int key_size, void* value) {
    return art_insert(ptr, key, key_size, value);
}

void* art_delete_wrapper(void* ptr, unsigned char* key, int key_size) {
    art_delete(ptr, key, key_size);
}

void* art_search_wrapper(void* ptr, unsigned char* key, int key_size) {
    return art_search(ptr, key, key_size);
}

void* art_search_approx_wrapper(void* ptr, unsigned char* key, int key_size) {
    return art_search_approx(ptr, key, key_size);
}

int iter_values_cb (void *data, const unsigned char *key, uint32_t key_len, void *value) {
    struct M_Vector* vec = (struct M_Vector*)data;
    if (push_back(vec, value) == false) {
        return -1;
    }
    return 0;
}

/// NOTE: Vec must been deleted by the caller
void art_values_wrapper(void* ptr, void* vec_data_ptr, int vec_capacity) {
    // print size
    struct M_Vector* vec_ptr = new_vec(vec_data_ptr, vec_capacity);
    if(art_iter(ptr, iter_values_cb, vec_ptr) != 0) {
        printf("Error: failed to iterate over values\n");
    }
    free_vec(vec_ptr);
}

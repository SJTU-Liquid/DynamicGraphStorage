#pragma once
#include <fstream>
#include <string>
#include <sys/resource.h>
#include <iostream>

#define CACHE_LINE_SIZE 64  
#define L1D_CACHE_SIZE (3 * 1024 * 1024)  
#define NUM_INSTANCES 64 

int parseLine(char* line) {
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = atoi(p);
    return i;
}

int getValue(){ 
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmRSS:", 6) == 0){
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    return result;
}

void get_status() {
    std::ifstream file("/proc/self/status");
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Failed to open /proc/self/status" << std::endl;
    }

    while (std::getline(file, line)) {
        std::cout << line << std::endl;
    }

    file.close();
}

void get_page() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("Maximum resident set size: %ld kB\n", usage.ru_maxrss);
}

void cache_warmup() {
    size_t total_cache_size = L1D_CACHE_SIZE * NUM_INSTANCES;
    char *buffer = (char *)malloc(total_cache_size); 

    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    for (size_t i = 0; i < total_cache_size; i += CACHE_LINE_SIZE) {
        buffer[i] = (char)(i % 256);  
    }

    asm volatile("" : : "r,m"(buffer) : "memory");

    free(buffer);
}
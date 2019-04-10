#define _GNU_SOURCE
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <limits.h>
#include "bloom.h"
#include "xorfilter.h"
#include "xorfilter_plus.h"
#include "hexutil.h"

static void printusage(char *command) {
    printf(
        " Try %s -f xor+8 mydatabase \n",
        command);
    ;
}

int main(int argc, char** argv) {
    int c;
    char *filtername = "xor+8";
    while ((c = getopt(argc, argv, "f:h")) != -1) switch (c) {
            case 'f':
                filtername = optarg;
                break;
            case 'h':
                printusage(argv[0]);
                return 0;
            default:
                abort();
    }
    if (optind >= argc) {
        printusage(argv[0]);
        return -1;
    }
    const char *filename = argv[optind];
    


    char * line = NULL;
    size_t line_capacity = 0;
    size_t read;

    size_t array_capacity = 600 * 1024 * 1024;
    uint64_t * array = (uint64_t*) malloc(array_capacity * sizeof(uint64_t));
    if(array == NULL) {
        printf("Cannot allocate 5GB. Use a machine with plenty of RAM.");
        return EXIT_FAILURE;        
    }
    size_t array_size = 0; 

    FILE * fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Cannot read the input file %s.",filename);
        free(array);
        return EXIT_FAILURE;
    }
    clock_t start = clock();

    while ((read = getline(&line, &line_capacity, fp)) != -1) {
        if(read < 16) {
            printf("\r%256s", "");
            fflush(NULL);
            printf("Skipping line %s since it is too short (%zu)\n", 
              line, read);
            continue;
        }
        uint64_t x1 = hex_to_u32_nocheck((const uint8_t *)line);
        uint64_t x2 = hex_to_u32_nocheck((const uint8_t *)line + 4);
        uint64_t x3 = hex_to_u32_nocheck((const uint8_t *)line + 8);
        uint64_t x4 = hex_to_u32_nocheck((const uint8_t *)line + 12);
        if((x1 | x2 | x3 | x4) > 0xFFFF) {
            printf("\r%256s", "");
            fflush(NULL);
            printf("Skipping line %s since it does not start with an hexadecimal\n", 
              line);
            continue;
        }
        if(array_size >= array_capacity) {
            array_capacity = 3 * array_size / 2 + 64;
            uint64_t * newarray = (uint64_t *)realloc(array, array_capacity);
            if(newarray == NULL) {
              printf("Reallocation failed. Aborting.\n");
              return EXIT_FAILURE;
            }
            array = newarray;
        }
        array[array_size++] = (x1 << 48) | (x2 << 32) | (x3 << 16) | x4;
        if((array_size > 0) && ((array_size % 1000000)  == 0)) {
            printf("\rread %zu hashes.", array_size);
        }
        fflush(NULL);
    }
    clock_t end = clock();
    printf("\rI read %zu hashes in total (%.3f seconds).\n",array_size, (float)(end - start) / CLOCKS_PER_SEC);
    printf("Constructing the filter...\n");
    fflush(NULL);
    start = clock();
    using Table = xorfilter::XorFilter<uint64_t, uint8_t, SimpleMixSplit>;
    Table table(array_size);
    table.AddAll(array, 0, array_size);
    end = clock();
    printf("Done in %.3f seconds.\n", (float)(end - start) / CLOCKS_PER_SEC);
    free(array);

    fclose(fp);
    free(line);
    return EXIT_SUCCESS;
}

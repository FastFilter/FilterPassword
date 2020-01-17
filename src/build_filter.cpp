#define __STDC_FORMAT_MACROS
#define _GNU_SOURCE
#include <getopt.h>
#include <inttypes.h>
#include <iostream>
#include <limits.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bloom.h"
#include "hexutil.h"
#include "xorfilter.h"
#include "xor_singleheader/include/xorfilter.h"
#include "mappeablebloomfilter.h"


static void printusage(char *command) {
  printf(" Try %s -f xor8 -o filter.bin mydatabase \n", command);
  ;
  printf("The supported filters are xor8 and bloom12.\n");

  printf("The -V flag verifies the resulting filter.\n");
}

int main(int argc, char **argv) {
  int c;
  size_t maxline =
      1000 * 1000 * 1000; // one billion lines ought to be more than enough?
  const char *filtername = "xor8";
  bool printall = false;
  bool verify = false;
  const char *outputfilename = "filter.bin";
  while ((c = getopt(argc, argv, "af:ho:m:V")) != -1)
    switch (c) {
    case 'f':
      filtername = optarg;
      break;
    case 'o':
      outputfilename = optarg;
      break;
    case 'V':
      verify = true;
      break;
    case 'm':
      maxline = atoll(optarg);
      printf("setting the max. number of entries to %zu \n", maxline);
      break;
    case 'a':
      printall = true;
      break;
    case 'h':
    default:
      printusage(argv[0]);
      return 0;
    }
  if (optind >= argc) {
    printusage(argv[0]);
    return -1;
  }
  const char *filename = argv[optind];

  char *line = NULL;
  size_t line_capacity = 0;
  int read;

  size_t array_capacity = 600 * 1024 * 1024;
  uint64_t *array = (uint64_t *)malloc(array_capacity * sizeof(uint64_t));
  if (array == NULL) {
    printf("Cannot allocate 5GB. Use a machine with plenty of RAM.");
    return EXIT_FAILURE;
  }
  size_t array_size = 0;

  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("Cannot read the input file %s.", filename);
    free(array);
    return EXIT_FAILURE;
  }
  clock_t start = clock();

  while ((read = getline(&line, &line_capacity, fp)) != -1) {
    if (read < 16) {
      printf("\r%256s", "");
      fflush(NULL);
      printf("Skipping line %s since it is too short (%d)\n", line, read);
      continue;
    }
    uint64_t x1 = hex_to_u32_nocheck((const uint8_t *)line);
    uint64_t x2 = hex_to_u32_nocheck((const uint8_t *)line + 4);
    uint64_t x3 = hex_to_u32_nocheck((const uint8_t *)line + 8);
    uint64_t x4 = hex_to_u32_nocheck((const uint8_t *)line + 12);
    if ((x1 | x2 | x3 | x4) > 0xFFFF) {
      printf("\r%256s", "");
      fflush(NULL);
      printf("Skipping line %s since it does not start with an hexadecimal\n",
             line);
      continue;
    }
    if (array_size >= array_capacity) {
      array_capacity = 3 * array_size / 2 + 64;
      uint64_t *newarray = (uint64_t *)realloc(array, array_capacity);
      if (newarray == NULL) {
        printf("Reallocation failed. Aborting.\n");
        return EXIT_FAILURE;
      }
      array = newarray;
    }
    uint64_t hexval = (x1 << 48) | (x2 << 32) | (x3 << 16) | x4;
    if (printall)
      printf("hexval = 0x%" PRIx64 "\n", hexval);
    array[array_size++] = hexval;
    if ((array_size > 0) && ((array_size % 1000000) == 0)) {
      printf("\rread %zu hashes.", array_size);
    }
    if (array_size == maxline) {
      printf("Reached the maximum number of lines %zu.\n", maxline);
      break;
    }
    fflush(NULL);
  }
  clock_t end = clock();
  free(line);
  size_t numberofbytes = ftello(fp);
  fclose(fp);

  printf("\rI read %zu hashes in total (%.3f seconds).\n", array_size,
         (float)(end - start) / CLOCKS_PER_SEC);
  printf("Bytes read = %zu.\n", numberofbytes);
  printf("Constructing the filter...\n");
  fflush(NULL);
  if (strcmp("xor8", filtername) == 0) {
    start = clock();
    xor8_t filter;
    xor8_allocate(array_size, &filter);
    xor8_buffered_populate(array, array_size, &filter);
    end = clock();
    printf("Done in %.3f seconds.\n", (float)(end - start) / CLOCKS_PER_SEC);
    if(verify) {
      printf("Checking for false negatives\n");
      for(size_t i = 0; i < array_size; i++) {
        if(!xor8_contain(array[i],&filter)) {
          printf("Detected a false negative. You probably have a bug. Aborting.\n");
          return EXIT_FAILURE;
        }
      }
      printf("Verified with success: no false negatives\n");
    }
    free(array);

    FILE *write_ptr;
    write_ptr = fopen(outputfilename, "wb");
    if (write_ptr == NULL) {
      printf("Cannot write to the output file %s.", outputfilename);
      return EXIT_FAILURE;
    }
    uint64_t cookie = 1234567;
    uint64_t seed = filter.seed;
    uint64_t BlockLength = filter.blockLength;
    bool isok = true;
    size_t total_bytes = sizeof(cookie) + sizeof(seed) + sizeof(BlockLength) +
                         sizeof(uint8_t) * 3 * BlockLength;
    isok &= fwrite(&cookie, sizeof(cookie), 1, write_ptr);
    isok &= fwrite(&seed, sizeof(seed), 1, write_ptr);
    isok &= fwrite(&BlockLength, sizeof(BlockLength), 1, write_ptr);
    isok &= fwrite(filter.fingerprints, sizeof(uint8_t) * 3 * BlockLength, 1,
                   write_ptr);
    isok &= (fclose(write_ptr) == 0);
    if (isok) {
      printf("filter data saved to %s. Total bytes = %zu. \n", outputfilename,
             total_bytes);
    } else {
      printf("failed to write filter data to %s.\n", outputfilename);
      return EXIT_FAILURE;
    }
    xor8_free(&filter);
  }  else if (strcmp("bloom12", filtername) == 0) {
    start = clock();
    using Table = bloomfilter::BloomFilter<uint64_t, 12, false, SimpleMixSplit>;
    Table table(array_size);
    for(size_t i = 0; i < array_size; i++) {
      table.Add(array[i]);
    }
    end = clock();
    printf("Done in %.3f seconds.\n", (float)(end - start) / CLOCKS_PER_SEC);
    if(verify) {
      printf("Checking for false negatives\n");
      for(size_t i = 0; i < array_size; i++) {
        if(table.Contain(array[i]) != bloomfilter::Ok) {
          printf("Detected a false negative. You probably have a bug. Aborting.\n");
          return EXIT_FAILURE;
        }
      }
      MappeableBloomFilter<12> filter(
        table.SizeInBytes() / 8, table.hasher.seed, table.data);
      for(size_t i = 0; i < array_size; i++) {
        if(!filter.Contain(array[i])) {
          printf("Detected a false negative. You probably have a bug. Aborting.\n");
          return EXIT_FAILURE;
        }
      }
      printf("Verified with success: no false negatives\n");
    }
    free(array);
    FILE *write_ptr;
    write_ptr = fopen(outputfilename, "wb");
    if (write_ptr == NULL) {
      printf("Cannot write to the output file %s.", outputfilename);
      return EXIT_FAILURE;
    }
    uint64_t cookie = 1234568;
    uint64_t seed = table.hasher.seed;
    uint64_t ArrayLength = table.SizeInBytes() / 8;
    bool isok = true;
    size_t total_bytes = sizeof(cookie) + sizeof(seed) + sizeof(ArrayLength) +
                         sizeof(uint8_t) * 8 * ArrayLength;
    isok &= fwrite(&cookie, sizeof(cookie), 1, write_ptr);
    isok &= fwrite(&seed, sizeof(seed), 1, write_ptr);
    isok &= fwrite(&ArrayLength, sizeof(ArrayLength), 1, write_ptr);
    isok &= fwrite(table.data, sizeof(uint8_t) * 8 * ArrayLength, 1, write_ptr);
    isok &= (fclose(write_ptr) == 0);
    if (isok) {
      printf("filter data saved to %s. Total bytes = %zu. \n", outputfilename,
             total_bytes);
    } else {
      printf("failed to write filter data to %s.\n", outputfilename);
      return EXIT_FAILURE;
    }

  } else {
    printf("unknown filter: %s \n", filtername);
  }
  return EXIT_SUCCESS;
}

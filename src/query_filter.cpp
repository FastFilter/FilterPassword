#define __STDC_FORMAT_MACROS
#include "hexutil.h"
#include "mappeablebloomfilter.h"
#include "sha.h"
#include "xor_singleheader/include/binaryfusefilter.h"
#include "xor_singleheader/include/xorfilter.h"
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

static void printusage(char *command) {
  printf(" Try %s  filter.bin  7C4A8D09CA3762AF (use 16-hex-character keys)\n", command);
  ;
  printf("Use the -s if you want to provide a string to be hashed (SHA1).\n");
}

int main(int argc, char **argv) {
  int c;
  bool shaourinput = false;
  while ((c = getopt(argc, argv, "hs")) != -1)
    switch (c) {
    case 'h':
      printusage(argv[0]);
      return 0;
    case 's':
      shaourinput = true;
      break;
    default:
      printusage(argv[0]);
      return 0;
    }
  if (optind + 1 >= argc) {
    printusage(argv[0]);
    return -1;
  }
  const char *filename = argv[optind];
  printf("using database: %s\n", filename);
  uint64_t hexval;
  if (shaourinput) {
    printf("We are going to hash your input.\n");
    sha1::SHA1 s;
    const char *tobehashed = argv[optind + 1];
    printf("hashing this word: %s (length in bytes = %zu)\n", tobehashed,
           strlen(tobehashed));
    s.processBytes(tobehashed, strlen(tobehashed));
    uint32_t digest[5];
    s.getDigest(digest);
    char tmp[48];
    snprintf(tmp, 45, "%08x %08x %08x %08x %08x", digest[0], digest[1],
             digest[2], digest[3], digest[4]);
    printf("SHA1 hash %s\n", tmp);
    hexval = ((uint64_t)digest[0] << 32) | digest[1];
  } else {
    const char *hashhex = argv[optind + 1];
    if (strlen(hashhex) != 16) {
      printf("bad hex. length is %zu, 16 hexadecimal characters expected.\n", strlen(hashhex));
      printusage(argv[0]);
      return EXIT_FAILURE;
    }
    uint64_t x1 = hex_to_u32_nocheck((const uint8_t *)hashhex);
    uint64_t x2 = hex_to_u32_nocheck((const uint8_t *)hashhex + 4);
    uint64_t x3 = hex_to_u32_nocheck((const uint8_t *)hashhex + 8);
    uint64_t x4 = hex_to_u32_nocheck((const uint8_t *)hashhex + 12);
    if ((x1 | x2 | x3 | x4) > 0xFFFF) {
      printf("bad hex value  %s \n", hashhex);
      return EXIT_FAILURE;
    }
    hexval = (x1 << 48) | (x2 << 32) | (x3 << 16) | x4;
  }
  printf("hexval = 0x%" PRIx64 "\n", hexval);
  uint64_t cookie = 0;

  uint64_t seed = 0;
  uint64_t BlockLength = 0;
  clock_t start = clock();

  // could open the file once (map it), instead of this complicated mess.
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    printf("Cannot read the input file %s.", filename);
    return EXIT_FAILURE;
  }
  // should be done with an enum:
  bool xor8 = false;
  bool bin16 = false;
  bool bloom12 = false;
  if (fread(&cookie, sizeof(cookie), 1, fp) != 1)
    printf("failed read.\n");
  if (fread(&seed, sizeof(seed), 1, fp) != 1)
    printf("failed read.\n");
  // should be a switch/case.
  if (cookie != 1234569) {
    if(cookie == 1234570) {
      bin16 = true;
    } else  if (cookie == 1234567) {
      xor8 = true;
      if (fread(&BlockLength, sizeof(BlockLength), 1, fp) != 1)
        printf("failed read.\n");

    } else if (cookie == 1234567 + 1) {
      bloom12 = true;
      if (fread(&BlockLength, sizeof(BlockLength), 1, fp) != 1)
        printf("failed read.\n");

    } else {
      printf("Not a filter file. Cookie found: %llu.\n", (long long unsigned int)cookie);
      return EXIT_FAILURE;
    }
  }
  size_t length = 0;
  binary_fuse8_t binfilter;
  binary_fuse16_t binfilter16;

  if (bloom12) {
    length = BlockLength * sizeof(uint64_t) + 3 * sizeof(uint64_t);
  } else if (xor8) {
    length = 3 * BlockLength * sizeof(uint8_t) + 3 * sizeof(uint64_t);
  } else if(bin16) {
    bool isok = true;
    binfilter16.Seed = seed;
    isok &=
        fread(&binfilter16.SegmentLength, sizeof(binfilter16.SegmentLength), 1, fp);
    isok &=
        fread(&binfilter16.SegmentLengthMask, sizeof(binfilter16.SegmentLengthMask), 1, fp);
    isok &=
        fread(&binfilter16.SegmentCount, sizeof(binfilter16.SegmentCount), 1, fp);
    isok &= fread(&binfilter16.SegmentCountLength,
                  sizeof(binfilter16.SegmentCountLength), 1, fp);
    isok &= fread(&binfilter16.ArrayLength, sizeof(binfilter16.ArrayLength), 1, fp);
    if (!isok)
      printf("failed read.\n");
    length =
        sizeof(cookie) + sizeof(binfilter16.Seed) +
        sizeof(binfilter16.SegmentLength) + sizeof(binfilter16.SegmentLengthMask) +
        sizeof(binfilter16.SegmentCount) + sizeof(binfilter16.SegmentCountLength) +
        sizeof(binfilter16.ArrayLength) + sizeof(uint16_t) * binfilter16.ArrayLength;

  } else {
    bool isok = true;
    binfilter.Seed = seed;
    isok &=
        fread(&binfilter.SegmentLength, sizeof(binfilter.SegmentLength), 1, fp);
    isok &=
        fread(&binfilter.SegmentLengthMask, sizeof(binfilter.SegmentLengthMask), 1, fp);
    isok &=
        fread(&binfilter.SegmentCount, sizeof(binfilter.SegmentCount), 1, fp);
    isok &= fread(&binfilter.SegmentCountLength,
                  sizeof(binfilter.SegmentCountLength), 1, fp);
    isok &= fread(&binfilter.ArrayLength, sizeof(binfilter.ArrayLength), 1, fp);
    if (!isok)
      printf("failed read.\n");
    length =
        sizeof(cookie) + sizeof(binfilter.Seed) +
        sizeof(binfilter.SegmentLength) + sizeof(binfilter.SegmentLengthMask) +
        sizeof(binfilter.SegmentCount) + sizeof(binfilter.SegmentCountLength) +
        sizeof(binfilter.ArrayLength) + sizeof(uint8_t) * binfilter.ArrayLength;
  }

  if (bloom12)
    printf("Bloom filter detected.\n");
  else if (xor8)
    printf("Xor filter detected.\n");
  else if(bin16)
    printf("16-bit binary fuse filter detected.\n");
  else
    printf("8-bit binary fuse filter detected.\n");
  fclose(fp);
  int fd = open(filename, O_RDONLY);
  bool shared = false;

  printf("I expect the file to span %zu bytes.\n", length);
// for Linux:
#if defined(__linux__) && defined(USE_POPULATE)
  uint8_t *addr = (uint8_t *)(mmap(
      NULL, length, PROT_READ,
      MAP_FILE | (shared ? MAP_SHARED : MAP_PRIVATE) | MAP_POPULATE, fd, 0));
#else
  uint8_t *addr =
      (uint8_t *)(mmap(NULL, length, PROT_READ,
                       MAP_FILE | (shared ? MAP_SHARED : MAP_PRIVATE), fd, 0));
#endif
  if (addr == MAP_FAILED) {
    printf("Data can't be mapped???\n");
    return EXIT_FAILURE;
  } else {
    printf("memory mapping is a success.\n");
  }
  if (bloom12) {
    MappeableBloomFilter<12> filter(
        BlockLength, seed, (const uint64_t *)(addr + 3 * sizeof(uint64_t)));
    if (filter.Contain(hexval)) {
      printf("Probably in the set.\n");
    } else {
      printf("Surely not in the set.\n");
    }
  } else if (xor8) {
    xor8_t filter;
    filter.seed = seed;
    filter.blockLength = BlockLength;
    filter.fingerprints = addr + 3 * sizeof(uint64_t);
    if (xor8_contain(hexval, &filter)) {
      printf("Probably in the set.\n");
    } else {
      printf("Surely not in the set.\n");
    }
  } else if (bin16) {
    binfilter16.Fingerprints = reinterpret_cast<uint16_t*>(
        addr + sizeof(cookie) + sizeof(binfilter16.Seed) +
        sizeof(binfilter16.SegmentLength) + sizeof(binfilter16.SegmentLengthMask) +
        sizeof(binfilter16.SegmentCount) + sizeof(binfilter16.SegmentCountLength) +
        sizeof(binfilter16.ArrayLength));
    if (binary_fuse16_contain(hexval, &binfilter16)) {
      printf("Probably in the set.\n");
    } else {
      printf("Surely not in the set.\n");
    }
  } else {
    binfilter.Fingerprints =
        addr + sizeof(cookie) + sizeof(binfilter.Seed) +
        sizeof(binfilter.SegmentLength) + sizeof(binfilter.SegmentLengthMask) +
        sizeof(binfilter.SegmentCount) + sizeof(binfilter.SegmentCountLength) +
        sizeof(binfilter.ArrayLength);
    if (binary_fuse8_contain(hexval, &binfilter)) {
      printf("Probably in the set.\n");
    } else {
      printf("Surely not in the set.\n");
    }
  }
  clock_t end = clock();

  printf("Processing time %.3f microseconds.\n",
         (float)(end - start) * 1000.0 * 1000.0 / CLOCKS_PER_SEC);
  printf("Expected number of queries per second: %.3f \n",
         (float)CLOCKS_PER_SEC / (end - start));
  munmap(addr, length);
  return EXIT_SUCCESS;
}

#include "util.h"
#include <cstdint>
#include <inttypes.h>
#include <limits.h>
#include <sstream>
inline uint64_t getBit(uint32_t index) { return 1L << (index & 63); }

template <int k> class MappeableBloomFilter {
public:
  size_t arrayLength;
  const uint64_t *data;
  MixSplit hasher;

  explicit MappeableBloomFilter(const size_t arrayLength, const uint64_t seed,
                                const uint64_t *fps)
      : arrayLength(arrayLength), data(fps), hasher(seed) {}

  // Report if the item is inserted, with false positive rate.
  bool Contain(const uint64_t key) const {
    uint64_t hash = hasher(key);
    uint32_t a = (uint32_t)(hash >> 32);
    uint32_t b = (uint32_t)hash;
    for (int i = 0; i < k; i++) {
      if ((data[reduce(a, this->arrayLength)] & getBit(a)) == 0) {
        return false;
      }
      a += b;
    }
    return true;
  }
};

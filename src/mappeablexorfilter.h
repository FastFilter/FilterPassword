#include "util.h"
#include <cstdint>
#include <inttypes.h>
#include <limits.h>
#include <sstream>

template <class fingertype = uint8_t> class MappeableXorFilter {
public:
  size_t blockLength;
  const fingertype *fingerprints;
  MixSplit hasher;

  explicit MappeableXorFilter(const size_t BlockLength, const uint64_t seed,
                              const fingertype *fps)
      : blockLength(BlockLength), fingerprints(fps), hasher(seed) {}

  // Report if the item is inserted, with false positive rate.
  bool Contain(const uint64_t key) const {
    uint64_t hash = hasher(key);
    uint64_t f = fingerprint(hash);
    uint32_t r0 = (uint32_t)hash;
    uint32_t r1 = (uint32_t)rotl64(hash, 21);
    uint32_t r2 = (uint32_t)rotl64(hash, 42);
    uint32_t h0 = reduce(r0, blockLength);
    uint32_t h1 = reduce(r1, blockLength) + blockLength;
    uint32_t h2 = reduce(r2, blockLength) + 2 * blockLength;
    f ^= fingerprints[h0] ^ fingerprints[h1] ^ fingerprints[h2];
    return f == 0;
  }

  inline fingertype fingerprint(const uint64_t hash) const {
    return (fingertype)hash ^ (hash >> 32);
  }
};

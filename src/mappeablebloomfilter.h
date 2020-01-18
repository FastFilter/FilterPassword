#include "util.h"
#include <cstdint>
#include <inttypes.h>
#include <limits.h>
#include <sstream>
#include <cmath>

namespace {
static inline size_t getBestK(size_t bitsPerItem) {
  return std::max(1, (int)round((double)bitsPerItem * log(2)));
}
}



/**
* Given a value "word", produces an integer in [0,p) without division.
* The function is as fair as possible in the sense that if you iterate
* through all possible values of "word", then you will generate all
* possible outputs as uniformly as possible.
*/
static inline uint32_t fastrange32(uint32_t word, uint32_t p) {
  // http://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
	return (uint32_t)(((uint64_t)word * (uint64_t)p) >> 32);
}

#if defined(_MSC_VER) && defined (_WIN64)
#include <intrin.h>// should be part of all recent Visual Studio
#pragma intrinsic(_umul128)
#endif // defined(_MSC_VER) && defined (_WIN64)


/**
* Given a value "word", produces an integer in [0,p) without division.
* The function is as fair as possible in the sense that if you iterate
* through all possible values of "word", then you will generate all
* possible outputs as uniformly as possible.
*/
static inline uint64_t fastrange64(uint64_t word, uint64_t p) {
  // http://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
#ifdef __SIZEOF_INT128__ // then we know we have a 128-bit int
	return (uint64_t)(((__uint128_t)word * (__uint128_t)p) >> 64);
#elif defined(_MSC_VER) && defined(_WIN64)
	// supported in Visual Studio 2005 and better
	uint64_t highProduct;
	_umul128(word, p, &highProduct); // ignore output
	return highProduct;
	unsigned __int64 _umul128(
		unsigned __int64 Multiplier,
		unsigned __int64 Multiplicand,
		unsigned __int64 *HighProduct
	);
#else
	return word % p; // fallback
#endif // __SIZEOF_INT128__
}


#ifndef UINT32_MAX
#define UINT32_MAX  (0xffffffff)
#endif // UINT32_MAX

/**
* Given a value "word", produces an integer in [0,p) without division.
* The function is as fair as possible in the sense that if you iterate
* through all possible values of "word", then you will generate all
* possible outputs as uniformly as possible.
*/
static inline size_t fastrangesize(uint64_t word, size_t p) {
#if (SIZE_MAX == UINT32_MAX)
	return (size_t)fastrange32(word, p);
#else // assume 64-bit
	return (size_t)fastrange64(word, p);
#endif // SIZE_MAX == UINT32_MAX
}

inline uint64_t getBit(uint32_t index) { return 1L << (index & 63); }

template <int bitsPerItem> class MappeableBloomFilter {
public:
  size_t arrayLength;
  const uint64_t *data;
  MixSplit hasher;
  int k;

  explicit MappeableBloomFilter(const size_t arrayLength, const uint64_t seed,
                                const uint64_t *fps)
      : arrayLength(arrayLength), data(fps), hasher(seed), k(getBestK(bitsPerItem)) {}

  // Report if the item is inserted, with false positive rate.
  bool Contain(const uint64_t key) const {
    uint64_t hash = hasher(key);
    uint64_t a = (hash >> 32) | (hash << 32);;
    uint64_t b = hash;
    for (int i = 0; i < k; i++) {
      if ((data[fastrangesize(a, this->arrayLength)] & getBit(a)) == 0) {
        return false;
      }
      a += b;
    }
    return true;
  }
};

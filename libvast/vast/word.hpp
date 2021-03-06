#ifndef VAST_WORD_HPP
#define VAST_WORD_HPP

#include <cstddef>
#include <limits>

#include "vast/detail/assert.hpp"

namespace vast {

/// A fixed-size piece unsigned piece of data that supports various bitwise
/// operations.
template <class T>
struct word {
  static_assert(std::is_unsigned<T>::value && std::is_integral<T>::value,
                "bitwise operations require unsigned integral, types");

  // -- general ---------------------------------------------------------------

  /// The underlying block type.
  using value_type = T;

  /// The type to represent sizes.
  using size_type = size_t;

  /// The number of bits per block (aka. word size).
  static constexpr size_type width = std::numeric_limits<value_type>::digits;

  /// A value that represents an invalid or "not found" position.
  static constexpr size_type npos = ~size_type{0};

  // -- special block values --------------------------------------------------

  /// A block with all 0s.
  static constexpr value_type none = value_type{0};

  /// A block with all 1s.
  static constexpr value_type all = ~none;

  /// A block with only an MSB of 0.
  static constexpr value_type msb0 = all >> 1;

  /// A block with only an MSB of 1.
  static constexpr value_type msb1 = ~msb0;

  /// A block with only an LSB of 1.
  static constexpr value_type lsb1 = value_type{1};

  /// A block with only an LSB of 0.
  static constexpr value_type lsb0 = ~lsb1;

  // -- masks -----------------------------------------------------------------

  /// Computes a bitmask for a given position.
  /// @param i The position where the 1-bit should be.
  /// @return `1 << i`
  /// @pre `i < width`
  static constexpr value_type mask(size_type i) {
    return lsb1 << i;
  }

  /// Computes a bitmask with only the *i* least significant bits set to 1.
  /// @param i The number least significant bits to set to 1.
  /// @returns `~(all << i)`
  /// @pre `i < width`
  static constexpr value_type lsb_mask(size_type i) {
    return ~(all << i);
  }

  /// Computes a bitmask with only the *i* least significant bits set to 1.
  /// @param i The number least significant bits to set to 1.
  /// @returns `all >> (width - i)`
  /// @pre `i > 0 && i <= width`
  static constexpr value_type lsb_fill(size_type i) {
    return all >> (width - i);
  }

  /// Computes a bitmask with only the *i* most significant bits set to 1.
  /// @param i The number most significant bits to set to 1.
  /// @returns `~(all << i)`
  /// @pre `i < width`
  static constexpr value_type msb_mask(size_type i) {
    return ~(all >> i);
  }

  /// Computes a bitmask with only the *i* most significant bits set to 1.
  /// @param i The number most significant bits to set to 1.
  /// @returns `all << (width - i)`
  /// @pre `i > 0 && i <= width`
  static constexpr value_type msb_fill(size_type i) {
    return all << (width - i);
  }

  // -- tests -----------------------------------------------------------------

  /// Tests whether a block is either all 0 or all 1.
  /// @param x The block to test.
  /// @returns `x == all || x == none`
  static constexpr bool all_or_none(value_type x) {
    return ((x + 1) & lsb0) <= 1;
  }

  /// Tests whether the *k* least signficant bits block are all 0 or all 1.
  /// @param x The block to test.
  /// @param k The number of least significant bits to consider.
  /// @returns `x & lsb_mask(k) == all || x & lsb_mask(k) == none`
  /// @pre `k < width`
  static constexpr bool all_or_none(value_type x, size_type k) {
    return ((x + 1) & lsb_mask(k)) <= 1;
  }

  /// Extracts the *i*-th bit in a block.
  /// @param x The block to test.
  /// @param i The bit to extract.
  /// @returns The value at position *i*, counted from the LSB.
  /// @pre `i < width`
  static constexpr bool test(value_type x, size_type i) {
    return (x & mask(i)) == mask(i);
  }

  // -- manipulation ----------------------------------------------------------

  /// Flips a bit in a block at a given position.
  /// @param x The block to flip a bit in.
  /// @param i The position to flip.
  /// @returns `x ^ (1 << i)`
  /// @pre `i < width`
  static constexpr value_type flip(value_type x, size_type i) {
    return x ^ mask(i);
  }

  /// Sets a specific bit in a block to 0 or 1.
  /// @param x The block to set the bit in.
  /// @param i The position to set.
  /// @pre `i < width`
  static constexpr value_type set(value_type x, size_type i, bool b) {
    return b ? x | mask(i) : x & ~mask(i);
  }

  // -- counting --------------------------------------------------------------

  template <class B>
  using enable_if_32 = std::enable_if_t<(word<B>::width <= 32), B>;

  template <class B>
  using enable_if_64 = std::enable_if_t<(word<B>::width == 64), B>;

  /// Counts the number of trailing zeros.
  /// @param x The block value.
  /// @returns The number trailing zeros in *x*.
  /// @pre `x > 0`
  template <class B = value_type>
  static constexpr auto count_trailing_zeros(value_type x) -> enable_if_32<B> {
    return __builtin_ctz(x);
  }

  template <class B = value_type>
  static constexpr auto count_trailing_zeros(value_type x) -> enable_if_64<B> {
    return __builtin_ctzll(x);
  }

  /// Counts the number of trailing ones.
  /// @param x The block value.
  /// @returns The number trailing ones in *x*.
  /// @pre `x > 0`
  static constexpr value_type count_trailing_ones(value_type x) {
    return count_trailing_zeros(~x);
  }

  /// Counts the number of leading zeros.
  /// @param x The block value.
  /// @returns The number leading zeros in *x*.
  /// @pre `x > 0`
  template <class B = value_type>
  static constexpr auto count_leading_zeros(value_type x) -> enable_if_32<B> {
    // The compiler builtin always assumes a width of 32 bits. We have to adapt
    // the return value according to the actual block width.
    return __builtin_clz(x) - (32 - width);
  }

  template <class B = value_type>
  static constexpr auto count_leading_zeros(value_type x) -> enable_if_64<B> {
    return __builtin_clzll(x);
  }

  /// Counts the number of leading ones.
  /// @param x The block value.
  /// @returns The number leading ones in *x*.
  /// @pre `x > 0`
  static constexpr value_type count_leading_ones(value_type x) {
    return count_leading_zeros(~x);
  }

  /// Counts the number of set bits
  /// @param x The block value.
  /// @returns The number of set bits in *x*.
  /// @pre `x > 0`
  template <class B = value_type>
  static constexpr auto popcount(value_type x) -> enable_if_32<B> {
    return __builtin_popcount(x);
  }

  template <class B = value_type>
  static constexpr auto popcount(value_type x) -> enable_if_64<B> {
    return __builtin_popcountll(x);
  }

  /// Computes *rank_i* of a block, i.e., the number of 1-bits up to and
  /// including position *i*, counted from the LSB.
  /// @param x The block to compute the rank for.
  /// @param i The position up to where to count.
  /// @returns *rank_i(x)*.
  /// @pre `i < width`
  static constexpr size_type rank(value_type x, size_type i) {
    auto masked = x & lsb_fill(i + 1);
    return masked == 0 ? 0 : popcount(masked);
  }

  /// Computes the parity of a block, i.e., the number of 1-bits modulo 2.
  /// @param x The block value.
  /// @returns The parity of *x*.
  /// @pre `x > 0`
  template <class B = value_type>
  static constexpr auto parity(value_type x) -> enable_if_32<B> {
    return __builtin_parity(x);
  }

  template <class B = value_type>
  static constexpr auto parity(value_type x) -> enable_if_64<B> {
    return __builtin_parityll(x);
  }

  // -- search ----------------------------------------------------------------

  /// Find the next 1-bit starting at position relative to the LSB.
  /// @param x The block to search.
  /// @param i The position relative to the LSB to start searching.
  static constexpr size_type next(value_type x, size_type i) {
    if (i == width - 1)
      return npos;
    auto top = x & (all << (i + 1));
    return top == 0 ? npos : count_trailing_zeros(top);
  }

  /// Find the previous 1-bit starting at position relative to the LSB.
  /// @param x The block to search.
  /// @param i The position relative to the LSB to start searching.
  /// @pre `i < width`
  static constexpr size_type prev(value_type x, size_type i) {
    if (i == 0)
      return npos;
    auto bottom = x & ~(all << i);
    return bottom == 0 ? npos : width - count_leading_zeros(bottom) - 1;
  }

  /// Computes the position of the i-th occurrence of a 1-bit.
  /// @param x The block to search.
  /// @param i The position of the *i*-th occurrence of 1 in *b*.
  /// @pre `i > 0 && i <= width`
  static constexpr size_type select(value_type x, size_type i) {
    // TODO: make this efficient and branch-free. There is one implementation
    // that counts from the right for 64-bit here:
    // http://graphics.stanford.edu/~seander/bithacks.html
    auto cum = 0u;
    for (auto j = 0u; j < width; ++j)
      if (test(x, j))
        if (++cum == i)
          return j;
    return npos;
  }

  // -- math ------------------------------------------------------------------

  /// Computes the binary logarithm (*log2*) for a given block.
  /// @param x The block value.
  /// @returns `log2(x)`
  /// @pre `x > 0`
  static constexpr value_type log2(value_type x) {
    return width - count_leading_zeros(x) - 1;
  }
};

template <class T>
constexpr typename word<T>::size_type word<T>::width;

template <class T>
constexpr typename word<T>::size_type word<T>::npos;

template <class T>
constexpr typename word<T>::value_type word<T>::none;

template <class T>
constexpr typename word<T>::value_type word<T>::all;

template <class T>
constexpr typename word<T>::value_type word<T>::msb0;

template <class T>
constexpr typename word<T>::value_type word<T>::msb1;

template <class T>
constexpr typename word<T>::value_type word<T>::lsb0;

template <class T>
constexpr typename word<T>::value_type word<T>::lsb1;

} // namespace vast

#endif

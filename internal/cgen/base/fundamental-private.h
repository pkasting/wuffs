// After editing this file, run "go generate" in the ../data directory.

// Copyright 2017 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// ---------------- Fundamentals

// WUFFS_BASE__MAGIC is a magic number to check that initializers are called.
// It's not foolproof, given C doesn't automatically zero memory before use,
// but it should catch 99.99% of cases.
//
// Its (non-zero) value is arbitrary, based on md5sum("wuffs").
#define WUFFS_BASE__MAGIC ((uint32_t)0x3CCB6C71)

// WUFFS_BASE__DISABLED is a magic number to indicate that a non-recoverable
// error was previously encountered.
//
// Its (non-zero) value is arbitrary, based on md5sum("disabled").
#define WUFFS_BASE__DISABLED ((uint32_t)0x075AE3D2)

// Denote intentional fallthroughs for -Wimplicit-fallthrough.
//
// The two #if lines are deliberately separate. Combining the two conditions
// into a single "#if foo && bar" line would not be portable. See
// https://gcc.gnu.org/onlinedocs/cpp/_005f_005fhas_005fattribute.html
#if defined(__has_attribute)
#if __has_attribute(fallthrough)
#define WUFFS_BASE__FALLTHROUGH __attribute__((fallthrough))
#else
#define WUFFS_BASE__FALLTHROUGH
#endif
#else
#define WUFFS_BASE__FALLTHROUGH
#endif

// Use switch cases for coroutine suspension points, similar to the technique
// in https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
//
// We use trivial macros instead of an explicit assignment and case statement
// so that clang-format doesn't get confused by the unusual "case"s.
#define WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0 case 0:;
#define WUFFS_BASE__COROUTINE_SUSPENSION_POINT(n) \
  coro_susp_point = n;                            \
  WUFFS_BASE__FALLTHROUGH;                        \
  case n:;

#define WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(n) \
  if (!status.repr) {                                           \
    goto ok;                                                    \
  } else if (*status.repr != '$') {                             \
    goto exit;                                                  \
  }                                                             \
  coro_susp_point = n;                                          \
  goto suspend;                                                 \
  case n:;

// Clang also defines "__GNUC__".
#if defined(__GNUC__)
#define WUFFS_BASE__LIKELY(expr) (__builtin_expect(!!(expr), 1))
#define WUFFS_BASE__UNLIKELY(expr) (__builtin_expect(!!(expr), 0))
#else
#define WUFFS_BASE__LIKELY(expr) (expr)
#define WUFFS_BASE__UNLIKELY(expr) (expr)
#endif

// --------

static inline wuffs_base__empty_struct  //
wuffs_base__ignore_status(wuffs_base__status z) {
  return wuffs_base__make_empty_struct();
}

static inline wuffs_base__status  //
wuffs_base__status__ensure_not_a_suspension(wuffs_base__status z) {
  if (z.repr && (*z.repr == '$')) {
    z.repr = wuffs_base__error__cannot_return_a_suspension;
  }
  return z;
}

// --------

// wuffs_base__iterate_total_advance returns the exclusive pointer-offset at
// which iteration should stop. The overall slice has length total_len, each
// iteration's sub-slice has length iter_len and are placed iter_advance apart.
//
// The iter_advance may not be larger than iter_len. The iter_advance may be
// smaller than iter_len, in which case the sub-slices will overlap.
//
// The return value r satisfies ((0 <= r) && (r <= total_len)).
//
// For example, if total_len = 15, iter_len = 5 and iter_advance = 3, there are
// four iterations at offsets 0, 3, 6 and 9. This function returns 12.
//
// 0123456789012345
// [....]
//    [....]
//       [....]
//          [....]
//             $
// 0123456789012345
//
// For example, if total_len = 15, iter_len = 5 and iter_advance = 5, there are
// three iterations at offsets 0, 5 and 10. This function returns 15.
//
// 0123456789012345
// [....]
//      [....]
//           [....]
//                $
// 0123456789012345
static inline size_t  //
wuffs_base__iterate_total_advance(size_t total_len,
                                  size_t iter_len,
                                  size_t iter_advance) {
  if (total_len >= iter_len) {
    size_t n = total_len - iter_len;
    return ((n / iter_advance) * iter_advance) + iter_advance;
  }
  return 0;
}

// ---------------- Numeric Types

extern const uint8_t wuffs_base__low_bits_mask__u8[8];
extern const uint16_t wuffs_base__low_bits_mask__u16[16];
extern const uint32_t wuffs_base__low_bits_mask__u32[32];
extern const uint64_t wuffs_base__low_bits_mask__u64[64];

#define WUFFS_BASE__LOW_BITS_MASK__U8(n) (wuffs_base__low_bits_mask__u8[n])
#define WUFFS_BASE__LOW_BITS_MASK__U16(n) (wuffs_base__low_bits_mask__u16[n])
#define WUFFS_BASE__LOW_BITS_MASK__U32(n) (wuffs_base__low_bits_mask__u32[n])
#define WUFFS_BASE__LOW_BITS_MASK__U64(n) (wuffs_base__low_bits_mask__u64[n])

// --------

static inline void  //
wuffs_base__u8__sat_add_indirect(uint8_t* x, uint8_t y) {
  *x = wuffs_base__u8__sat_add(*x, y);
}

static inline void  //
wuffs_base__u8__sat_sub_indirect(uint8_t* x, uint8_t y) {
  *x = wuffs_base__u8__sat_sub(*x, y);
}

static inline void  //
wuffs_base__u16__sat_add_indirect(uint16_t* x, uint16_t y) {
  *x = wuffs_base__u16__sat_add(*x, y);
}

static inline void  //
wuffs_base__u16__sat_sub_indirect(uint16_t* x, uint16_t y) {
  *x = wuffs_base__u16__sat_sub(*x, y);
}

static inline void  //
wuffs_base__u32__sat_add_indirect(uint32_t* x, uint32_t y) {
  *x = wuffs_base__u32__sat_add(*x, y);
}

static inline void  //
wuffs_base__u32__sat_sub_indirect(uint32_t* x, uint32_t y) {
  *x = wuffs_base__u32__sat_sub(*x, y);
}

static inline void  //
wuffs_base__u64__sat_add_indirect(uint64_t* x, uint64_t y) {
  *x = wuffs_base__u64__sat_add(*x, y);
}

static inline void  //
wuffs_base__u64__sat_sub_indirect(uint64_t* x, uint64_t y) {
  *x = wuffs_base__u64__sat_sub(*x, y);
}

// ---------------- Slices and Tables

// wuffs_base__slice_u8__prefix returns up to the first up_to bytes of s.
static inline wuffs_base__slice_u8  //
wuffs_base__slice_u8__prefix(wuffs_base__slice_u8 s, uint64_t up_to) {
  if (((uint64_t)(s.len)) > up_to) {
    s.len = ((size_t)up_to);
  }
  return s;
}

// wuffs_base__slice_u8__suffix returns up to the last up_to bytes of s.
static inline wuffs_base__slice_u8  //
wuffs_base__slice_u8__suffix(wuffs_base__slice_u8 s, uint64_t up_to) {
  if (((uint64_t)(s.len)) > up_to) {
    s.ptr += ((uint64_t)(s.len)) - up_to;
    s.len = ((size_t)up_to);
  }
  return s;
}

// wuffs_base__slice_u8__copy_from_slice calls memmove(dst.ptr, src.ptr, len)
// where len is the minimum of dst.len and src.len.
//
// Passing a wuffs_base__slice_u8 with all fields NULL or zero (a valid, empty
// slice) is valid and results in a no-op.
static inline uint64_t  //
wuffs_base__slice_u8__copy_from_slice(wuffs_base__slice_u8 dst,
                                      wuffs_base__slice_u8 src) {
  size_t len = dst.len < src.len ? dst.len : src.len;
  if (len > 0) {
    memmove(dst.ptr, src.ptr, len);
  }
  return len;
}

// --------

static inline wuffs_base__slice_u8  //
wuffs_base__table_u8__row(wuffs_base__table_u8 t, uint32_t y) {
  if (y < t.height) {
    return wuffs_base__make_slice_u8(t.ptr + (t.stride * y), t.width);
  }
  return wuffs_base__make_slice_u8(NULL, 0);
}

// ---------------- Slices and Tables (Utility)

#define wuffs_base__utility__empty_slice_u8 wuffs_base__empty_slice_u8

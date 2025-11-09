#pragma once

#include <cstdint>

/* Unsigned integers */
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

/* Signed integers */
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

/* Floating point numbers */
using f32 = float;
using f64 = double;

/* 2D integer size. */
struct Size2D {
    u32 x = 0u, y = 0u;
};

/* 3D integer size. */
struct Size3D {
    u32 x = 0u, y = 0u, z = 0u;
    inline bool is_2d() const { return z == 0u; };
};

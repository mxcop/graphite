#pragma once

#include "graphite/utils/types.hh"

/* Sampler filter. */
enum class Filter : u32 {
    Nearest = 0u,
    Linear  = 1u,
};

/* Sampler addressing mode. */
enum class AddressMode : u32 {
    Repeat = 0u, /* The UV coordinates are repeated outside the [0..1] range. (mod(uv, 1.0)) */
    MirrorRepeat  = 1u, /* The UV coordinates are repeated & mirrored outside the [0..1] range. */
    ClampToEdge   = 2u, /* The UV coordinates are clamped within the [0..1] range. */
    ClampToBorder = 3u, /* Any UV coordinates outside the [0..1] range return the border color. */
};

/* Sampler border color. (relevant for AddressMode::eClampToBorder) */
enum class BorderColor : u32 {
    RGB0A0_Float = 0u, /* Black color, transparent, floating-point format. */
    RGB0A0_Int   = 1u, /* Black color, transparent, integer format. */
    RGB0A1_Float = 2u, /* Black color, opaque, floating-point format. */
    RGB0A1_Int   = 3u, /* Black color, opaque, integer format. */
    RGB1A1_Float = 4u, /* White color, opaque, floating-point format. */
    RGB1A1_Int   = 5u, /* White color, opaque, integer format. */
};

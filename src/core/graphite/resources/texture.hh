#pragma once

#include "graphite/utils/enum_flags.hh"
#include "graphite/utils/types.hh"

/* Texture usage flags. */
enum class TextureUsage : u32 {
    Invalid = 0u,               /* Invalid texture usage. */
    TransferDst = 1u << 1u,     /* This texture can be a GPU transfer destination. */
    TransferSrc = 1u << 2u,     /* This texture can be a GPU transfer source. */
    Sampled = 1u << 3u,         /* Texture can be sampled using hardware samplers. */
    Storage = 1u << 4u,         /* GPU write-able texture. */
    ColorAttachment = 1u << 5u, /* This texture can be bound to a raster pass as a color attachment. */
    DepthStencil = 1u << 6u,    /* This texture can be bound to a raster pass as depth attachment. */
};
ENUM_CLASS_FLAGS(TextureUsage);

/* Texture formats. */
enum class TextureFormat : u32 {
    Invalid = 0u,  /* Invalid texture format. */
    RGBA8Unorm,    /* RGBA 8 bits per channel, unsigned normalized. */
    RG32Uint,      /* RG 32 bits per channel, unsigned integer. */
    RGBA16Sfloat,  /* RGBA 16 bits per channel, signed float. */
    RG11B10Ufloat, /* RG 11 bits per channel, B 10 bits per channel, unsigned float. */
    R32Sfloat,     /* R 32 bits, signed float. */
    D32Sfloat,     /* Depth 32 bits, signed float. */
    EnumLimit      /* Anything above or equal is invalid. */
};

/* Texture meta data. */
struct TextureMeta {
    u32 mips = 1u; /* Mip levels. */
    u32 arrays = 1u; /* Array layers. */
};

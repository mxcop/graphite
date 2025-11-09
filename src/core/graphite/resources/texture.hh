#pragma once

#include "graphite/utils/enum_flags.hh"
#include "graphite/utils/types.hh"

/* Texture usage flags. */
enum class TextureUsage : u32 {
    Invalid = 0u,           /* Invalid texture usage. */
    TransferDst = 1u << 1u, /* This texture can be a GPU transfer destination. */
    TransferSrc = 1u << 2u, /* This texture can be a GPU transfer source. */
    Sampled = 1u << 3u,     /* Texture can be sampled using hardware samplers. */
    Storage = 1u << 4u,     /* GPU write-able texture. */
    ColorAttachment = 1u << 5u, /* This texture can be bound to a graphics pass as a color attachment. */
};
ENUM_CLASS_FLAGS(TextureUsage);

/* Texture formats. */
enum class TextureFormat : u32 {
    Invalid = 0u, /* Invalid texture format. */
    RGBA8Unorm,   /* RGBA 8 bits per channel, unsigned normalized. */
    EnumLimit     /* Anything above or equal is invalid. */
};

/* Texture meta data. */
struct TextureMeta {
    u32 mips = 1u; /* Mip levels. */
    u32 arrays = 1u; /* Array layers. */
};

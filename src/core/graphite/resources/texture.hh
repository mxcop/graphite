#pragma once

#include "graphite/utils/enum_flags.hh"
#include "graphite/utils/types.hh"

/* Texture usage flags. */
enum class TextureUsage : u32 {
    Invalid = 0u,           /* Invalid buffer usage. */
    TransferDst = 1u << 1u, /* This buffer can be a GPU transfer destination. */
    TransferSrc = 1u << 2u, /* This buffer can be a GPU transfer source. */
    Storage = 1u << 3u,     /* Read-write texture */
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

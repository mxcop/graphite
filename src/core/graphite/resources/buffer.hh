#pragma once

#include "graphite/utils/enum_flags.hh"
#include "graphite/utils/types.hh"

/* Buffer Usage Flags */
enum class BufferUsage : u32 {
    eInvalid = 0u, /* Invalid buffer usage. */
    eTransferDst = 1u << 1u,  /* This buffer can be a GPU transfer destination. */
    eTransferSrc = 1u << 2u,  /* This buffer can be a GPU transfer source. */
    eConstant = 1u << 3u,     /* Read-only buffer, aka. `UniformBuffer` */
    //eStorage = 1u << 2u,      /* Read/Write buffer, aka. `StructuredBuffer` */
};
ENUM_CLASS_FLAGS(BufferUsage);
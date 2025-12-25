#pragma once

#include "graphite/utils/enum_flags.hh"
#include "graphite/utils/types.hh"

/* Buffer usage flags. */
enum class BufferUsage : u32 {
    Invalid = 0u,            /* Invalid buffer usage. */
    TransferDst = 1u << 1u,  /* This buffer can be a GPU transfer destination. */
    TransferSrc = 1u << 2u,  /* This buffer can be a GPU transfer source. */
    Constant = 1u << 3u,     /* Read-only buffer, aka. `UniformBuffer` */
    Storage = 1u << 4u,      /* Read/Write buffer, aka. `StructuredBuffer` */
    Vertex = 1u << 5u,       /* Vertex buffer */
    Indirect = 1u << 6u,     /* Draw/Dispatch Indirect Commands */
};
ENUM_CLASS_FLAGS(BufferUsage);

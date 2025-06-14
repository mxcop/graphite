#pragma once

#include "graphite/utils/types.hh"

/* Render Graph Resource Type. */
enum class ResourceType : u32 {
    Invalid = 0u,
    Buffer = 1u,
    Texture = 2u,
    Image = 3u,
    Sampler = 4u,
};

/* Render Graph Resource Handle (32 bits) */
struct OpaqueHandle {
    /* Only let the VRAM Bank access the data inside. */ 
    friend class VRAMBank;
private:
    u32         index : 28;
    ResourceType type : 4;

    /* Resource type helper functions */
    inline u32 get_type_uint() const { return static_cast<u32>(type); }
    inline void set_type_uint(const u32 x) { type = static_cast<ResourceType>(x); }
    inline bool is_null() const { return type == ResourceType::Invalid; }
    inline bool is_bindable() const { return type != ResourceType::Invalid && type != ResourceType::Texture; }
};

/* Resource handle which can be bound to a pass. */
struct BindHandle : OpaqueHandle {};

/* Type-safe resource handles. */
struct Buffer  : BindHandle   {}; /* Buffer resource handle. (can be bound) */
struct Texture : OpaqueHandle {}; /* Texture resource handle. (**cannot** be bound, create an `Image` instead) */
struct Image   : BindHandle   {}; /* Image (Texture View) resource handle. (can be bound) */
struct Sampler : BindHandle   {}; /* Sampler resource handle. (can be bound) */

/* Ensure that the handles are 32 bits in size. */
static_assert(sizeof(OpaqueHandle) == 4u);
static_assert(sizeof(BindHandle)   == 4u);
static_assert(sizeof(Buffer)  == 4u);
static_assert(sizeof(Texture) == 4u);
static_assert(sizeof(Image)   == 4u);
static_assert(sizeof(Sampler) == 4u);

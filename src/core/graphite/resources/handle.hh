#pragma once

#include "graphite/utils/types.hh"

/* Render Graph Resource Type. */
enum class ResourceType : u32 {
    Invalid = 0u,
    RenderTarget = 1u,
    Buffer = 2u,
    Texture = 3u,
    Image = 4u,
    Sampler = 5u,
};

/* Render Graph Resource Handle (32 bits) */
struct OpaqueHandle {
    /* Create a NULL handle. */
    OpaqueHandle() = default;

    /* Get the raw handle for debugging. */
    inline u32 raw() const { return index | static_cast<u32>(type); }
    /* Get the type of this resource handle. */
    inline ResourceType get_type() const { return type; }
    /* Returns true if this handle is null. */
    inline bool is_null() const { return type == ResourceType::Invalid; }
    /* Returns true if this handle can be bound as a resource. */
    inline bool is_bindable() const { return type != ResourceType::Invalid && type != ResourceType::Texture; }
    
private:
    u32 index : 24; /* Resource index, at most 16.777.216 resources per Bank. */
    u32 bank : 4;   /* Bank index, at most 16 banks per GPU Adapter. */
    ResourceType type : 4;

    OpaqueHandle(u32 index, u32 bank, ResourceType type) : index(index), bank(bank), type(type) {}

    /* To access the constructor. */
    template<typename, typename, ResourceType>
    friend class Stock;
};

/* Resource handle which can be bound to a pass. */
struct BindHandle : OpaqueHandle {};

/* Type-safe resource handles. */
struct RenderTarget : BindHandle {}; /* Render Target resource handle. (can be bound) */
struct Buffer  : BindHandle   {}; /* Buffer resource handle. (can be bound) */
struct Texture : OpaqueHandle {}; /* Texture resource handle. (**cannot** be bound, create an `Image` instead) */
struct Image   : BindHandle   {}; /* Image (Texture View) resource handle. (can be bound) */
struct Sampler : BindHandle   {}; /* Sampler resource handle. (can be bound) */

/* Ensure that the handles are 32 bits in size. */
static_assert(sizeof(OpaqueHandle) == 4u);
static_assert(sizeof(BindHandle)   == 4u);
static_assert(sizeof(RenderTarget) == 4u);
static_assert(sizeof(Buffer)  == 4u);
static_assert(sizeof(Texture) == 4u);
static_assert(sizeof(Image)   == 4u);
static_assert(sizeof(Sampler) == 4u);

#pragma once

#include "graphite/utils/types.hh"

class AgnVRAMBank;

/* Render Graph Resource Type. */
enum class ResourceType : u32 {
    Invalid = 0u,
    RenderTarget = 1u,
    Buffer = 2u,
    Texture = 3u,
    Image = 4u,
    Sampler = 5u,
};

/* Render Graph Resource Handle. */
struct OpaqueHandle {
    /* Create a NULL handle. */
    OpaqueHandle() = default;

    /* Copy */
    OpaqueHandle(const OpaqueHandle& o) noexcept;
    OpaqueHandle& operator=(const OpaqueHandle& o) noexcept;

    /* Move */
    OpaqueHandle(OpaqueHandle&& o) noexcept;
    OpaqueHandle& operator=(OpaqueHandle&& o) noexcept;

    ~OpaqueHandle();

    /* Get the raw handle for debugging. */
    inline uint32_t raw() const { return index | static_cast<uint32_t>(type); }
    /* Get the index of this resource handle. */
    inline uint32_t get_index() const { return index; }
    /* Get the type of this resource handle. */
    inline ResourceType get_type() const { return type; }
    /* Returns true if this handle is null. */
    inline bool is_null() const { return type == ResourceType::Invalid; }
    /* Returns true if this handle can be bound as a resource. */
    inline bool is_bindable() const { return type != ResourceType::Invalid && type != ResourceType::Texture; }

private:
    /* Resource index, at most 268.435.456 resources of each type per GPU. */
    uint32_t index : 28; 
    /* Resource type, at most 16 unique resource types. */
    ResourceType type : 4;

    AgnVRAMBank* bank = nullptr;

    OpaqueHandle(AgnVRAMBank* bank, u32 index, ResourceType type) : index(index), type(type), bank(bank) {}

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

/* Ensure that the handles are 16 bytes in size. */
// static_assert(sizeof(OpaqueHandle) == 16u);
// static_assert(sizeof(BindHandle)   == 16u);
// static_assert(sizeof(RenderTarget) == 16u);
// static_assert(sizeof(Buffer)  == 16u);
// static_assert(sizeof(Texture) == 16u);
// static_assert(sizeof(Image)   == 16u);
// static_assert(sizeof(Sampler) == 16u);

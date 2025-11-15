#pragma once

#include <cassert>

#include "graphite/utils/types.hh"
#include "handle.hh"

template<typename Slot, typename Handle>
struct StockPair {
    Handle handle {};
    Slot& data;
    StockPair(Handle handle, Slot& data) : handle(handle), data(data) {};
};

/**
 * Stack Pool. (Stock)  
 * Used for efficiently managing resources using handles.
 */
template<typename Slot, typename Handle, ResourceType RType>
class Stock {
    Handle* stack = nullptr; /* Handle stack. */
    Slot* pool = nullptr; /* Slot pool. */
    u32* refs = nullptr; /* Reference counters. */

    u32 stack_ptr = 0u; /* Stack pointer. */
    u32 stack_size = 0u; /* Stack size, same as the pool size. */

    Stock() = default;
    ~Stock() { destroy(); }

    /* No Copy */
    Stock(const Stock&) = delete;
    Stock& operator=(const Stock&) = delete;

    /* Create a Stack Pool of given size. */
    Stock(VRAMBank* bank, const u32 size) : stack(new Handle[size] {}), pool(new Slot[size] {}), refs(new u32[size] {}), stack_size(size) {
        /* Initialize the handle stack */
        for (u32 i = 0u; i < size; ++i) stack[i] = reinterpret_cast<Handle&&>(OpaqueHandle(bank, i + 1u, RType));
    }

    /* Init the Stack Pool, clears out any existing data. */
    void init(VRAMBank* bank, const u32 size) {
        destroy(); /* Free existing resources. */

        /* Allocate the new resources. */
        stack = new Handle[size] {};
        pool = new Slot[size] {};
        refs = new u32[size] {};
        stack_size = size;

        /* Initialize the handle stack */
        for (u32 i = 0u; i < size; ++i) stack[i] = reinterpret_cast<Handle&&>(OpaqueHandle(bank, i + 1u, RType));
    }

    /* Free the Stack Pool resources. */
    void destroy() {
        if (stack != nullptr) delete[] stack;
        if (pool  != nullptr) delete[] pool;
        if (refs  != nullptr) delete[] refs;
        stack_ptr = stack_size = 0u;
        stack = nullptr;
        pool = nullptr;
    }

    /* Pop a new resource off the stack. */
    StockPair<Slot, Handle> pop() {
        assert(stack_ptr < stack_size && "stock overflow!");
        const u32 handle_index = stack_ptr;
        const Handle handle = stack[handle_index];
        refs[handle_index] = 1u;
        stack_ptr += 1u;
        Slot& data = pool[handle.index - 1u];
        return StockPair(handle, data);
    }

    /* Push a resource back onto the stack. (returns a reference to the now open slot) */
    Slot& push(OpaqueHandle& handle) {
        stack[--stack_ptr] = (Handle&)handle;
        Slot& data = pool[handle.index - 1u];
        handle = OpaqueHandle();
        return data;
    }

    /* Increment handle reference counter. */
    void inc_ref(const OpaqueHandle& handle) {
        if (RType == ResourceType::RenderTarget) {
            // printf("inc ref! (%d)\n", refs[handle.index - 1u]);
        }
        refs[handle.index - 1u]++;
    }

    /* Decrement handle reference counter. (returns true if the reference count is 0) */
    bool dec_ref(const OpaqueHandle& handle) {
        if (RType == ResourceType::RenderTarget) {
            // printf("dec ref! (%d)\n", refs[handle.index - 1u]);
        }
        return (--refs[handle.index - 1u]) == 0;
    }

public:
    /* Get a resource slot from its handle. */
    Slot& get(const OpaqueHandle& handle) {
        return pool[handle.index - 1u];
    }

    /* Get a resource slot from its handle. */
    const Slot& get(const OpaqueHandle& handle) const {
        return pool[handle.index - 1u];
    }

    /* To access the constructor. */
    friend class AgnVRAMBank;
    friend class VRAMBank;
};

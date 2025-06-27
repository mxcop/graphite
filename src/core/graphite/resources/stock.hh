#pragma once

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

    u32 stack_ptr = 0u; /* Stack pointer. */
    u32 stack_size = 0u; /* Stack size, same as the pool size. */
    u32 bank_index = 0u; /* VRAM Bank index. */

    Stock() = default;
    ~Stock() { destroy(); }

    /* No Copy */
    Stock(const Stock&) = delete;
    Stock& operator=(const Stock&) = delete;

    /* Create a Stack Pool of given size. */
    Stock(const u32 bank_index, const u32 size) : stack(new Handle[size] {}), pool(new Slot[size] {}), stack_size(size), bank_index(bank_index) {
        /* Initialize the handle stack */
        for (u32 i = 0u; i < size; ++i) stack[i] = reinterpret_cast<Handle&>(OpaqueHandle(i + 1u, bank_index, RType));
    }

    /* Init the Stack Pool, clears out any existing data. */
    void init(const u32 bank_index, const u32 size) {
        destroy(); /* Free existing resources. */

        /* Allocate the new resources. */
        stack = new Handle[size] {};
        pool = new Slot[size] {};
        stack_size = size;
        this->bank_index = bank_index;

        /* Initialize the handle stack */
        for (u32 i = 0u; i < size; ++i) stack[i] = reinterpret_cast<Handle&>(OpaqueHandle(i + 1u, bank_index, RType));
    }

    /* Free the Stack Pool resources. */
    void destroy() {
        if (stack != nullptr) delete[] stack;
        if (pool  != nullptr) delete[] pool;
        stack_ptr = stack_size = 0u;
        pool = nullptr;
        stack = nullptr;
    }

    /* Pop a new resource off the stack. */
    StockPair<Slot, Handle> pop() {
        /* Pop the handle stack */
        const Handle handle = stack[stack_ptr++];
        Slot& data = pool[handle.index - 1u];
        return StockPair(handle, data);
    }

    /* To access the constructor. */
    friend class VRAMBank;
};

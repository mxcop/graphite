#pragma once

#include "graphite/utils/types.hh"
#include "handle.hh"

template<typename T>
struct StockPair {
    OpaqueHandle handle {};
    T& data;
    StockPair(OpaqueHandle handle, T& data) : handle(handle), data(data) {};
};

/**
 * Stack Pool. (Stock)  
 * Used for efficiently managing resources using handles.
 */
template<typename T, ResourceType RT>
class Stock {
    OpaqueHandle* stack = nullptr; /* Handle stack. */
    T* pool = nullptr; /* Data pool. */

    u32 stack_ptr = 0u; /* Stack pointer. */
    u32 stack_size = 0u; /* Stack size, same as the pool size. */

    Stock() = default;
    ~Stock() { destroy(); }

    /* No Copy */
    Stock(const Stock&) = delete;
    Stock& operator=(const Stock&) = delete;

    /* Create a Stack Pool of given size. */
    Stock(const u32 size) : stack(new OpaqueHandle[size] {}), pool(new T[size] {}), stack_size(size) {
        /* Initialize the handle stack */
        for (u32 i = 0u; i < size; ++i) stack[i] = OpaqueHandle(i + 1u, RT);
    }

    /* Init the Stack Pool, clears out any existing data. */
    void init(const u32 size) {
        destroy(); /* Free existing resources. */

        /* Allocate the new resources. */
        stack = new OpaqueHandle[size] {};
        pool = new T[size] {};
        stack_size = size;

        /* Initialize the handle stack */
        for (u32 i = 0u; i < size; ++i) stack[i] = OpaqueHandle(i + 1u, RT);
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
    StockPair<T> pop() {
        /* Pop the handle stack */
        const OpaqueHandle handle = stack[stack_ptr++];
        T& data = pool[handle.index - 1u];
        return StockPair(handle, data);
    }

    /* To access the constructor. */
    friend class VRAMBank;
};

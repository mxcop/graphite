module common;

public struct Pixel {
    public float4 color;
};

public struct FrameData {
    public float time;
    public float win_width;
    public float win_height;
};

/* Bindless descriptors */
[[vk::binding(0, 1)]] __DynamicResource<__DynamicResourceKind.General> bindless_sampled_textures[];
[[vk::binding(1, 1)]] __DynamicResource<__DynamicResourceKind.General> bindless_storage_buffers[];

namespace bindless {

/**
 * Handles are 28 index bits, and 4 type bits.
 * That's why we mask away the 4 type bits.
 * Index 0 signals an invalid handle, that's why we subtract 1.
 */

/* Get a bindless texture with a given handle. */
public T texture_sampled<T>(const uint handle) where T : IOpaqueDescriptor {
    return bindless_sampled_textures[NonUniformResourceIndex((handle & 0x0FFFFFFF) - 1u)].asOpaqueDescriptor<T>();
}

/* Get a bindless storage buffer with a given handle. */
public T texture_storage<T>(const uint handle) where T : IOpaqueDescriptor {
    return bindless_storage_buffers[NonUniformResourceIndex((handle & 0x0FFFFFFF) - 1u)].asOpaqueDescriptor<T>();
}

} // bindless
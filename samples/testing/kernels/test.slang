RWTexture2D<float4> out_texture;

[shader("compute")] /* Compute shader entry point */
[numthreads(16, 16, 1)]
void entry(int2 thread_id: SV_DispatchThreadID) {
    out_texture[thread_id] = float4(1.0, 0.0, 0.0, 1.0);
}


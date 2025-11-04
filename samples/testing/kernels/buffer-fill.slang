struct Pixel {
    float4 color;
};

RWStructuredBuffer<Pixel> buffer;

[shader("compute")] /* Compute shader entry point */
[numthreads(64, 1, 1)]
void entry(int2 thread_id: SV_DispatchThreadID) {
    // Guard against out-of-bounds writes
    if (thread_id.x >= 1440 * 810)
        return;

    buffer[thread_id.x].color = float4(1.0f, 1.0f, 1.0f, 1.0f);
}
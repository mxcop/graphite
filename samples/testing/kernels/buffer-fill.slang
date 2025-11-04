struct Pixel {
    float4 color;
};

RWStructuredBuffer<Pixel> buffer;

[shader("compute")] /* Compute shader entry point */
[numthreads(16, 16, 1)]
void entry(int2 thread_id: SV_DispatchThreadID) {
    uint index = thread_id.x + thread_id.y * 1440;
    float2 uv = thread_id.xy / float2(1440.0f - 1.0f, 810.0f - 1.0f);

    buffer[index].color = float4(uv, 0.0f, 1.0f);
}
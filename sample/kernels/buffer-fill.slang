import common;

ConstantBuffer<FrameData> frame_data;
RWStructuredBuffer<Pixel> buffer;

[shader("compute")] /* Compute shader entry point */
[numthreads(16, 16, 1)]
void entry(int2 thread_id: SV_DispatchThreadID) {
    uint index = thread_id.x + thread_id.y * (uint)frame_data.win_width;
    float2 uv = thread_id.xy / float2(frame_data.win_width - 1.0f, frame_data.win_height - 1.0f);

    buffer[index].color = float4(uv, 0.0f, 1.0f);
}
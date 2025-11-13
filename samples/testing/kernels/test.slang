import common;

RWTexture2D<float4> out_texture;
ConstantBuffer<FrameData> frame_data;
//StructuredBuffer<Pixel> in_buffer;

/* Source: XorDev <https://www.shadertoy.com/view/WclSWn> */ 

// Number of turbulence waves
#define TURB_NUM 10.0
// Turbulence wave amplitude
#define TURB_AMP 0.7
// Turbulence wave speed
#define TURB_SPEED 0.3
// Turbulence frequency
#define TURB_FREQ 2.0
// Turbulence frequency multiplier
#define TURB_EXP 1.4

// Apply turbulence to coordinates
float2 turbulence(float2 p) {
    // Turbulence starting scale
    float freq = TURB_FREQ;

    // Turbulence rotation matrix
    float2x2 rot = float2x2(0.6, -0.8, 0.8, 0.6);

    // Loop through turbulence octaves
    for (float i = 0.0; i < TURB_NUM; i++) {
        // Scroll along the rotated y coordinate
        float phase = freq * mul(rot, p).y + TURB_SPEED * frame_data.time + i;
        // Add a perpendicular sine wave offset
        p += TURB_AMP * rot[0] * sin(phase) / freq;

        // Rotate for the next octave
        rot = mul(rot, float2x2(0.6, -0.8, 0.8, 0.6));
        // Scale down for the next octave
        freq *= TURB_EXP;
    }

    return p;
}

[shader("compute")] /* Compute shader entry point */
[numthreads(16, 8, 1)]
void entry(int2 thread_id: SV_DispatchThreadID) {
    const float2 uv = 2.0 * ((float2)thread_id * 2.0 - float2(frame_data.win_width, frame_data.win_height)) / frame_data.win_height;
    const float2 t = turbulence(uv);
    // Subtle blue and yellow gradient
    float3 col = 0.5 * exp(0.1 * t.x * float3(-1, 0, 2));
    // Vary brightness
    col /= dot(cos(t * 3.), sin(-t.yx * 3. * .618)) + 2.0;
    // Exponential tonemap
    col = 1.0 - exp(-col);
    out_texture[thread_id] = float4(col, 1);

    Texture2D<float4> tex = bindless::texture_sampled<Texture2D<float4>>(1);
    out_texture[thread_id] = tex.Load(int3(thread_id, 0));

    StructuredBuffer<Pixel> buf = bindless::storage_buffer<StructuredBuffer<Pixel>>(3);
    out_texture[thread_id] = buf[int(thread_id.x + thread_id.y * frame_data.win_width)].color;
}

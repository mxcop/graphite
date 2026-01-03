import common;

struct VertexInput {
    float3 position : POSITION;
};

struct VertexOutput {
    float4 position : SV_Position;
};

[shader("vertex")]
VertexOutput vertex_entry(VertexInput input) {
    VertexOutput output;
    output.position = float4(input.position, 1.0f);
    return output;
}

[shader("fragment")]
float4 fragment_entry() : SV_Target0 {
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
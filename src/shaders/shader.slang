static float2 triangle_positions[3] = float2[](
    float2(0.0, -0.5),
    float2(0.5, 0.5),
    float2(-0.5, 0.5)
);

static float3 triangle_colors[3] = float3[](
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0)
);

struct VertexShaderOutput {
    float4 position : SV_Position;
    float3 color;
};

[shader("vertex")]
VertexShaderOutput shaderMapVertex(uint vertexId : SV_VertexID) {
    VertexShaderOutput output;
    output.position = float4(triangle_positions[vertexId], 0, 1);
    output.color = triangle_colors[vertexId];
    return output;
}

[shader("fragment")]
float4 shaderMapFragment(VertexShaderOutput vertexIn) : SV_Target {
    float3 color = vertexIn.color;
    return float4(color, 1);
}
#if !defined(FLAGS_ADDED)
# error FLAGS_ADDED not defined
#endif

cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
  matrix model;
  matrix view;
  matrix projection;
};

struct VertexShaderInput
{
  float3 pos : POSITION;
  float3 color : COLOR0;
};

struct VertexShaderOutput
{
  float4 pos : SV_POSITION;
  float3 color : COLOR0;
};

VertexShaderOutput mainVS(VertexShaderInput input)
{
  VertexShaderOutput output;
  float4 pos = float4(input.pos, 1.0f);

  // Transform the vertex position into projected space.
  pos = mul(pos, model);
  pos = mul(pos, view);
  pos = mul(pos, projection);
  output.pos = pos;

  // Pass through the color without modification.
  output.color = input.color;

  return output;
}

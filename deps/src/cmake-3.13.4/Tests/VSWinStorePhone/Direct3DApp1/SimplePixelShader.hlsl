#if !defined(FLAGS_ADDED)
# error FLAGS_ADDED not defined
#endif

struct PixelShaderInput
{
  float4 pos : SV_POSITION;
  float3 color : COLOR0;
};

float4 mainPS(PixelShaderInput input) : SV_TARGET
{
  return float4(input.color,1.0f);
}

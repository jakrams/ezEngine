[PLATFORMS] 
ALL

[PERMUTATIONS]

EDITOR_PICKING

[VERTEXSHADER]

#include "Common.inc"

VS_OUT main(VS_IN input)
{
  VS_OUT output;
  output.pos = mul(mvp, float4(input.pos, 1.0));
  output.color = input.color;

  return output;
}

[PIXELSHADER]

#include "Common.inc"

float4 main(PS_IN input) : SV_Target
{
#if EDITOR_PICKING != 0
  //return input.color / 4.0 * 4.0;
  return ObjectID;
#else
  //return ObjectID;
  return input.color / 4.0 * 2.0;
#endif
}


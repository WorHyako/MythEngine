/**/
#version 460

#include <Vulkan/SSAO/SSAOBlurCommon.h>

void main()
{
  vec4 Color = vec4( 0.0, 0.0, 0.0, 0.0 );

  for ( int i = 0; i < 11; i++ )
  {
     vec2 Coord = vec2( texCoord.x + gaussFilter[i].y * texScaler + texOffset,
                        texCoord.y + gaussFilter[i].x * texScaler + texOffset );

     Color += texture( texSSAO, Coord ) * gaussFilter[i].w;
  }

  Color.a *= 2.0;
  outColor = Color;
}

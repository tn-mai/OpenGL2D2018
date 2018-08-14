#version 410

layout(location=0) in vec3 vPosition;
layout(location=1) in vec2 vTexCoord;
layout(location=2) in vec4 vColor;
layout(location=3) in vec4 vBorderColor;
layout(location=4) in vec2 vThicknessAndOutline;

layout(location=0) out vec3 outTexCoord;
layout(location=1) out vec4 outColor;
layout(location=2) out vec4 outBorderColor;
layout(location=3) out vec2 outThicknessAndOutline;

void main()
{
  outTexCoord = vec3(vTexCoord.xy, vPosition.z);
  outColor = vColor;
  outBorderColor = vBorderColor;
  outThicknessAndOutline = vThicknessAndOutline;
  gl_Position = vec4(vPosition.xy, 0, 1);
}
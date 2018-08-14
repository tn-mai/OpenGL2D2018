#version 410

layout(location=0) in vec3 inTexCoord;
layout(location=1) in vec4 inColor;
layout(location=2) in vec4 inBorderColor;
layout(location=3) in vec2 inThicknessAndOutline;

out vec4 fragColor;

uniform sampler2D colorSampler[3];
const float smoothing = 1.0 / 16.0;
//const vec2 thicknessAndOutline = vec2(0.75, 0.25);
//const vec4 borderColor = vec4(0.1, 0.1, 0.1, 1.0);

void main()
{
  float distance = texture(colorSampler[int(inTexCoord.z)], inTexCoord.xy).r;
#if 0
  float alpha = smoothstep(0.4 - smoothing, 0.4 + smoothing, distance);
  fragColor = vec4(inColor.rgb, inColor.a * alpha);
#else
  float outline = smoothstep(inThicknessAndOutline.x - smoothing, inThicknessAndOutline.x + smoothing, distance);
  fragColor = mix(inBorderColor, inColor, outline);
  fragColor.a *= smoothstep(inThicknessAndOutline.y - smoothing, inThicknessAndOutline.y + smoothing, distance);
#endif
}
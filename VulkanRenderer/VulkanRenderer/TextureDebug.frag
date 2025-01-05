#version 450
layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 TexCoord;
layout(binding = 0) uniform sampler2D debugTex;

void main(){
	outColor =vec4(texture(debugTex,TexCoord));
}

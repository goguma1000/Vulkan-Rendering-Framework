#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(set = 0, binding = 0) uniform vertexShaderUBO{
	mat4 lightSpaceMat;
	mat4 view;
	mat4 proj;
}ubo;

layout(push_constant) uniform VertexShaderPushConstant{
	mat4 model;
}pushed_Mat;

void main(){
	vec4 pos = vec4(inPosition,1.0f);
	gl_Position = ubo.proj * ubo.view * pushed_Mat.model * pos; 
}

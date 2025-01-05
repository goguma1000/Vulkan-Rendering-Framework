#version 450
layout(location = 0) out vec2 texCoord;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 worldPos;
layout(location = 3) out vec4 lightSpaceFragPos;
layout(location = 4) out mat4 lightProj;

layout(set = 0, binding = 0) uniform VertexShaderUBO{
	mat4 lightSpaceMat;
	mat4 view;
	mat4 proj;
} ubo;

layout(push_constant) uniform VertexShaderPushConstant{
	mat4 model;
}pushed_Mat;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

void main(){
	gl_Position = ubo.proj * ubo.view * pushed_Mat.model * vec4(inPosition,1.0f);
	texCoord = inTexCoord;
	outNormal = normalize((pushed_Mat.model * vec4(inNormal,0.0f)).xyz);
	worldPos = (pushed_Mat.model * vec4(inPosition, 1.0f)).xyz;
	lightSpaceFragPos = ubo.lightSpaceMat * pushed_Mat.model * vec4(inPosition,1.0f);
	lightProj = ubo.lightSpaceMat;
}
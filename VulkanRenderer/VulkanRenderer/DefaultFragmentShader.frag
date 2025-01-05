#version 450
#extension GL_EXT_nonuniform_qualifier : require
layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 worldPos;
layout(location = 3) in vec4 lightSpaceFragPos;
layout(location = 4) in mat4 lightProj;

struct DirectionalLight{
	vec3 dir;
	float intensity;
	float zNear;
};

layout(set=0, binding = 1) uniform FragmentShaderUBO
{
	DirectionalLight dirLight;
	vec3 cameraPos;
}ubo;

layout(set = 0, binding = 2) uniform sampler2D shadowMap;

layout(set = 1, binding = 0) uniform sampler2D textures[]; 

layout(std140, push_constant) uniform TextureIndexPushConstant{
layout(offset = 64)
	int diffTexIdx;
	int specTexIdx;
	int bumpMapIdx;
	int normalMapIdx;
	int emissionMapIdx;
	int opacityMapIdx;
	int roughnessMapIdx;
	int metalnessMapIdx;
	int ambOcclMapIdx;
}texIDX;

const float PI = 3.1415926;
int blockerSampleCount = 32;
int shadowSampleCount = 64;
float rand(vec2 co){
	return fract(sin(dot(co.xy,vec2(12.9898, 78.233))) * 43758.5453);
}
vec2 VogleSample(int idx, int sampleCount, float offset){
    float i = float(idx);
	float goldenAngle = 2.4f;
	float r = sqrt(i + 0.5f) / sqrt(sampleCount);
	float theta = goldenAngle * i + offset;
	return vec2(r*cos(theta), r * sin(theta));
}

float BlockerSearch(vec3 projCoord, vec2 searchR){
	float randomOffset = rand(gl_FragCoord.xy) * PI;
	float D_blocker = 0.0f;
	int count = 0;
	for(int i = 0; i< blockerSampleCount; i++){
		vec2 offset = VogleSample(i, shadowSampleCount,randomOffset) * searchR;
		float D_shadowMap =  texture(shadowMap, projCoord.xy + offset).r;
		if(D_shadowMap < projCoord.z){
			D_blocker += D_shadowMap;
			count++;
		}
	}
	if(count < 1) return projCoord.z;
	return D_blocker / float(count);
}

float PCF(vec3 projCoord, vec2 W_penumbra){
	float result = 0.0f;
	float randomOffset = rand(gl_FragCoord.xy) * PI;
	for(int i = 0; i< shadowSampleCount; i++){
		if(texture(shadowMap, projCoord.xy + W_penumbra* VogleSample(i, shadowSampleCount,randomOffset)).r > projCoord.z){
			result += 1.0f;
		}
	}
	return smoothstep(0.0f, 0.75f,  result / float(shadowSampleCount));
}

float PCSS(vec4 lightSpaceFragPos){
	float shadow = 1.0f;
	float W_light = 0.6f;
	float zNear = ubo.dirLight.zNear;
	vec3 projCoord = lightSpaceFragPos.xyz / lightSpaceFragPos.w;
	projCoord =  vec3(projCoord.xy * 0.5f + vec2(0.5f), projCoord.z); 
	float D_frag = projCoord.z;
	if(D_frag < texture(shadowMap,projCoord.xy).r) return 1.0f;
	vec2 searchR =  vec2(W_light) * (D_frag - zNear) / D_frag;
	float D_blocker = BlockerSearch(projCoord,searchR);
	if(D_frag < D_blocker) return 1.0f;
	vec2 W_penumbra = vec2(W_light) * (D_frag - D_blocker) / D_blocker;

	return PCF(projCoord, W_penumbra);
}

float ShadowCalculation(vec4 lightSpaceFragPos){
	//perform perspective divide
	//when using orthographic projection, it is meaningless
	vec3 projCoords = lightSpaceFragPos.xyz / lightSpaceFragPos.w;
	//convert cliped Coordinate[-1,1] -> NDC [0,1]
	projCoords =  vec3(projCoords.xy * 0.5f + vec2(0.5f), projCoords.z); 
	float closestDepth = texture(shadowMap, projCoords.xy).r;
	float currentDepth = projCoords.z;
	return closestDepth < currentDepth ? 0.0f : 1.0f;
}

DirectionalLight directionalLight;
vec3 lightColor = vec3(1.0f,1.0f,1.0f);
void main(){
	directionalLight = ubo.dirLight;
	
	float intensity = directionalLight.intensity;

	vec3 texColor = vec3(1.0f);
	if(texIDX.diffTexIdx >= 0) texColor = texture(textures[texIDX.diffTexIdx],texCoord).rgb;
	vec3 arm = vec3(1.0f);
	if(texIDX.roughnessMapIdx >= 0) arm = texture(textures[texIDX.roughnessMapIdx],texCoord).rgb;

	vec3 N = normalize(inNormal);
	vec3 dir = normalize(-directionalLight.dir);
	vec3 R =   normalize(2*dot(N,dir)*N - dir);
	vec3 view = normalize(ubo.cameraPos - worldPos);
	float shadow = PCSS(lightSpaceFragPos);
	//shadow = shadow >= 1.0f ? shadow : shadow + 0.2f;
	//shadow *= smoothstep(cos(60.0f * PI/ 180.0f ), cos(60.0f * PI/ 180.0f ) + 0.05f, dot(dir, normalize(dir - worldPos)));
	
	float diff = max(dot(dir,-N),0);
	vec3 diffuse = vec3(1.0f) * diff;
	float spec = pow(max(dot(view,R), 0), 64.0f);
	vec3 specular = vec3(arm[1]) * spec * vec3(1.0f);
	vec3 ambient = arm.r * vec3(0.15f);
	vec3 color = (shadow + ambient) *  (diffuse + specular) * texColor ;
	outColor = vec4(color, 1.0f);
}
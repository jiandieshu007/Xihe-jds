#version 450

layout(location =0) in vec2 uv;

layout(set = 0, binding =0) uniform sampler2D color;
layout(set = 0, binding =1) uniform sampler2D depth;

const int kernelSize = 8;
const vec2[] kernel = vec2[kernelSize](
	vec2(1,0),vec2(0,1),vec2(0,1),vec2(0,-1),vec2(1,1),vec2(1,-1),vec2(-1,1),vec2(-1,-1)
);

layout(location = 0) out vec4 frag;
void main(){
	
	float dst = texture(depth,uv).r;
	ivec2 texSize = textureSize(depth,0);
	float occlusion = 0.f;
	for(int i=0; i<kernelSize; ++i){
		vec2 sam = uv + kernel[i]/texSize;
		float src = texture(depth,sam).r;
		if( src > dst ){
			occlusion += 1;
		}
	}
	frag = texture(color,uv) * occlusion/kernelSize;
}
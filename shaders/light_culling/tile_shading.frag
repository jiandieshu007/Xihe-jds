#version 450

layout(location = 0) in vec2 uv;


layout(set = 0, binding = 0) uniform sampler2D depth;
layout(set = 0, binding =1 ) uniform sampler2D albedo;
layout(set=0, binding =2) uniform sampler2D normal;

struct Light{
	vec4 pos;
	vec4 color;
	vec4 direction;
	vec2 info;
};


layout(set = 0, binding =3) readonly buffer lights{
	Light light[4096];
};

struct Tile{
	uint size;
	uint index[32];
};

// split screen into 32*32 tiles then shading 
layout(std430,set = 0, binding =4) readonly buffer tiles{
	Tile tile[32*32];
};

layout(push_constant)uniform inv{
	mat4 inverseViewPro;
	uint light_num;
	uint tile_size;
};

uint caculateIndex(vec2 p){
	uint a = int(p.x * tile_size);
	uint b = int(p.y * tile_size);
	return a*tile_size +b;
}

layout(location = 0) out vec4 fragColor;
 
void main(){
	uint index = caculateIndex(uv);
	if( index >= tile_size * tile_size) return ;

	vec3 postion = (inverseViewPro * vec4(uv,texture(depth,uv).r, 1.f)).rgb;
	vec4 alb = texture(albedo,uv);
	vec3 nor = normalize(texture(normal,uv).xyz);

	vec4 color = vec4(0.f);
	Tile cacu = tile[index];
	for(uint i=0; i<cacu.size && i< tile_size; ++i){
		Light cal = light[cacu.index[i]];
		vec3 lightPos = cal.pos.rgb;
		float type = cal.pos.a;
		vec3 lightDir = cal.direction.rgb;
		float radius = cal.direction.a;
		vec3 incolor = cal.color.rgb;

		if( type == 0){
			float dis = length(lightPos-postion); 
			if( dis < radius){
				color += vec4(incolor * dot(nor,normalize(lightPos-postion)),1.f) * alb;
			}
		}else if( type == 1){
			color += vec4(incolor * dot(nor,normalize(lightDir)), 1.f) * alb;
		}
	}

	fragColor = color;
}

#version 450

layout(location = 0) in vec3 position;

layout(location = 0)out vec3 uvs;

layout( set = 0, binding = 0) uniform vp{
	mat4 VP;
};

void main(){
	uvs = position;
	gl_Position = VP*vec4(position,1.f);

}
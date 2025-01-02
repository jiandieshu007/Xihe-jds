#version 450

layout(set =0, binding = 1 )uniform samplerCube Skybox;

layout(location = 0)in vec3 uvs;

layout(location = 0) out vec4 fragColor;

void main(){
	fragColor = texture(Skybox,uvs);
}
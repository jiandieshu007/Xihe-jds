#version 450

layout(set =0, binding = 1 )uniform samplerCube Skybox;

in vec3 uvs;

out vec4 fragColor;

void main(){
	fragColor = texture(Skybox,uvs);
}
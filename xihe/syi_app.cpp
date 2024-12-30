#include "syi_app.h"

#include "backend/shader_compiler/glsl_compiler.h"

xihe::syiAPP::syiAPP()
{
	// Adding device extensions
	add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	add_device_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME);
	add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);

	backend::GlslCompiler::set_target_environment(glslang::EShTargetSpv, glslang::EShTargetSpv_1_4);
}

bool xihe::syiAPP::prepare(Window *window)
{
	if (!XiheApp::prepare(window))
	{
		return false;
	}
	load_scene("scenes/cube.gltf");
	// load_scene("scenes/cube.gltf");
	assert(scene_ && "Scene not loaded");

	auto &camera_node = xihe::sg::add_free_camera(*scene_, "main_camera", render_context_->get_surface_extent());
	auto  camera      = &camera_node.get_component<xihe::sg::Camera>();
	camera_           = camera;


}

void xihe::syiAPP::update(float delta_time)
{
}

void xihe::syiAPP::request_gpu_features(backend::PhysicalDevice &gpu)
{
}

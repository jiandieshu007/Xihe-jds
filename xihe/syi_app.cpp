#include "syi_app.h"

#include "backend/shader_compiler/glsl_compiler.h"
#include "scene_graph/gltf_loader.h"
#include "rendering/passes/skybox_pass.h"

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
	xihe::GltfLoader loader(*device_);
	std::string      path = "scenes/cube.gltf";
	skyBoxModel = loader.read_scene_from_file(path);

	if (!skyBoxModel)
	{
		LOGE("Cannot load scene: {}", path.c_str());
		throw std::runtime_error("Cannot load scene: " + path);
	}
	// load_scene("scenes/cube.gltf");
	assert(skyBoxModel && "Scene not loaded");

	auto &camera_node = xihe::sg::add_free_camera(*skyBoxModel, "main_camera", render_context_->get_surface_extent());
	auto  camera      = &camera_node.get_component<xihe::sg::Camera>();
	camera_           = camera;

	{
		auto skyBoxPass = std::make_unique<rendering::SkyboxPass>(*device_,skyBoxModel->get_components<sg::Mesh>(), *camera);
		graph_builder_->add_pass("SkyBoxPass", std::move(skyBoxPass))

		    .attachments({{rendering::AttachmentType::kColor, "output"}})
		    .shader({"skybox/skybox.vert", "skybox/skybox.frag"})
		    .present()
		    .finalize();
			
	}
	graph_builder_->build();

	return true;
}

void xihe::syiAPP::update(float delta_time)
{
}

void xihe::syiAPP::request_gpu_features(backend::PhysicalDevice &gpu)
{
	XiheApp::request_gpu_features(gpu);

	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceMeshShaderFeaturesEXT, meshShader);
	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceMeshShaderFeaturesEXT, meshShaderQueries);
	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceMeshShaderFeaturesEXT, taskShader);

	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceDynamicRenderingFeatures, dynamicRendering);
}

std::unique_ptr<xihe::Application> create_application()
{
	return std::make_unique<xihe::syiAPP>();
}
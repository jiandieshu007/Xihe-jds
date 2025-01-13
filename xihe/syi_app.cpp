#include "syi_app.h"

#include "backend/shader_compiler/glsl_compiler.h"
#include "rendering/passes/geometry_pass.h"
#include "rendering/passes/light_culling.h"
#include "rendering/passes/skybox_pass.h"
#include "scene_graph/gltf_loader.h"

xihe::syiAPP::syiAPP()
{
	// Adding device extensions
	add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	add_device_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME);
	add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
	// add_device_extension(VK_EXT_SHADER_TILE_IMAGE_EXTENSION_NAME);
	backend::GlslCompiler::set_target_environment(glslang::EShTargetSpv, glslang::EShTargetSpv_1_4);
}

struct alignas(16) Tile
{
	int index[32]{};
	int size = 0;
};


bool xihe::syiAPP::prepare(Window *window)
{
	if (!XiheApp::prepare(window))
	{
		return false;
	}
	// load_scene("scenes/cube.gltf");
	load_scene("scenes/sponza/Sponza01.gltf");

	if (!scene_)
	{
		LOGE("Cannot load scene: {}", " ");
		throw std::runtime_error("Cannot load scene: ");
	}
	// load_scene("scenes/cube.gltf");
	assert(scene_ && "Scene not loaded");
	update_bindless_descriptor_sets();

	auto &camera_node = xihe::sg::add_free_camera(*scene_, "main_camera", render_context_->get_surface_extent());
	auto  camera      = &camera_node.get_component<xihe::sg::Camera>();
	camera_           = camera;

	{
		auto geometry_pass = std::make_unique<rendering::GeometryPass>(scene_->get_components<sg::Mesh>(), *camera);

		graph_builder_->add_pass("Geometry", std::move(geometry_pass))

		    .attachments({{rendering::AttachmentType::kDepth, "depth"},
		                  {rendering::AttachmentType::kColor, "albedo"},
		                  {rendering::AttachmentType::kColor, "normal", vk::Format::eA2B10G10R10UnormPack32}})

		    .shader({"deferred/geometry.vert", "deferred/geometry.frag"})

		    .finalize();
	}

	{
		auto     culling_pass = std::make_unique<rendering::culling_lighting>(*camera);
		auto     lights       = culling_pass->lights;
		uint32_t size         = culling_pass->tile_size * culling_pass->tile_size * sizeof(Tile);
		graph_builder_->add_pass("tile_culling", std::move(std::move(std::move(culling_pass))))
		    .bindables({{rendering::BindableType::kSampled, "depth"},
		                {.type = rendering::BindableType::kStorageBufferReadWrite, .name = "tiles", .buffer_size = size}})
		    .shader({"light_culling/culling.comp"})
		    .finalize();

		auto shading_pass = std::make_unique<rendering::tile_shading>(std::move(lights), *camera);
		graph_builder_->add_pass("tile_shading", std::move(shading_pass))
		    .bindables({{rendering::BindableType::kSampled, "depth"},
		                {rendering::BindableType::kSampled, "albedo"},
		                {rendering::BindableType::kSampled, "normal"},
		                {rendering::BindableType::kStorageBufferRead, "tiles"}})
		    .shader({"light_culling/tile_shading.vert", "light_culling/tile_shading.frag"})
		    .present()
		    .finalize();
	}
	graph_builder_->build();

	return true;
}

void xihe::syiAPP::update(float delta_time)
{
	XiheApp::update(delta_time);
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
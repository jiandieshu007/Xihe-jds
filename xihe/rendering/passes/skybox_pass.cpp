#include "skybox_pass.h"

#include "platform/filesystem.h"


xihe::rendering::SkyboxPass::SkyboxPass(backend::Device &device,std::vector<sg::Mesh *> meshes, sg::Camera &camera) :
    camera_(camera), meshes_(meshes)
{
	// load skybox as textureCube
	std::string file_name = "textures/hdr/pisa_cube.ktx";
	skyboxImage          = sg::Image::load("skyboxImage", file_name, sg::Image::kUnknown);
	skyboxImage->create_vk_image(device, vk::ImageViewType::eCube, vk::ImageCreateFlagBits::eCubeCompatible);
	vk::SamplerCreateInfo sampler_create;
	skyboxSampler = std::make_unique<sg::Sampler>("skyboxSampler", std::move(*(new backend::Sampler(device, sampler_create))));
}

void xihe::rendering::SkyboxPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	DepthStencilState depth_stencil_state{};
	depth_stencil_state.depth_test_enable  = false;
	depth_stencil_state.depth_write_enable = false;

	command_buffer.set_depth_stencil_state(depth_stencil_state);


	for(auto& mesh : meshes_)
	{
		for( auto& submess: mesh->get_submeshes())
		{
			update_uniform(command_buffer, active_frame,thread_index_);
			draw_submesh(command_buffer, *submess);
		}

	}
}

void xihe::rendering::SkyboxPass::update_uniform(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, size_t thread_index)
{
	glm::mat4 VPuniform{};

	VPuniform= camera_.get_pre_rotation() * vulkan_style_projection(camera_.get_projection()) * camera_.get_view();

	auto allocation = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(SceneUniform), thread_index);

	allocation.update(VPuniform);

	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 0, 0);
}

void xihe::rendering::SkyboxPass::draw_submesh(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh, vk::FrontFace front_face)
{

	auto &device = command_buffer.get_device();

	backend::ScopedDebugLabel{command_buffer, sub_mesh.get_name().c_str()};


	auto &resource_cache = command_buffer.get_device().get_resource_cache();

	auto &vert_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader(), sub_mesh.get_shader_variant());
	auto &frag_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader(), sub_mesh.get_shader_variant());

	std::vector<backend::ShaderModule *> shader_modules{&vert_shader_module, &frag_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules, &resource_cache.request_bindless_descriptor_set());

	command_buffer.bind_pipeline_layout(pipeline_layout);
	command_buffer.bind_image(skyboxImage->get_vk_image_view(), skyboxSampler->vk_sampler_, 0, 1,0);
	auto vertex_input_resources = pipeline_layout.get_resources(backend::ShaderResourceType::kInput, vk::ShaderStageFlagBits::eVertex);

	VertexInputState vertex_input_state{};

	for (auto &input_resource : vertex_input_resources)
	{
		VertexAttribute attribute;

		if (!sub_mesh.get_attribute(input_resource.name, attribute))
		{
			continue;
		}

		vk::VertexInputAttributeDescription vertex_attribute{
		    input_resource.location,
		    input_resource.location,
		    attribute.format,
		    attribute.offset};

		vertex_input_state.attributes.push_back(vertex_attribute);

		vk::VertexInputBindingDescription vertex_binding{
		    input_resource.location,
		    attribute.stride};

		vertex_input_state.bindings.push_back(vertex_binding);
	}

	command_buffer.set_vertex_input_state(vertex_input_state);

	for (auto &input_resource : vertex_input_resources)
	{
		const auto &buffer_iter = sub_mesh.vertex_buffers.find(input_resource.name);

		if (buffer_iter != sub_mesh.vertex_buffers.end())
		{
			std::vector<std::reference_wrapper<const backend::Buffer>> buffers;
			buffers.emplace_back(std::ref(buffer_iter->second));

			command_buffer.bind_vertex_buffers(input_resource.location, buffers, {0});
		}
	}

	if (sub_mesh.index_count != 0)
	{
		command_buffer.bind_index_buffer(*sub_mesh.index_buffer, sub_mesh.index_offset, sub_mesh.index_type);

		command_buffer.draw_indexed(sub_mesh.index_count, 1, 0, 0, 0);
	}
	else
	{
		command_buffer.draw(sub_mesh.vertex_count, 1, 0, 0);
	}
}
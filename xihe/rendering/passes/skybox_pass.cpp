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
	auto& device = command_buffer.get_device();
	RasterizationState rasterization_state;
	rasterization_state.front_face = front_face;
	
	command_buffer.set_rasterization_state(rasterization_state);

	auto &resource_cache   = command_buffer.get_device().get_resource_cache();
	auto &vertShaderModule = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader(), sub_mesh.get_shader_variant());
	auto& fragShaderModule = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment,get_fragment_shader(),sub_mesh.get_shader_variant());

	std::vector<backend::ShaderModule *> shaderMudule{&vertShaderModule, &fragShaderModule};
	auto                                &pipelineLayout = resource_cache.request_pipeline_layout(shaderMudule);
	command_buffer.bind_pipeline_layout(pipelineLayout);

//	const auto &DescriptorSetLayout = pipelineLayout.get_descriptor_set_layout(0);
	command_buffer.bind_image(skyboxImage->get_vk_image_view(), skyboxSampler->vk_sampler_,
	                          0, 1, 0);
	auto vertexInputResources = pipelineLayout.get_resources(backend::ShaderResourceType::kInput, vk::ShaderStageFlagBits::eVertex);

	VertexInputState vertexInputState{};
	for (auto &inputResource : vertexInputResources)
	{
		VertexAttribute attribute;
		if (!sub_mesh.get_attribute(inputResource.name, attribute))
		{
			continue;
		}
		vk::VertexInputAttributeDescription vertexAttribute
		{
			inputResource.location, inputResource.location, attribute.format, attribute.offset
		};
		vertexInputState.attributes.push_back(vertexAttribute);
		vk::VertexInputBindingDescription vertexBinding{
		    inputResource.location, attribute.stride
		};
		vertexInputState.bindings.push_back(vertexBinding);
	}
	command_buffer.set_vertex_input_state(vertexInputState);
	for (auto& inputResource: vertexInputResources)
	{
		const auto &bufferIter = sub_mesh.vertex_buffers.find(inputResource.name);
		if (bufferIter != sub_mesh.vertex_buffers.end())
		{
			std::vector<std::reference_wrapper<const backend::Buffer>> buffers;
			buffers.emplace_back(std::ref(bufferIter->second));

			command_buffer.bind_vertex_buffers(inputResource.location, buffers, {0});
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
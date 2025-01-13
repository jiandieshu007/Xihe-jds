#pragma once

#include "render_pass.h"

namespace xihe
{
using namespace rendering;
vk::SamplerCreateInfo get_linear_sampler()
{
	auto sampler_info         = vk::SamplerCreateInfo{};
	sampler_info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.minFilter    = vk::Filter::eLinear;
	sampler_info.magFilter    = vk::Filter::eLinear;
	sampler_info.maxLod       = VK_LOD_CLAMP_NONE;

	return sampler_info;
}

class ssao_pass : public xihe::rendering::RenderPass
{
  public:
	ssao_pass() = default;
	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override
	{
		auto &resource_cache = command_buffer.get_device().get_resource_cache();

		auto &vert_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader());
		auto &frag_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader());

		std::vector<backend::ShaderModule *> shader_modules = {&vert_shader_module, &frag_shader_module};

		auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
		command_buffer.bind_pipeline_layout(pipeline_layout);

		auto &color = input_bindables[0].image_view();
		auto &depth = input_bindables[1].image_view();

		command_buffer.bind_image(color, resource_cache.request_sampler(get_linear_sampler()), 0, 0, 0);
		command_buffer.bind_image(depth, resource_cache.request_sampler(get_linear_sampler()), 0, 1, 0);

		RasterizationState rasterization_state;
		rasterization_state.cull_mode = vk::CullModeFlagBits::eNone;
		command_buffer.set_rasterization_state(rasterization_state);

		command_buffer.draw(3, 1, 0, 0);
	};
};


}        // namespace xihe
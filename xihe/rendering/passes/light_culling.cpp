#include "light_culling.h"
#include <random>

static glm::vec3 generate_position()
{
	static std::random_device               rd;
	static std::mt19937                     gen(rd());
	static std::uniform_real_distribution<> dis1(-1600, 1600);
	static std::uniform_real_distribution<> dis2(0, 730);

	float x = dis1(gen);
	float y = dis2(gen) / 2 + 8;
	float z = dis1(gen);

	return glm::vec3(x, y, z);
}
static glm::vec4 generate_color()
{
	static std::random_device               rd;
	static std::mt19937                     gen(rd());
	static std::uniform_real_distribution<> dis1(0, 1);
	static std::uniform_real_distribution<> dis2(0, 730);

	float x = dis1(gen);
	float y = dis1(gen);
	float z = dis1(gen);
	float a = dis2(gen);
	return glm::vec4(x, y, z, a);
}

void xihe::rendering::culling_lighting::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	auto &resource_cache     = command_buffer.get_device().get_resource_cache();
	auto &comp_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eCompute, get_compute_shader());

	std::vector<backend::ShaderModule *> shader_modules = {&comp_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	light_buffer = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eStorageBuffer, sizeof(Light) * light_count, thread_index_);
	light_buffer.update(lights);

	ubo_buffer = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(ubo), thread_index_);
	ubo update;
	update.light_num             = light_count;
	update.tile_size             = tile_size;
	update.inverse_viewprojecion = glm::inverse(vulkan_style_projection(camera.get_projection()) * camera.get_view());
	ubo_buffer.update(update);

	command_buffer.bind_buffer(light_buffer.get_buffer(), light_buffer.get_offset(), light_buffer.get_size(), 0, 1, 0);
	command_buffer.bind_buffer(input_bindables[1].buffer(), 0, input_bindables[1].buffer().get_size(), 0, 0, 0);
	command_buffer.bind_buffer(ubo_buffer.get_buffer(), ubo_buffer.get_offset(), ubo_buffer.get_size(), 0, 2, 0);
	command_buffer.bind_image(input_bindables[0].image_view(), resource_cache.request_sampler({}), 0, 3, 0);
	command_buffer.dispatch(1, 1, 1);
}

void xihe::rendering::culling_lighting::prepareLight()
{
	lights.resize(light_count);
	auto light_pos = glm::vec3(-150.0f, 188.0f, -225.0f);
	for (int i = 0; i < light_count; ++i)
	{
		auto &light = lights[i];
		if (i == 0)
		{
			lights[i].position.a = 1;
			lights[i].direction  = generate_color();
			lights[i].color      = generate_color();
		}
		else
		{
			lights[i].position  = glm::vec4(generate_position() + light_pos, 0);
			lights[i].direction = generate_color();
			lights[i].color     = generate_color();
		}
	}
}

void xihe::rendering::tile_shading::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	auto &resource_cache = command_buffer.get_device().get_resource_cache();

	std::vector<backend::ShaderModule *> shader_modules;

	auto &vert_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader(), {});
	auto &frag_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader(), {});
	shader_modules           = {&vert_shader_module, &frag_shader_module};
	RasterizationState rasterization_state;
	rasterization_state.cull_mode = vk::CullModeFlagBits::eNone;
	command_buffer.set_rasterization_state(rasterization_state);

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules, nullptr);
	command_buffer.bind_pipeline_layout(pipeline_layout);
	light_buffer = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eStorageBuffer, sizeof(Light) * light_count, thread_index_);
	light_buffer.update(lights);

	command_buffer.bind_buffer(light_buffer.get_buffer(), light_buffer.get_offset(), light_buffer.get_size(), 0, 3, 0);
	command_buffer.bind_buffer(input_bindables[3].buffer(), 0, input_bindables[3].buffer().get_size(), 0, 4, 0);
	struct pushConstant
	{
		glm::mat4 re;
		glm::uint light_count;
		glm::uint tile_size;
	};
	pushConstant pc{glm::inverse(vulkan_style_projection(camera.get_projection()) * camera.get_view()), light_count, tile_size};

	command_buffer.push_constants(to_bytes(pc));
	command_buffer.bind_image(input_bindables[0].image_view(), resource_cache.request_sampler({}), 0, 0, 0);
	command_buffer.bind_image(input_bindables[1].image_view(), resource_cache.request_sampler({}), 0, 1, 0);
	command_buffer.bind_image(input_bindables[2].image_view(), resource_cache.request_sampler({}), 0, 2, 0);
	command_buffer.draw(3, 1, 0, 0);
}
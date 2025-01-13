#pragma once
#include "render_pass.h"

#include "scene_graph/components/camera.h"

namespace xihe::rendering
{
	struct alignas(16) Light
	{
		glm::vec4 position{};         // position.w represents type of light
		glm::vec4 color{};            // color.w represents light intensity
		glm::vec4 direction{};        // direction.w represents range
		glm::vec2 info{};             // (only used for spot lights) info.x represents light inner cone angle, info.y represents light outer cone angle
	};
	struct alignas(16) Tile
	{
		uint32_t index[32]{};
		uint32_t size = 0;
	};
	struct alignas(16) ubo
	{
		glm::mat4 inverse_viewprojecion;
		uint32_t  light_num;
		uint32_t  tile_size;
	};

class culling_lighting : public RenderPass
{
  public:
  public:
	culling_lighting(sg::Camera &cam) :
	    camera(cam)
	{
		prepareLight();
	};

	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;

	void                      prepareLight();
	std::vector<Light>        lights;
	backend::BufferAllocation light_buffer;
	backend::BufferAllocation ubo_buffer;
	const unsigned int        light_count = 4096;
	const unsigned int        tile_size   = 32;
	sg::Camera               &camera;
};

class tile_shading : public rendering::RenderPass
{
  public:
	tile_shading(std::vector<Light> &&light, sg::Camera &cam) :
	    lights(std::move(light)), camera(cam)
	{}

	void                      execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;
	std::vector<Light>        lights;
	backend::BufferAllocation light_buffer;
	const unsigned int        light_count = 4096;
	const unsigned int        tile_size   = 32;
	sg::Camera               &camera;
};
}        // namespace xihe::rendering
#pragma once
#include "render_pass.h"
#include "backend/command_buffer.h"
#include "backend/resources_management/resource_cache.h"
#include "backend/shader_module.h"
#include "rendering/render_frame.h"

#include <optional>

namespace xihe
{

namespace sg
{
class Scene;
class Node;
class Mesh;
class SubMesh;
class Camera;
}        // namespace sg

namespace rendering
{
struct PBRMaterialUniform
{
	glm::uvec4 texture_indices;

	glm::vec4 base_color_factor;

	float metallic_factor;

	float roughness_factor;
};

struct alignas(16) SceneUniform
{
	glm::mat4 model;

	glm::mat4 camera_view_proj;

	glm::vec3 camera_position;
};

class GeometryPass : public RenderPass
{
  public:
	/**
	 * \brief
	 */
	GeometryPass(std::vector<sg::Mesh *> meshes, sg::Camera &camera);

	virtual ~GeometryPass() = default;

	/**
	 * \brief
	 * \param command_buffer
	 * \param active_frame 用于获取当前帧的资源，或者分配资源
	 */
	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame) override;

  private:
	virtual void update_uniform(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, sg::Node &node, size_t thread_index);

	void draw_submesh(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh, vk::FrontFace front_face = vk::FrontFace::eCounterClockwise);

	virtual void prepare_pipeline_state(backend::CommandBuffer &command_buffer, vk::FrontFace front_face, bool double_sided_material);

	virtual void prepare_push_constants(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh);

	virtual void draw_submesh_command(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh);

	/**
	 * @brief Sorts objects based on distance from camera and classifies them
	 *        into opaque and transparent in the arrays provided
	 */
	void get_sorted_nodes(std::multimap<float, std::pair<sg::Node *, sg::SubMesh *>> &opaque_nodes,
	                      std::multimap<float, std::pair<sg::Node *, sg::SubMesh *>> &transparent_nodes) const;

	std::vector<sg::Mesh *> meshes_;
	sg::Camera             &camera_;

	uint32_t color_attachments_count_{2};
};
}        // namespace rendering
}        // namespace xihe
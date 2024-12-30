#pragma once

#include "render_pass.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/image.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/sampler.h"
namespace xihe::rendering
{
class SkyboxPass : public RenderPass
{
  public:
	/**
	 * @param device 
	 * @param meshes 
	 * @param camera 
	 */
	SkyboxPass(backend::Device &device, std::vector<sg::Mesh *> meshes, sg::Camera &camera);
	~SkyboxPass() = default;

	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;
	virtual void update_uniform(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, size_t thread_index);

	void                        draw_submesh(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh, vk::FrontFace front_face = vk::FrontFace::eCounterClockwise);

	std::vector<sg::Mesh *>    meshes_;
	sg::Camera                 &camera_;
	std::unique_ptr<sg::Image> skyboxImage;
	std::unique_ptr<sg::Sampler>  skyboxSampler;
};
}
#include "skybox_pass.h"

#include "platform/filesystem.h"

xihe::rendering::SkyboxPass::SkyboxPass(std::vector<sg::Mesh *> meshes, sg::Camera &camera) : camera_(camera), meshes_(meshes)
{

}

void xihe::rendering::SkyboxPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	DepthStencilState depth_stencil_state{};
	depth_stencil_state.depth_test_enable  = true;
	depth_stencil_state.depth_write_enable = true;

	command_buffer.set_depth_stencil_state(depth_stencil_state);


}

void xihe::rendering::SkyboxPass::update_uniform(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, sg::Node &node, size_t thread_index)
{
	glm::mat4 VPuniform{};

	VPuniform= camera_.get_pre_rotation() * vulkan_style_projection(camera_.get_projection()) * camera_.get_view();

	auto allocation = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(SceneUniform), thread_index);

	allocation.update(VPuniform);

	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 0, 0);
}
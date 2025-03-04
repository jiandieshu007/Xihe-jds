#include "meshlet_pass.h"

#include "scene_graph/node.h"
#include "scene_graph/scene.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/image.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/texture.h"

#include <ranges>

namespace xihe::rendering
{

namespace
{
glm::vec4 normalize_plane(const glm::vec4 &plane)
{
	float length = glm::length(glm::vec3(plane.x, plane.y, plane.z));
	return plane / length;
}
}        // namespace

MeshletPass::MeshletPass(std::vector<sg::Mesh *> meshes, sg::Camera &camera): meshes_(std::move(meshes)), camera_(camera)
{}

void MeshletPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	std::multimap<float, std::pair<sg::Node *, sg::MshaderMesh *>> opaque_nodes;
	std::multimap<float, std::pair<sg::Node *, sg::MshaderMesh *>> transparent_nodes;

	get_sorted_nodes(opaque_nodes, transparent_nodes);

	command_buffer.set_has_mesh_shader(true);

	DepthStencilState depth_stencil_state{};
	depth_stencil_state.depth_test_enable  = true;
	depth_stencil_state.depth_write_enable = true;

	command_buffer.set_depth_stencil_state(depth_stencil_state);

	{
		backend::ScopedDebugLabel label{command_buffer, "Opaque objects"};

		for (const auto &[node, mshader_mesh] : opaque_nodes | std::views::values)
		{
			update_uniform(command_buffer,active_frame, *node, 0);
			draw_mshader_mesh(command_buffer, *mshader_mesh);
		}
	}

	command_buffer.set_has_mesh_shader(false);
}

void MeshletPass::show_meshlet_view(bool show, sg::Scene &scene)
{
	if (show == show_debug_view_)
	{
		return;
	}
	show_debug_view_ = show;
	for (auto mshader_mesh : scene.get_components<sg::MshaderMesh>())
	{
		auto &variant = mshader_mesh->get_mut_shader_variant();
		if (show)
		{
			variant.add_define("SHOW_MESHLET_VIEW");
		}
		else
		{
			variant.remove_define("SHOW_MESHLET_VIEW");
		}
	}
}

void MeshletPass::freeze_frustum(bool freeze, sg::Camera *camera)
{
	assert(camera);
	if (freeze == freeze_frustum_)
	{
		return;
	}
	freeze_frustum_ = freeze;
	if (freeze)
	{
		frozen_view_              = camera->get_view();
		glm::mat4 m               = glm::transpose(camera->get_pre_rotation() * camera->get_projection());
		frozen_frustum_planes_[0] = normalize_plane(m[3] + m[0]);
		frozen_frustum_planes_[1] = normalize_plane(m[3] - m[0]);
		frozen_frustum_planes_[2] = normalize_plane(m[3] + m[1]);
		frozen_frustum_planes_[3] = normalize_plane(m[3] - m[1]);
		frozen_frustum_planes_[4] = normalize_plane(m[3] + m[2]);
		frozen_frustum_planes_[5] = normalize_plane(m[3] - m[2]);
	}
}

void MeshletPass::update_uniform(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, sg::Node &node, size_t thread_index)
{
	MeshletSceneUniform global_uniform{};
	global_uniform.camera_view_proj = camera_.get_pre_rotation() * vulkan_style_projection(camera_.get_projection()) * camera_.get_view();

	global_uniform.camera_position = glm::vec3((glm::inverse(camera_.get_view())[3]));

	global_uniform.model = node.get_transform().get_world_matrix();

	if (freeze_frustum_)
	{
		global_uniform.view              = frozen_view_;
		global_uniform.frustum_planes[0] = frozen_frustum_planes_[0];
		global_uniform.frustum_planes[1] = frozen_frustum_planes_[1];
		global_uniform.frustum_planes[2] = frozen_frustum_planes_[2];
		global_uniform.frustum_planes[3] = frozen_frustum_planes_[3];
		global_uniform.frustum_planes[4] = frozen_frustum_planes_[4];
		global_uniform.frustum_planes[5] = frozen_frustum_planes_[5];
	}
	else
	{
		global_uniform.view              = camera_.get_view();
		glm::mat4 m                      = glm::transpose(camera_.get_pre_rotation() * camera_.get_projection());
		global_uniform.frustum_planes[0] = normalize_plane(m[3] + m[0]);
		global_uniform.frustum_planes[1] = normalize_plane(m[3] - m[0]);
		global_uniform.frustum_planes[2] = normalize_plane(m[3] + m[1]);
		global_uniform.frustum_planes[3] = normalize_plane(m[3] - m[1]);
		global_uniform.frustum_planes[4] = normalize_plane(m[3] + m[2]);
		global_uniform.frustum_planes[5] = normalize_plane(m[3] - m[2]);
	}

	auto  allocation   = active_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(MeshletSceneUniform), thread_index);

	allocation.update(global_uniform);

	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 2, 0);
}

void MeshletPass::draw_mshader_mesh(backend::CommandBuffer &command_buffer, sg::MshaderMesh &mshader_mesh)
{
	auto &resource_cache     = command_buffer.get_device().get_resource_cache();

	auto &task_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eTaskEXT, get_task_shader(), mshader_mesh.get_shader_variant());
	auto &mesh_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eMeshEXT, get_mesh_shader(), mshader_mesh.get_shader_variant());
	auto &frag_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader(), mshader_mesh.get_shader_variant());

	std::vector<backend::ShaderModule *> shader_modules{&task_shader_module, &mesh_shader_module, &frag_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules, &resource_cache.request_bindless_descriptor_set());
	command_buffer.bind_pipeline_layout(pipeline_layout);

	if (pipeline_layout.get_push_constant_range_stage(sizeof(PBRMaterialUniform)))
	{
		const auto pbr_material = dynamic_cast<const sg::PbrMaterial *>(mshader_mesh.get_material());

		PBRMaterialUniform pbr_material_uniform;
		pbr_material_uniform.texture_indices   = pbr_material->texture_indices;
		pbr_material_uniform.base_color_factor = pbr_material->base_color_factor;
		pbr_material_uniform.metallic_factor   = pbr_material->metallic_factor;
		pbr_material_uniform.roughness_factor  = pbr_material->roughness_factor;

		if (const auto data = to_bytes(pbr_material_uniform); !data.empty())
		{
			command_buffer.push_constants(data);
		}
	}

	const backend::DescriptorSetLayout &descriptor_set_layout = pipeline_layout.get_descriptor_set_layout(0);

	for (auto &[name, texture] : mshader_mesh.get_material()->textures)
	{
		if (const auto layout_binding = descriptor_set_layout.get_layout_binding(name))
		{
			command_buffer.bind_image(texture->get_image()->get_vk_image_view(),
			                          texture->get_sampler()->vk_sampler_,
			                          0, layout_binding->binding, 0);
		}
	}

	command_buffer.bind_buffer(mshader_mesh.get_meshlet_buffer(), 0, mshader_mesh.get_meshlet_buffer().get_size(), 0, 3, 0);
	command_buffer.bind_buffer(mshader_mesh.get_vertex_buffer(), 0, mshader_mesh.get_vertex_buffer().get_size(), 0, 4, 0);
	command_buffer.bind_buffer(mshader_mesh.get_meshlet_vertices_buffer(), 0, mshader_mesh.get_meshlet_vertices_buffer().get_size(), 0, 5, 0);
	command_buffer.bind_buffer(mshader_mesh.get_packed_meshlet_indices_buffer(), 0, mshader_mesh.get_packed_meshlet_indices_buffer().get_size(), 0, 6, 0);
	command_buffer.bind_buffer(mshader_mesh.get_mesh_draw_counts_buffer(), 0, mshader_mesh.get_mesh_draw_counts_buffer().get_size(), 0, 7, 0);
	command_buffer.bind_buffer(mshader_mesh.get_meshlet_buffer(), 0, mshader_mesh.get_meshlet_buffer().get_size(), 0, 8, 0);

	uint32_t num_workgroups_x = (mshader_mesh.get_meshlet_count() + 31) / 32;        // meshlets count

	uint32_t num_workgroups_y = 1;
	uint32_t num_workgroups_z = 1;
	command_buffer.draw_mesh_tasks(num_workgroups_x, num_workgroups_y, num_workgroups_z);
}

void MeshletPass::get_sorted_nodes(std::multimap<float, std::pair<sg::Node *, sg::MshaderMesh *>> &opaque_nodes, std::multimap<float, std::pair<sg::Node *, sg::MshaderMesh *>> &transparent_nodes) const
{
	auto camera_transform = camera_.get_node()->get_transform().get_world_matrix();

	for (auto &mesh : meshes_)
	{
		for (auto &node : mesh->get_nodes())
		{
			auto node_transform = node->get_transform().get_world_matrix();

			const sg::AABB &mesh_bounds = mesh->get_bounds();

			sg::AABB world_bounds{mesh_bounds.get_min(), mesh_bounds.get_max()};
			world_bounds.transform(node_transform);

			float distance = glm::length(glm::vec3(camera_transform[3]) - world_bounds.get_center());

			for (auto &mshader_mesh_mesh : mesh->get_mshader_meshes())
			{
				// todo  handle transparent objects
				opaque_nodes.emplace(distance, std::make_pair(node, mshader_mesh_mesh));
			}
		}
	}
}
}

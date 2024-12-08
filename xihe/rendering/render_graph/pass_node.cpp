#include "pass_node.h"

#include "render_graph.h"

namespace xihe::rendering
{
PassNode::PassNode(RenderGraph &render_graph, std::string name, PassInfo &&pass_info, std::unique_ptr<RenderPass> &&render_pass) :
    render_graph_{render_graph}, name_{std::move(name)}, type_{render_pass->get_type()}, pass_info_{std::move(pass_info)}, render_pass_{std::move(render_pass)}
{
}

void PassNode::execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target, RenderFrame &render_frame)
{
	backend::ScopedDebugLabel subpass_debug_label{command_buffer, name_.c_str()};

	std::vector<ShaderBindable> shader_bindable(bindables_.size());

	for (const auto &[index, input_info] : bindables_)
	{

		shader_bindable[index] = render_graph_.get_resource_bindable(input_info.handle);

		if (!input_info.barrier.has_value())
		{
			continue;
		}

		if (shader_bindable[index].is_buffer())
		{
			// command_buffer.buffer_memory_barrier(std::get<backend::Buffer *>(input_info.resource), input_info.barrier);
		}
		else
		{
			command_buffer.image_memory_barrier(shader_bindable[index].image_view(), std::get<common::ImageMemoryBarrier>(input_info.barrier.value()));
		}
	}

	auto &output_views = render_target.get_views();
	for (const auto &[index, barrier] : attachment_barriers_)
	{
		if (std::holds_alternative<common::ImageMemoryBarrier>(barrier))
		{
			command_buffer.image_memory_barrier(output_views[index], std::get<common::ImageMemoryBarrier>(barrier));
		}
		else
		{
		}
	}

	if (type_ == PassType::kRaster)
	{
		command_buffer.begin_rendering(render_target);	
	}

	render_pass_->execute(command_buffer, render_frame, shader_bindable);

	if (type_ == PassType::kRaster)
	{
		command_buffer.end_rendering();
		if (!render_target_)
		{
			auto                      &views = render_target.get_views();
			common::ImageMemoryBarrier memory_barrier{};
			memory_barrier.old_layout      = vk::ImageLayout::eColorAttachmentOptimal;
			memory_barrier.new_layout      = vk::ImageLayout::ePresentSrcKHR;
			memory_barrier.src_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;
			memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
			memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eBottomOfPipe;

			command_buffer.image_memory_barrier(views[0], memory_barrier);
		}
	}

	for (auto &[handle, barrier] : release_barriers_)
	{

		if (std::holds_alternative<common::ImageMemoryBarrier>(barrier))
		{
			command_buffer.image_memory_barrier(render_graph_.get_resource_bindable(handle).image_view(), std::get<common::ImageMemoryBarrier>(barrier));
		}
		else
		{
		}
	}
}

PassInfo &PassNode::get_pass_info()
{
	return pass_info_;
}

PassType PassNode::get_type() const
{
	return type_;
}

void PassNode::set_render_target(std::unique_ptr<RenderTarget> &&render_target)
{
	render_target_ = std::move(render_target);
}

RenderTarget *PassNode::get_render_target()
{
	return render_target_.get();
}

void PassNode::set_batch_index(uint64_t batch_index)
{
	batch_index_ = batch_index;
}

int64_t PassNode::get_batch_index() const
{
	if (batch_index_ == -1)
	{
		throw std::runtime_error("Batch index not set");
	}
	return batch_index_;
}

void PassNode::add_bindable(uint32_t index, const ResourceHandle &handle, Barrier &&barrier)
{
	bindables_[index] = {handle, barrier};
}

void PassNode::add_bindable(uint32_t index, const ResourceHandle &handle)
{
	bindables_[index] = {handle, std::nullopt};
}

void PassNode::add_attachment_memory_barrier(uint32_t index, Barrier &&barrier)
{
	attachment_barriers_[index] = barrier;
}

void PassNode::add_release_barrier(const ResourceHandle &handle, Barrier &&barrier)
{
	release_barriers_[handle] = barrier;
}
}        // namespace xihe::rendering
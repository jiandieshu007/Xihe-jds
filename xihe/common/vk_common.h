#pragma once

#include <map>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_format_traits.hpp>

#include "common/logging.h"

template <class T>
using ShaderStageMap = std::map<VkShaderStageFlagBits, T>;

template <class T>
using BindingMap = std::map<uint32_t, std::map<uint32_t, T>>;

namespace xihe
{

struct BufferMemoryBarrier
{
	vk::PipelineStageFlags src_stage_mask  = vk::PipelineStageFlagBits::eBottomOfPipe;
	vk::PipelineStageFlags dst_stage_mask  = vk::PipelineStageFlagBits::eTopOfPipe;
	vk::AccessFlags        src_access_mask = {};
	vk::AccessFlags        dst_access_mask = {};
};

struct ImageMemoryBarrier
{
	vk::PipelineStageFlags src_stage_mask = vk::PipelineStageFlagBits::eBottomOfPipe;
	vk::PipelineStageFlags dst_stage_mask = vk::PipelineStageFlagBits::eTopOfPipe;
	vk::AccessFlags        src_access_mask;
	vk::AccessFlags        dst_access_mask;
	vk::ImageLayout        old_layout       = vk::ImageLayout::eUndefined;
	vk::ImageLayout        new_layout       = vk::ImageLayout::eUndefined;
	uint32_t               old_queue_family = VK_QUEUE_FAMILY_IGNORED;
	uint32_t               new_queue_family = VK_QUEUE_FAMILY_IGNORED;
};

struct LoadStoreInfo
{
	vk::AttachmentLoadOp  load_op  = vk::AttachmentLoadOp::eClear;
	vk::AttachmentStoreOp store_op = vk::AttachmentStoreOp::eStore;
};

inline bool is_depth_only_format(vk::Format format)
{
	return vk::componentCount(format) == 1 && (std::string(vk::componentName(format, 0)) == "D");
}

inline bool is_depth_stencil_format(vk::Format format)
{
	return vk::componentCount(format) == 2 && (std::string(vk::componentName(format, 0)) == "D") && (std::string(vk::componentName(format, 1)) == "S");
}

inline bool is_depth_format(vk::Format format)
{
	return std::string(vk::componentName(format, 0)) == "D";
}

inline bool is_dynamic_buffer_descriptor_type(vk::DescriptorType type)
{
		return type == vk::DescriptorType::eUniformBufferDynamic || type == vk::DescriptorType::eStorageBufferDynamic;
}

inline bool is_buffer_descriptor_type(vk::DescriptorType descriptor_type)
{
	return descriptor_type == vk::DescriptorType::eUniformBuffer || descriptor_type == vk::DescriptorType::eStorageBuffer || is_dynamic_buffer_descriptor_type(descriptor_type);
}

inline vk::Format get_suitable_depth_format(vk::PhysicalDevice             physical_device,
                                            bool                           depth_only                 = false,
                                            const std::vector<vk::Format> &depth_format_priority_list = {
                                                vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint, vk::Format::eD16Unorm})
{
	vk::Format depth_format = vk::Format::eUndefined;

	for (auto &format : depth_format_priority_list)
	{
		if (depth_only && !is_depth_only_format(format))
		{
			continue;
			;
		}
		vk::FormatProperties format_properties = physical_device.getFormatProperties(format);

		if (format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
		{
			depth_format = format;
			break;
		}
	}

	if (depth_format == vk::Format::eUndefined)
	{
		throw std::runtime_error("failed to find a suitable depth format!");
	}
	LOGI("depth format: {}", vk::to_string(depth_format).c_str());

	return depth_format;
}
}        // namespace xihe

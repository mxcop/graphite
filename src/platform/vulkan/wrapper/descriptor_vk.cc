#include "descriptor_vk.hh"

#include "graphite/render_graph.hh"
#include "graphite/vram_bank.hh"
#include "graphite/nodes/node.hh"
#include "translate_vk.hh"

#include "vulkan/wrapper/pipeline_cache_vk.hh"

/* Create a descriptor layout binding for a render target resource. */
VkDescriptorSetLayoutBinding render_target_layout(u32 slot, const Dependency& dep);

/* Create the descriptor layout for a render graph node. */
Result<VkDescriptorSetLayout> node_descriptor_layout(VkDevice device, const Node &node) {
    /* Descriptor bindings */
    std::vector<VkDescriptorSetLayoutBinding> bindings {};

    /* Create a binding for each dependency */
    for (const Dependency& dep : node.dependencies) {
        const ResourceType rtype = dep.resource.get_type();
        const u32 slot = (u32)bindings.size();

        switch (rtype) {
            case ResourceType::RenderTarget:
                bindings.push_back(render_target_layout(slot, dep));
                break;
            // case ResourceType::Buffer:
            //     bindings.push_back(buffer_binding((uint32)bindings.size(), dep));
            //     break;
            // case ResourceType::Image:
            //     bindings.push_back(image_binding((uint32)bindings.size(), dep));
            //     break;
            default:
                return Err("invalid resource type used in graph.");
        }
    }

    /* Descriptor set layout creation info (using push descriptors) */
    VkDescriptorSetLayoutCreateInfo layout_ci { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layout_ci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
    layout_ci.bindingCount = (u32)bindings.size();
    layout_ci.pBindings = bindings.data();

    /* Create the descriptor set layout */
    VkDescriptorSetLayout layout {};
    if (vkCreateDescriptorSetLayout(device, &layout_ci, nullptr, &layout) != VK_SUCCESS) {
        return Err("failed to create descriptor set layout for '%s' node.", node.label.data());
    }
    return Ok(layout);
}

/* Create a descriptor layout binding for a render target resource. */
VkDescriptorSetLayoutBinding render_target_layout(u32 slot, const Dependency& dep) {
    VkDescriptorSetLayoutBinding binding {};
    binding.binding = slot;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    binding.descriptorCount = 1u;
    binding.stageFlags = translate::stage_flags(dep.stages);
    return binding;
}

/* Push all descriptors for a render graph node onto the command buffer. */
Result<void> node_push_descriptors(const RenderGraph& rg, const Pipeline& pipeline, const Node &node) {
    /* Allocate memory for all the write commands and descriptors */
    const u32 binding_count = (u32)node.dependencies.size();
    std::vector<VkWriteDescriptorSet> writes(binding_count);
    std::vector<VkDescriptorBufferInfo> buffer_info(binding_count);
    std::vector<VkDescriptorImageInfo> texture_info(binding_count);

    /* Fill-in push descriptor write commands */
    u32 bindings = 0u;
    for (u32 i = 0u; i < binding_count; ++i) {
        const Dependency& dep = node.dependencies[i];
        const ResourceType rtype = dep.resource.get_type();

        /* Attachments are not bound as descriptors */
        if (has_flag(dep.flags, DependencyFlags::Attachment)) continue;

        /* Create the write command */
        VkWriteDescriptorSet& write = writes[bindings];
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorCount = 1u;
        write.dstBinding = bindings;

        switch (rtype) {
            case ResourceType::RenderTarget: {
                RenderTargetSlot& rt = rg.gpu->get_vram_bank().get_render_target(rg.target);
                texture_info[bindings].sampler = VK_NULL_HANDLE;
                texture_info[bindings].imageLayout = translate::desired_image_layout(dep.flags);
                texture_info[bindings].imageView = rt.view();
                write.descriptorType = translate::desired_image_type(dep.flags);
                write.pImageInfo = &texture_info[bindings];
                break;
            }
            // case ResourceType::eBuffer:
            //     buffer_push(dep, buffer_info[binding], write);
            //     break;
            // case ResourceType::eImage:
            //     image_push(dep, texture_info[binding], write);
            //     break;
            default:
                return Err("unknown resource type for push descriptors.");
        }

        bindings++;
    }

    /* Push the descriptor writes onto the command buffer */
    const VkPipelineBindPoint bind_point = translate::pipeline_bind_point(node.type);
    if (bind_point == VK_PIPELINE_BIND_POINT_MAX_ENUM) return Err("unknown pipeline bind point from node type.");
    vkCmdPushDescriptorSetKHR(rg.active_graph().cmd, bind_point, pipeline.layout, 0u, bindings, writes.data());
    return Ok();
}

/* Synchronize all descriptors for a render graph wave. */
Result<void> wave_sync_descriptors(const RenderGraph& rg, u32 start, u32 end) {
    /* Memory barriers */
    std::vector<VkBufferMemoryBarrier2> buf_barriers {};
    std::vector<VkImageMemoryBarrier2> tex_barriers {};

    /* Gather barrier information */
    for (u32 i = start; i < end; ++i) {
        const Node& node = *rg.nodes[rg.waves[i].lane];

        /* Add resource barriers for this execution wave */
        for (const Dependency& dep : node.dependencies) {
            const ResourceType rtype = dep.resource.get_type();

            switch (rtype) {
                case ResourceType::RenderTarget: {
                    RenderTargetSlot& rt = rg.gpu->get_vram_bank().get_render_target(rg.target);
                    VkImageMemoryBarrier2& barrier = tex_barriers.emplace_back(VkImageMemoryBarrier2 { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 });
                    barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
                    barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
                    barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
                    barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
                    barrier.oldLayout = rt.old_layout();
                    barrier.newLayout = translate::desired_image_layout(dep.flags);
                    rt.old_layout() = barrier.newLayout;
                    barrier.image = rt.image();
                    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u};
                    break;
                }
                // case ResourceType::eBuffer:
                //     buf_barriers.push_back(buffer_barrier(dep));
                //     break;
                // case ResourceType::eImage:
                //     tex_barriers.push_back(image_barrier(dep));
                //     break;
                default:
                    return Err("unknown resource type for sync barriers.");
            }
        }
    }

    /* Wave dependency info */
    VkDependencyInfo dep_info { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    dep_info.bufferMemoryBarrierCount = (u32)buf_barriers.size();
    dep_info.pBufferMemoryBarriers = buf_barriers.data();
    dep_info.imageMemoryBarrierCount = (u32)tex_barriers.size();
    dep_info.pImageMemoryBarriers = tex_barriers.data();
    
    /* Insert the wave sync barrier */
    vkCmdPipelineBarrier2KHR(rg.active_graph().cmd, &dep_info);
    return Ok();
}

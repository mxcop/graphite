#include "descriptor_vk.hh"

#include "graphite/render_graph.hh"
#include "graphite/vram_bank.hh"
#include "graphite/nodes/node.hh"
#include "translate_vk.hh"

#include "vulkan/wrapper/pipeline_cache_vk.hh"

/* Create a descriptor layout binding for a render target resource. */
VkDescriptorSetLayoutBinding render_target_layout(u32 slot, const Dependency& dep);

/* Create a descriptor layout binding for a buffer resource. */
VkDescriptorSetLayoutBinding buffer_layout(u32 slot, const Dependency& dep, const BufferUsage& buffer_usage);

/* Create a descriptor layout binding for an image resource. */
VkDescriptorSetLayoutBinding image_layout(u32 slot, const Dependency& dep, const TextureUsage& texture_usage);

/* Create a descriptor layout binding for an sampler resource. */
VkDescriptorSetLayoutBinding sampler_layout(u32 slot, const Dependency& dep);

/* Create the descriptor layout for a render graph node. */
Result<VkDescriptorSetLayout> node_descriptor_layout(GPUAdapter& gpu, const Node &node) {
    /* Descriptor bindings */
    std::vector<VkDescriptorSetLayoutBinding> bindings {};

    /* Create a binding for each dependency */
    for (const Dependency& dep : node.dependencies) {
        const ResourceType rtype = dep.resource.get_type();
        const u32 slot = (u32)bindings.size();

        /* Skip resources that don't need to be in the descriptor layout (ex: Vertex Buffers) */
        if (dep.is_unbound()) continue;

        switch (rtype) {
            case ResourceType::RenderTarget:
                bindings.push_back(render_target_layout(slot, dep));
                break;
            case ResourceType::Buffer: {
                const BufferSlot& buffer = gpu.get_vram_bank().buffers.get(dep.resource);
                bindings.push_back(buffer_layout(slot, dep, buffer.usage));
                break;
            }
            case ResourceType::Image: {
                const TextureSlot& texture = gpu.get_vram_bank().textures.get(dep.resource);
                bindings.push_back(image_layout(slot, dep, texture.usage));
                break;
            }
            case ResourceType::Sampler: {
                bindings.push_back(sampler_layout(slot, dep));
                break;
            }
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
    if (vkCreateDescriptorSetLayout(gpu.logical_device, &layout_ci, nullptr, &layout) != VK_SUCCESS) {
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

/* Create a descriptor layout binding for a buffer resource. */
VkDescriptorSetLayoutBinding buffer_layout(u32 slot, const Dependency& dep, const BufferUsage& buffer_usage) {
    VkDescriptorSetLayoutBinding binding {};
    binding.binding = slot;
    binding.descriptorType = translate::buffer_descriptor_type(buffer_usage);
    binding.descriptorCount = 1u;
    binding.stageFlags = translate::stage_flags(dep.stages);
    return binding;
}

/* Create a descriptor layout binding for an image resource. */
VkDescriptorSetLayoutBinding image_layout(u32 slot, const Dependency& dep, const TextureUsage& texture_usage) {
    VkDescriptorSetLayoutBinding binding {};
    binding.binding = slot;
    binding.descriptorType = translate::image_descriptor_type(dep, texture_usage);
    binding.descriptorCount = 1u;
    binding.stageFlags = translate::stage_flags(dep.stages);
    return binding;
}

/* Create a descriptor layout binding for an sampler resource. */
VkDescriptorSetLayoutBinding sampler_layout(u32 slot, const Dependency& dep) {
    VkDescriptorSetLayoutBinding binding {};
    binding.binding = slot;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
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

    /* Get the active VRAM bank */
    VRAMBank& bank = rg.gpu->get_vram_bank();

    /* Fill-in push descriptor write commands */
    u32 bindings = 0u;
    for (u32 i = 0u; i < binding_count; ++i) {
        const Dependency& dep = node.dependencies[i];
        const ResourceType rtype = dep.resource.get_type();

        /* Skip resources that don't need to be in the descriptor layout (ex: Vertex Buffers) */
        if (dep.is_unbound()) continue;

        /* Create the write command */
        VkWriteDescriptorSet& write = writes[bindings];
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorCount = 1u;
        write.dstBinding = bindings;

        switch (rtype) {
            case ResourceType::RenderTarget: {
                RenderTargetSlot& rt = bank.render_targets.get(rg.target);
                texture_info[bindings].sampler = VK_NULL_HANDLE;
                texture_info[bindings].imageLayout = translate::desired_image_layout(dep, TextureUsage::Storage | TextureUsage::Sampled);
                texture_info[bindings].imageView = rt.view();
                write.descriptorType = translate::desired_image_type(dep);
                write.pImageInfo = &texture_info[bindings];
                break;
            }
            case ResourceType::Buffer: {
                const BufferSlot& buffer = bank.buffers.get(dep.resource);
                buffer_info[bindings].buffer = buffer.buffer;
                buffer_info[bindings].offset = 0u;
                buffer_info[bindings].range = buffer.size;
                write.descriptorType = translate::buffer_descriptor_type(buffer.usage);
                write.pBufferInfo = &buffer_info[bindings];
                break;
            }
            case ResourceType::Image: {
                const ImageSlot& image = bank.images.get(dep.resource);
                const TextureSlot& texture = bank.textures.get(image.texture);
                texture_info[bindings].sampler = VK_NULL_HANDLE;
                texture_info[bindings].imageLayout = translate::desired_image_layout(dep, texture.usage);
                texture_info[bindings].imageView = image.view;
                write.descriptorType = translate::image_descriptor_type(dep, texture.usage);
                write.pImageInfo = &texture_info[bindings];
                break;
            }
            case ResourceType::Sampler: {
                const SamplerSlot& sampler = bank.samplers.get(dep.resource);
                texture_info[bindings].sampler = sampler.sampler;
                write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                write.pImageInfo = &texture_info[bindings];
                break;
            }
            default:
                return Err("unknown resource type for push descriptors.");
        }

        bindings++;
    }
    if (bindings < 1) return Ok();

    /* Push the descriptor writes onto the command buffer */
    const VkPipelineBindPoint bind_point = translate::pipeline_bind_point(node.type);
    if (bind_point == VK_PIPELINE_BIND_POINT_MAX_ENUM) return Err("unknown pipeline bind point from node type.");
    vkCmdPushDescriptorSetKHR(rg.active_graph().cmd, bind_point, pipeline.layout, 0u, bindings, writes.data());
    return Ok();
}

VkAccessFlagBits2 buffer_access_flags(const Dependency& dep, const BufferUsage usage) {
    switch (dep.usage) {
        case DependencyUsage::VertexBuffer:
            return VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
        case DependencyUsage::IndirectBuffer:
            return VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
        case DependencyUsage::Readonly:
            if (has_flag(usage, BufferUsage::Constant)) {
                return VK_ACCESS_2_UNIFORM_READ_BIT;
            } else if (has_flag(usage, BufferUsage::Storage)) {
                return VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
            } else return VK_ACCESS_2_NONE; /* Unreachable */
        case DependencyUsage::ReadWrite:
            return VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
        default:
            return VK_ACCESS_2_NONE; /* Unreachable */
    }
}

VkAccessFlagBits2 image_access_flags(const Dependency& dep, const TextureUsage usage) {
    switch (dep.usage) {
        case DependencyUsage::Readonly:
            if (has_flag(usage, TextureUsage::Storage)) {
                return VK_ACCESS_2_SHADER_STORAGE_READ_BIT; /* VK_IMAGE_LAYOUT_GENERAL */
            }
            return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT; /* VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL */
        case DependencyUsage::ReadWrite:
            return VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT; /* VK_IMAGE_LAYOUT_GENERAL */
        case DependencyUsage::ColorAttachment:
            return VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT; /* VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL */
        default:
            return VK_ACCESS_2_NONE; /* Unreachable */
    }
}

/* Synchronize all descriptors for a render graph wave. */
Result<void> wave_sync_descriptors(const RenderGraph& rg, u32 start, u32 end) {
    /* Memory barriers */
    std::vector<VkBufferMemoryBarrier2> buf_barriers {};
    std::vector<VkImageMemoryBarrier2> tex_barriers {};

    /* Get the active VRAM bank */
    VRAMBank& bank = rg.gpu->get_vram_bank();

    /* Gather barrier information */
    for (u32 i = start; i < end; ++i) {
        const Node& dst_node = *rg.nodes[rg.waves[i].lane];

        /* Add resource barriers for this execution wave */
        for (const Dependency& dst_dep : dst_node.dependencies) {
            /* Get the source dependency */
            const Node* src_node = dst_dep.source_node == UINT_MAX ? nullptr : rg.nodes[dst_dep.source_node];
            const Dependency* src_dep = src_node ? &src_node->dependencies[dst_dep.source_index] : nullptr;
            
            /* Get the source and destination resource types */
            const ResourceType src_rtype = src_dep ? src_dep->resource.get_type() : ResourceType::Invalid;
            const ResourceType dst_rtype = dst_dep.resource.get_type();
            
            /* Get the pipeline stage flags for the source and destination */
            const VkPipelineStageFlags2 src_stage = src_node ? translate::stage_mask(src_dep->usage, src_dep->stages, src_node->type) : VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            const VkPipelineStageFlags2 dst_stage = translate::stage_mask(dst_dep.usage, dst_dep.stages, dst_node.type);

            VkAccessFlagBits2 src_access {};
            if (src_dep) {
                switch (src_rtype) {
                    case ResourceType::RenderTarget:
                        src_access = image_access_flags(*src_dep, TextureUsage::Storage | TextureUsage::Sampled);
                        break;
                    case ResourceType::Buffer:
                        src_access = buffer_access_flags(*src_dep, bank.buffers.get(src_dep->resource).usage);
                        break;
                    case ResourceType::Image:
                        src_access = image_access_flags(*src_dep, bank.textures.get(bank.images.get(src_dep->resource).texture).usage);
                        break;
                }
            }

            VkAccessFlagBits2 dst_access {};
            switch (dst_rtype) {
                case ResourceType::RenderTarget:
                    dst_access = image_access_flags(dst_dep, TextureUsage::Storage | TextureUsage::Sampled);
                    break;
                case ResourceType::Buffer:
                    dst_access = buffer_access_flags(dst_dep, bank.buffers.get(dst_dep.resource).usage);
                    break;
                case ResourceType::Image:
                    dst_access = image_access_flags(dst_dep, bank.textures.get(bank.images.get(dst_dep.resource).texture).usage);
                    break;
            }

            switch (dst_rtype) {
                case ResourceType::RenderTarget: {
                    RenderTargetSlot& rt = bank.render_targets.get(rg.target);
                    VkImageMemoryBarrier2& barrier = tex_barriers.emplace_back(VkImageMemoryBarrier2 { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 });
                    barrier.srcStageMask = src_stage;
                    barrier.srcAccessMask = src_access;
                    barrier.dstStageMask = dst_stage;
                    barrier.dstAccessMask = dst_access;
                    barrier.oldLayout = rt.old_layout();
                    barrier.newLayout = translate::desired_image_layout(dst_dep, TextureUsage::Storage | TextureUsage::Sampled);
                    rt.old_layout() = barrier.newLayout;
                    barrier.image = rt.image();
                    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u};
                    break;
                }
                case ResourceType::Buffer: {
                    const BufferSlot& buffer = bank.buffers.get(dst_dep.resource);
                    VkBufferMemoryBarrier2& barrier = buf_barriers.emplace_back(VkBufferMemoryBarrier2 { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 });
                    barrier.srcStageMask = src_stage;
                    barrier.srcAccessMask = src_access;
                    barrier.dstStageMask = dst_stage;
                    barrier.dstAccessMask = dst_access;
                    barrier.buffer = buffer.buffer;
                    barrier.offset = 0u;
                    barrier.size = buffer.size;
                    break;
                }
                case ResourceType::Image: {
                    const ImageSlot& image = bank.images.get(dst_dep.resource);
                    TextureSlot& texture = bank.textures.get(image.texture);
                    VkImageMemoryBarrier2& barrier = tex_barriers.emplace_back(VkImageMemoryBarrier2 { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 });
                    barrier.srcStageMask = src_stage;
                    barrier.srcAccessMask = src_access;
                    barrier.dstStageMask = dst_stage;
                    barrier.dstAccessMask = dst_access;
                    barrier.oldLayout = texture.layout;
                    barrier.newLayout = translate::desired_image_layout(dst_dep, texture.usage);
                    texture.layout = barrier.newLayout;
                    barrier.image = texture.image;
                    barrier.subresourceRange = image.sub_range;
                    break;
                }
                case ResourceType::Sampler: break;
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

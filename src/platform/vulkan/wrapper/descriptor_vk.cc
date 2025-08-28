#include "descriptor_vk.hh"

#include "graphite/nodes/node.hh"
#include "translate_vk.hh"

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

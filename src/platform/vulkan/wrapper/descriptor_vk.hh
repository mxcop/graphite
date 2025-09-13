#pragma once

#include "vulkan/api_vk.hh" /* Vulkan API */
#include "graphite/utils/result.hh"

struct Pipeline;
struct RenderTarget;

class Node;
class VRAMBank;

/* Create the descriptor layout for a render graph node. */
Result<VkDescriptorSetLayout> node_descriptor_layout(VkDevice device, const Node& node);

/* Push all descriptors for a render graph node onto the command buffer. */
Result<void> node_push_descriptors(VkCommandBuffer cmd, RenderTarget& target, const VRAMBank& bank, const Pipeline& pipeline, const Node& node);

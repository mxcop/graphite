#include "shader_vk.hh"

#include <vector>
#include <fstream> /* std::ifstream */

namespace shader {

using Bytes = std::vector<char>;

/* TODO: Allow the user to set their own "read_binary_file" function! */

/* Read the binary data of a file into a vector. */
inline Bytes read_binary_file(const std::string_view path) {
    /* Try to open the file */
    std::ifstream file{path.data(), std::ios::ate | std::ios::binary};
    if (!file.is_open()) return {};

    /* Get the file size & make a buffer */
    const size_t size = file.tellg();
    Bytes buffer(size);

    /* Reset the read position & read into the buffer */
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), size);

    file.close();
    return buffer;
}

/* Convert a shader alias to an asset path. */
std::string shader_path(const std::string_view alias) {
    /* Make sure the alias starts with `shader:` */
    if (alias._Starts_with("shader:") == false) return "";

    /* Convert the alias to an asset path */
    std::string path = "shaders/slang" + std::string(alias.substr(6u, alias.size() - 6u));
    for (char& ch : path) ch = (ch == ':' ? '/' : ch);
    return path + ".spv";
}

Result<VkShaderModule> from_alias(const VkDevice device, const std::string_view alias) {
    /* Try to load the shader as binary */
    const Bytes bytes = read_binary_file(shader_path(alias));
    if (bytes.empty()) return Err("failed to load shader file.");

    /* Shader module create info */
    VkShaderModuleCreateInfo create_info { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    create_info.codeSize = bytes.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(bytes.data());

    /* Create the shader module */
    VkShaderModule module {};
    if (vkCreateShaderModule(device, &create_info, nullptr, &module) != VK_SUCCESS) {
        return Err("failed to create shader module.");
    }
    return Ok(module);
}

} /* shader */

#include "raster_node.hh"

RasterNode::RasterNode(std::string_view label, std::string_view vx_path, std::string_view px_path)
    : Node(label, NodeType::Raster), vertex_path(vx_path), pixel_path(px_path) {}

RasterNode& RasterNode::write(BindHandle resource, ShaderStages stages) {
    /* Insert the write dependency */
    dependencies.emplace_back(resource, DependencyUsage::ReadWrite, stages);
    return *this;
}

RasterNode& RasterNode::read(BindHandle resource, ShaderStages stages) {
    /* Insert the read dependency */
    dependencies.emplace_back(resource, DependencyUsage::Readonly, stages);
    return *this;
}

RasterNode& RasterNode::attach(BindHandle resource) {
    dependencies.emplace_back(resource, DependencyUsage::ColorAttachment, DependencyStages::Pixel);
    return *this;
}

RasterNode& RasterNode::depth_stencil(Image image, bool test, bool write) {
    dependencies.emplace_back(image, DependencyUsage::DepthStencil, DependencyStages::Pixel);
    depth_stencil_image = image;
    depth_test = test;
    depth_write = write;
    return *this;
}

RasterNode& RasterNode::raster_extent(const u32 w, const u32 h, const u32 x, const u32 y) {
    raster_w = w;
    raster_h = h;
    raster_x = x;
    raster_y = y;
    return *this;
}

DrawCall& RasterNode::draw(
    const Buffer vertex_buffer, const u32 vertex_count, const u32 vertex_offset, const u32 instance_count,
    const u32 instance_offset
) {
    return draws.emplace_back(*this, vertex_buffer, vertex_count, vertex_offset, instance_count, instance_offset);
}

DrawCall& RasterNode::draw_indirect(const Buffer vertex_buffer, const Buffer indirect_buffer) {
    return draws.emplace_back(*this, vertex_buffer, indirect_buffer);
}

RasterNode& RasterNode::attribute(const AttrFormat format) {
    attributes.emplace_back(format);
    return *this;
}

RasterNode& RasterNode::topology(const Topology type) {
    prim_topology = type;
    return *this;
}

RasterNode& RasterNode::load_op_color(const LoadOp op) {
    pixel_load_op = op;
    return *this;
}

RasterNode& RasterNode::load_op_depth(const LoadOp op) {
    depth_load_op = op;
    return *this;
}

RasterNode& RasterNode::input_rate(const VertexInputRate rate) {
    vertex_input_rate = rate;
    return *this;
}

RasterNode& RasterNode::alpha_blending(const bool blend) {
    alpha_blend = blend;
    return *this;
}

DrawCall::DrawCall(
    RasterNode& parent_pass, const Buffer vertex_buffer, const u32 vertex_count, const u32 vertex_offset,
    const u32 instance_count, const u32 instance_offset
)
    : parent_pass(parent_pass),
      vertex_buffer(vertex_buffer),
      vertex_count(vertex_count),
      vertex_offset(vertex_offset),
      instance_count(instance_count),
      instance_offset(instance_offset) {
    if (vertex_buffer.is_null()) return;
    parent_pass.dependencies.emplace_back(
        vertex_buffer, DependencyUsage::VertexBuffer, DependencyStages::Vertex
    );
}

DrawCall::DrawCall(RasterNode& parent_pass, const Buffer vertex_buffer, const Buffer indirect_buffer)
    : parent_pass(parent_pass), vertex_buffer(vertex_buffer), indirect_buffer(indirect_buffer) {
    if (vertex_buffer.is_null()) return;
    parent_pass.dependencies.emplace_back(
        vertex_buffer, DependencyUsage::VertexBuffer, DependencyStages::Vertex
    );
}
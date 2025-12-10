#include "raster_node.hh"

RasterNode::RasterNode(std::string_view label, std::string_view vx_path, std::string_view px_path)
    : Node(label, NodeType::Raster), vertex_path(vx_path), pixel_path(px_path) {}

RasterNode& RasterNode::write(BindHandle resource, ShaderStages stages) {
    /* Insert the write dependency */
    dependencies.emplace_back(resource, DependencyFlags::None, stages);
    return *this;
}

RasterNode& RasterNode::read(BindHandle resource, ShaderStages stages) {
    /* Insert the read dependency */
    dependencies.emplace_back(resource, DependencyFlags::Readonly, stages);
    return *this;
}

RasterNode& RasterNode::attach(BindHandle resource) {
    dependencies.emplace_back(resource, DependencyFlags::Attachment | DependencyFlags::Unbound, DependencyStages::Pixel);
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

RasterNode& RasterNode::attribute(const AttrFormat format) {
    attributes.emplace_back(format);
    return *this;
}

RasterNode& RasterNode::topology(const Topology type) {
    prim_topology = type;
    return *this;
}

RasterNode& RasterNode::load_op(const LoadOp op) {
    pixel_load_op = op;
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
    parent_pass.dependencies.emplace_back(
        vertex_buffer, DependencyFlags::Readonly | DependencyFlags::Unbound, DependencyStages::Vertex
    );
}

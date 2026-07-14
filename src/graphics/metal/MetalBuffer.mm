// Metal Buffer Implementation

#ifdef __APPLE__

#import "MetalBuffer.hpp"
#import "MetalState.hpp"
#import <Metal/Metal.h>

namespace GE::Graphics::Metal {

// --- MetalVertexBuffer ---

MetalVertexBuffer::~MetalVertexBuffer() {
    if (this->m_buffer) CFRelease(this->m_buffer);
}

void MetalVertexBuffer::create(const void* data, size_t size, BufferUsage /*usage*/) {
    this->m_device = MetalState::getDevice();
    id<MTLDevice> device = (__bridge id<MTLDevice>)this->m_device;
    if (!device) return;

    this->m_size = size;

    if (data) {
        id<MTLBuffer> buf = [device newBufferWithBytes:data
                                               length:size
                                              options:MTLResourceStorageModeShared];
        this->m_buffer = (__bridge_retained void*)buf;
    } else {
        id<MTLBuffer> buf = [device newBufferWithLength:size
                                               options:MTLResourceStorageModeShared];
        this->m_buffer = (__bridge_retained void*)buf;
    }
}

void MetalVertexBuffer::update(const void* data, size_t size, size_t offset) {
    if (!this->m_buffer || !data) return;
    id<MTLBuffer> buf = (__bridge id<MTLBuffer>)this->m_buffer;
    memcpy((uint8_t*)[buf contents] + offset, data, size);
}

void MetalVertexBuffer::bind() {
    this->m_encoder = MetalState::getEncoder();
    if (!this->m_encoder || !this->m_buffer) return;
    id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)this->m_encoder;
    id<MTLBuffer> buf = (__bridge id<MTLBuffer>)this->m_buffer;
    [encoder setVertexBuffer:buf offset:0 atIndex:0];
}

void MetalVertexBuffer::unbind() {
    // No-op in Metal
}

// --- MetalIndexBuffer ---

MetalIndexBuffer::~MetalIndexBuffer() {
    if (this->m_buffer) CFRelease(this->m_buffer);
}

void MetalIndexBuffer::create(const uint32_t* data, size_t count, BufferUsage /*usage*/) {
    this->m_device = MetalState::getDevice();
    id<MTLDevice> device = (__bridge id<MTLDevice>)this->m_device;
    if (!device) return;

    this->m_count = count;
    size_t size = count * sizeof(uint32_t);

    if (data) {
        id<MTLBuffer> buf = [device newBufferWithBytes:data
                                               length:size
                                              options:MTLResourceStorageModeShared];
        this->m_buffer = (__bridge_retained void*)buf;
    } else {
        id<MTLBuffer> buf = [device newBufferWithLength:size
                                               options:MTLResourceStorageModeShared];
        this->m_buffer = (__bridge_retained void*)buf;
    }
}

void MetalIndexBuffer::bind() {
    // Index buffer is bound at draw time in Metal
}

void MetalIndexBuffer::unbind() {}

// --- MetalVertexArray ---

void MetalVertexArray::create() {
    // No VAO concept in Metal; this is a logical grouping
}

void MetalVertexArray::bind() {
    if (this->m_vbo) {
        auto* mtlVBO = static_cast<MetalVertexBuffer*>(this->m_vbo.get());
        mtlVBO->bind();
    }
}

void MetalVertexArray::unbind() {}

void MetalVertexArray::addVertexBuffer(std::shared_ptr<IVertexBuffer> vbo) {
    this->m_vbo = vbo;
}

void MetalVertexArray::setIndexBuffer(std::shared_ptr<IIndexBuffer> ibo) {
    this->m_ibo = ibo;
}

void MetalVertexArray::setupVertexLayout() {
    // Vertex layout is defined in the pipeline state (MetalShader)
    // Nothing to do here
}

int MetalVertexArray::toMTL(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Triangles: return 3; // MTLPrimitiveTypeTriangle
        case PrimitiveType::Lines:     return 1; // MTLPrimitiveTypeLine
        case PrimitiveType::LineStrip: return 2; // MTLPrimitiveTypeLineStrip
        case PrimitiveType::Points:    return 0; // MTLPrimitiveTypePoint
    }
    return 3;
}

void MetalVertexArray::draw(PrimitiveType primitive, size_t count) {
    this->m_encoder = MetalState::getEncoder();
    if (!this->m_encoder) return;
    bind();

    id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)this->m_encoder;
    [encoder drawPrimitives:(MTLPrimitiveType)toMTL(primitive)
                vertexStart:0
                vertexCount:count];
}

void MetalVertexArray::drawIndexed(PrimitiveType primitive) {
    this->m_encoder = MetalState::getEncoder();
    if (!this->m_encoder || !this->m_ibo) return;
    bind();

    auto* mtlIBO = static_cast<MetalIndexBuffer*>(this->m_ibo.get());
    id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)this->m_encoder;
    id<MTLBuffer> indexBuf = (__bridge id<MTLBuffer>)mtlIBO->getBuffer();

    [encoder drawIndexedPrimitives:(MTLPrimitiveType)toMTL(primitive)
                        indexCount:this->m_ibo->getCount()
                         indexType:MTLIndexTypeUInt32
                       indexBuffer:indexBuf
                 indexBufferOffset:0];
}

} // namespace GE::Graphics::Metal

#endif // __APPLE__

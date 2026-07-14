// Metal Texture Implementation

#ifdef __APPLE__

#import "MetalTexture.hpp"
#import "MetalState.hpp"
#import <Metal/Metal.h>

namespace GE::Graphics::Metal {

MetalTexture::~MetalTexture() {
    if (this->m_texture) CFRelease(this->m_texture);
    if (this->m_sampler) CFRelease(this->m_sampler);
}

bool MetalTexture::loadFromFile(const std::string& /*path*/) {
    // Placeholder: create 1x1 white texture
    unsigned char white[] = {255, 255, 255, 255};
    return create(1, 1, TextureFormat::RGBA8, white);
}

bool MetalTexture::create(int width, int height, TextureFormat format, const void* data) {
    this->m_device = MetalState::getDevice();
    id<MTLDevice> device = (__bridge id<MTLDevice>)this->m_device;
    if (!device) return false;

    this->m_width = width;
    this->m_height = height;

    MTLPixelFormat pixelFormat;
    switch (format) {
        case TextureFormat::RGB8:    pixelFormat = MTLPixelFormatRGBA8Unorm; break; // Metal doesn't have RGB8
        case TextureFormat::RGBA8:   pixelFormat = MTLPixelFormatRGBA8Unorm; break;
        case TextureFormat::Depth24: pixelFormat = MTLPixelFormatDepth32Float; break;
        default:                     pixelFormat = MTLPixelFormatRGBA8Unorm; break;
    }

    MTLTextureDescriptor* desc = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:pixelFormat
        width:width
        height:height
        mipmapped:NO];
    desc.usage = MTLTextureUsageShaderRead;

    id<MTLTexture> tex = [device newTextureWithDescriptor:desc];
    if (data) {
        NSUInteger bytesPerRow = 4 * width; // RGBA
        [tex replaceRegion:MTLRegionMake2D(0, 0, width, height)
               mipmapLevel:0
                 withBytes:data
               bytesPerRow:bytesPerRow];
    }

    if (this->m_texture) CFRelease(this->m_texture);
    this->m_texture = (__bridge_retained void*)tex;

    createSampler(TextureFilter::Linear, TextureFilter::Linear,
                  TextureWrap::Repeat, TextureWrap::Repeat);
    return true;
}

void MetalTexture::bind(uint32_t slot) {
    this->m_encoder = MetalState::getEncoder();
    if (!this->m_encoder || !this->m_texture) return;
    id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)this->m_encoder;
    id<MTLTexture> tex = (__bridge id<MTLTexture>)this->m_texture;
    [encoder setFragmentTexture:tex atIndex:slot];
    if (this->m_sampler) {
        id<MTLSamplerState> sampler = (__bridge id<MTLSamplerState>)this->m_sampler;
        [encoder setFragmentSamplerState:sampler atIndex:slot];
    }
}

void MetalTexture::unbind() {
    // No-op in Metal
}

void MetalTexture::setFilter(TextureFilter min, TextureFilter mag) {
    createSampler(min, mag, TextureWrap::Repeat, TextureWrap::Repeat);
}

void MetalTexture::setWrap(TextureWrap s, TextureWrap t) {
    createSampler(TextureFilter::Linear, TextureFilter::Linear, s, t);
}

void MetalTexture::createSampler(TextureFilter minFilter, TextureFilter /*magFilter*/,
                                  TextureWrap wrapS, TextureWrap wrapT) {
    id<MTLDevice> device = (__bridge id<MTLDevice>)this->m_device;
    if (!device) return;

    MTLSamplerDescriptor* desc = [[MTLSamplerDescriptor alloc] init];

    switch (minFilter) {
        case TextureFilter::Nearest:      desc.minFilter = MTLSamplerMinMagFilterNearest; break;
        case TextureFilter::Linear:       desc.minFilter = MTLSamplerMinMagFilterLinear; break;
        case TextureFilter::LinearMipmap: desc.minFilter = MTLSamplerMinMagFilterLinear;
                                          desc.mipFilter = MTLSamplerMipFilterLinear; break;
    }
    desc.magFilter = MTLSamplerMinMagFilterLinear;

    auto toWrap = [](TextureWrap w) -> MTLSamplerAddressMode {
        switch (w) {
            case TextureWrap::Repeat: return MTLSamplerAddressModeRepeat;
            case TextureWrap::Clamp:  return MTLSamplerAddressModeClampToEdge;
            case TextureWrap::Mirror: return MTLSamplerAddressModeMirrorRepeat;
        }
        return MTLSamplerAddressModeRepeat;
    };

    desc.sAddressMode = toWrap(wrapS);
    desc.tAddressMode = toWrap(wrapT);

    if (this->m_sampler) CFRelease(this->m_sampler);
    id<MTLSamplerState> sampler = [device newSamplerStateWithDescriptor:desc];
    this->m_sampler = (__bridge_retained void*)sampler;
}

} // namespace GE::Graphics::Metal

#endif // __APPLE__

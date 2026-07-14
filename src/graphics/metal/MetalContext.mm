// Metal Context Implementation

#ifdef __APPLE__

#import "MetalContext.hpp"
#import "MetalState.hpp"
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <GLFW/glfw3.h>

// GLFW native access
#define GLFW_EXPOSE_NATIVE_COCOA
#import <GLFW/glfw3native.h>

#import <Cocoa/Cocoa.h>

namespace GE::Graphics::Metal {

bool MetalContext::init(int width, int height, const std::string& title) {
    if (!glfwInit()) {
        std::cerr << "[MetalContext] Failed to initialize GLFW\n";
        return false;
    }

    // Tell GLFW not to create an OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    this->m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!this->m_window) {
        std::cerr << "[MetalContext] Failed to create window\n";
        glfwTerminate();
        return false;
    }

    this->m_width = width;
    this->m_height = height;

    // Create Metal device
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        std::cerr << "[MetalContext] Failed to create Metal device\n";
        return false;
    }
    this->m_device = (__bridge_retained void*)device;
    MetalState::setDevice(this->m_device);

    // Create command queue
    id<MTLCommandQueue> queue = [device newCommandQueue];
    this->m_commandQueue = (__bridge_retained void*)queue;

    // Get the native NSWindow and set up CAMetalLayer
    NSWindow* nsWindow = glfwGetCocoaWindow(this->m_window);
    CAMetalLayer* metalLayer = [CAMetalLayer layer];
    metalLayer.device = device;
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.framebufferOnly = YES;

    // Use actual framebuffer size (accounts for Retina/HiDPI)
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(this->m_window, &fbWidth, &fbHeight);
    metalLayer.drawableSize = CGSizeMake(fbWidth, fbHeight);
    this->m_width = fbWidth;
    this->m_height = fbHeight;

    nsWindow.contentView.wantsLayer = YES;
    nsWindow.contentView.layer = metalLayer;
    this->m_layer = (__bridge_retained void*)metalLayer;

    createDepthTexture();

    this->m_deviceName = [[device name] UTF8String];
    std::cout << "[MetalContext] Metal device: " << this->m_deviceName << "\n";
    return true;
}

void MetalContext::createDepthTexture() {
    id<MTLDevice> device = (__bridge id<MTLDevice>)this->m_device;

    MTLTextureDescriptor* depthDesc = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
        width:this->m_width
        height:this->m_height
        mipmapped:NO];
    depthDesc.usage = MTLTextureUsageRenderTarget;
    depthDesc.storageMode = MTLStorageModePrivate;

    id<MTLTexture> depthTex = [device newTextureWithDescriptor:depthDesc];
    if (this->m_depthTexture) {
        CFRelease(this->m_depthTexture);
    }
    this->m_depthTexture = (__bridge_retained void*)depthTex;
}

void MetalContext::shutdown() {
    if (this->m_depthTexture) { CFRelease(this->m_depthTexture); this->m_depthTexture = nullptr; }
    if (this->m_commandQueue) { CFRelease(this->m_commandQueue); this->m_commandQueue = nullptr; }
    if (this->m_layer) { CFRelease(this->m_layer); this->m_layer = nullptr; }
    if (this->m_device) { CFRelease(this->m_device); this->m_device = nullptr; }

    if (this->m_window) {
        glfwDestroyWindow(this->m_window);
        this->m_window = nullptr;
        glfwTerminate();
    }
}

void MetalContext::beginFrame() {
    CAMetalLayer* layer = (__bridge CAMetalLayer*)this->m_layer;
    id<CAMetalDrawable> drawable = [layer nextDrawable];
    this->m_currentDrawable = (__bridge_retained void*)drawable;

    id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)this->m_commandQueue;
    id<MTLCommandBuffer> cmdBuffer = [queue commandBuffer];
    this->m_currentCommandBuffer = (__bridge_retained void*)cmdBuffer;
    MetalState::setCommandBuffer(this->m_currentCommandBuffer);

    // Create render pass descriptor (retained for ImGui to use later in the frame)
    MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
    passDesc.colorAttachments[0].texture = drawable.texture;
    passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
    passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
    passDesc.colorAttachments[0].clearColor = MTLClearColorMake(
        this->m_clearColor.r, this->m_clearColor.g, this->m_clearColor.b, this->m_clearColor.a);

    if (this->m_depthTestEnabled && this->m_depthTexture) {
        id<MTLTexture> depthTex = (__bridge id<MTLTexture>)this->m_depthTexture;
        passDesc.depthAttachment.texture = depthTex;
        passDesc.depthAttachment.loadAction = MTLLoadActionClear;
        passDesc.depthAttachment.storeAction = MTLStoreActionDontCare;
        passDesc.depthAttachment.clearDepth = 1.0;
    }
    if (this->m_currentRenderPassDesc) CFRelease(this->m_currentRenderPassDesc);
    this->m_currentRenderPassDesc = (__bridge_retained void*)passDesc;
    MetalState::setRenderPassDescriptor(this->m_currentRenderPassDesc);

    id<MTLRenderCommandEncoder> encoder = [cmdBuffer renderCommandEncoderWithDescriptor:passDesc];
    [encoder setViewport:(MTLViewport){0, 0, (double)this->m_width, (double)this->m_height, 0.0, 1.0}];
    this->m_currentEncoder = (__bridge_retained void*)encoder;
    MetalState::setEncoder(this->m_currentEncoder);
}

void MetalContext::endFrame() {
    MetalState::setEncoder(nullptr);
    MetalState::setRenderPassDescriptor(nullptr);
    id<MTLRenderCommandEncoder> encoder = (__bridge_transfer id<MTLRenderCommandEncoder>)this->m_currentEncoder;
    [encoder endEncoding];
    this->m_currentEncoder = nullptr;

    if (this->m_currentRenderPassDesc) {
        CFRelease(this->m_currentRenderPassDesc);
        this->m_currentRenderPassDesc = nullptr;
    }

    id<MTLCommandBuffer> cmdBuffer = (__bridge_transfer id<MTLCommandBuffer>)this->m_currentCommandBuffer;
    id<CAMetalDrawable> drawable = (__bridge_transfer id<CAMetalDrawable>)this->m_currentDrawable;
    [cmdBuffer presentDrawable:drawable];
    [cmdBuffer commit];
    this->m_currentCommandBuffer = nullptr;
    this->m_currentDrawable = nullptr;

    glfwPollEvents();
}

bool MetalContext::shouldClose() {
    return glfwWindowShouldClose(this->m_window);
}

void MetalContext::setClearColor(const Color& color) {
    this->m_clearColor = color;
}

void MetalContext::setViewport(int x, int y, int w, int h) {
    this->m_width = w;
    this->m_height = h;
    if (this->m_currentEncoder) {
        id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)this->m_currentEncoder;
        [encoder setViewport:(MTLViewport){(double)x, (double)y, (double)w, (double)h, 0.0, 1.0}];
    }
}

void MetalContext::setDepthTest(bool enabled) {
    this->m_depthTestEnabled = enabled;
}

void MetalContext::setBlending(bool enabled) {
    this->m_blendingEnabled = enabled;
}

} // namespace GE::Graphics::Metal

#endif // __APPLE__

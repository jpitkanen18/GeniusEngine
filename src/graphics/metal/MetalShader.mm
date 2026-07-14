// Metal Shader Implementation

#ifdef __APPLE__

#import "MetalShader.hpp"
#import "MetalContext.hpp"
#import "MetalState.hpp"
#import <Metal/Metal.h>
#import <fstream>
#import <sstream>

namespace GE::Graphics::Metal {

MetalShader::~MetalShader() {
    if (this->m_pipelineState) CFRelease(this->m_pipelineState);
    if (this->m_depthStencilState) CFRelease(this->m_depthStencilState);
    if (this->m_vertexFunction) CFRelease(this->m_vertexFunction);
    if (this->m_fragmentFunction) CFRelease(this->m_fragmentFunction);
    if (this->m_library) CFRelease(this->m_library);
}

bool MetalShader::loadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc) {
    // Combine both into one MSL library (Metal uses a single source with named functions)
    // vertexSrc is expected to contain the vertex function named "vertexMain"
    // fragmentSrc is expected to contain the fragment function named "fragmentMain"
    std::string combined = vertexSrc + "\n" + fragmentSrc;

    this->m_device = MetalState::getDevice();
    id<MTLDevice> device = (__bridge id<MTLDevice>)this->m_device;
    if (!device) {
        std::cerr << "[MetalShader] No device set\n";
        return false;
    }

    NSError* error = nil;
    NSString* source = [NSString stringWithUTF8String:combined.c_str()];
    id<MTLLibrary> library = [device newLibraryWithSource:source options:nil error:&error];

    if (!library) {
        std::cerr << "[MetalShader] Compile error: " << [[error localizedDescription] UTF8String] << "\n";
        return false;
    }
    this->m_library = (__bridge_retained void*)library;

    id<MTLFunction> vertFunc = [library newFunctionWithName:@"vertexMain"];
    id<MTLFunction> fragFunc = [library newFunctionWithName:@"fragmentMain"];

    if (!vertFunc || !fragFunc) {
        std::cerr << "[MetalShader] Could not find vertexMain/fragmentMain functions\n";
        return false;
    }

    this->m_vertexFunction = (__bridge_retained void*)vertFunc;
    this->m_fragmentFunction = (__bridge_retained void*)fragFunc;

    return createPipelineState();
}

bool MetalShader::loadFromFile(const std::string& vertexPath, const std::string& fragmentPath) {
    auto readFile = [](const std::string& path) -> std::string {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    };

    std::string vSrc = readFile(vertexPath);
    std::string fSrc = readFile(fragmentPath);
    if (vSrc.empty() || fSrc.empty()) return false;
    return loadFromSource(vSrc, fSrc);
}

bool MetalShader::createPipelineState() {
    id<MTLDevice> device = (__bridge id<MTLDevice>)this->m_device;
    id<MTLFunction> vertFunc = (__bridge id<MTLFunction>)this->m_vertexFunction;
    id<MTLFunction> fragFunc = (__bridge id<MTLFunction>)this->m_fragmentFunction;

    // Vertex descriptor matching our Vertex struct
    MTLVertexDescriptor* vertDesc = [[MTLVertexDescriptor alloc] init];

    // Position: float3 at offset 0
    vertDesc.attributes[0].format = MTLVertexFormatFloat3;
    vertDesc.attributes[0].offset = offsetof(Vertex, position);
    vertDesc.attributes[0].bufferIndex = 0;

    // Normal: float3
    vertDesc.attributes[1].format = MTLVertexFormatFloat3;
    vertDesc.attributes[1].offset = offsetof(Vertex, normal);
    vertDesc.attributes[1].bufferIndex = 0;

    // TexCoord: float2
    vertDesc.attributes[2].format = MTLVertexFormatFloat2;
    vertDesc.attributes[2].offset = offsetof(Vertex, texCoord);
    vertDesc.attributes[2].bufferIndex = 0;

    // Color: float4
    vertDesc.attributes[3].format = MTLVertexFormatFloat4;
    vertDesc.attributes[3].offset = offsetof(Vertex, color);
    vertDesc.attributes[3].bufferIndex = 0;

    vertDesc.layouts[0].stride = sizeof(Vertex);
    vertDesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

    // Pipeline descriptor
    MTLRenderPipelineDescriptor* pipeDesc = [[MTLRenderPipelineDescriptor alloc] init];
    pipeDesc.vertexFunction = vertFunc;
    pipeDesc.fragmentFunction = fragFunc;
    pipeDesc.vertexDescriptor = vertDesc;
    pipeDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    pipeDesc.colorAttachments[0].blendingEnabled = YES;
    pipeDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    pipeDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipeDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
    pipeDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipeDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

    NSError* error = nil;
    id<MTLRenderPipelineState> pso = [device newRenderPipelineStateWithDescriptor:pipeDesc error:&error];
    if (!pso) {
        std::cerr << "[MetalShader] Pipeline error: " << [[error localizedDescription] UTF8String] << "\n";
        return false;
    }
    this->m_pipelineState = (__bridge_retained void*)pso;

    // Depth stencil state
    MTLDepthStencilDescriptor* depthDesc = [[MTLDepthStencilDescriptor alloc] init];
    depthDesc.depthCompareFunction = MTLCompareFunctionLess;
    depthDesc.depthWriteEnabled = YES;
    id<MTLDepthStencilState> dss = [device newDepthStencilStateWithDescriptor:depthDesc];
    this->m_depthStencilState = (__bridge_retained void*)dss;

    return true;
}

void MetalShader::bind() {
    this->m_encoder = MetalState::getEncoder();
    if (!this->m_encoder || !this->m_pipelineState) return;

    id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)this->m_encoder;
    id<MTLRenderPipelineState> pso = (__bridge id<MTLRenderPipelineState>)this->m_pipelineState;
    id<MTLDepthStencilState> dss = (__bridge id<MTLDepthStencilState>)this->m_depthStencilState;

    [encoder setRenderPipelineState:pso];
    if (dss) [encoder setDepthStencilState:dss];
}

void MetalShader::unbind() {
    // No-op in Metal
}

void MetalShader::uploadUniforms() {
    // Use setVertexBytes/setFragmentBytes to copy uniforms inline per draw call.
    // This ensures each draw gets its own snapshot of the uniform data,
    // unlike a shared MTLBuffer which would only hold the last-written values.
    if (!this->m_encoder) return;
    id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)this->m_encoder;
    [encoder setVertexBytes:&this->m_uniforms length:sizeof(Uniforms) atIndex:1];
    [encoder setFragmentBytes:&this->m_uniforms length:sizeof(Uniforms) atIndex:0];
}

// --- Uniform setters (each writes to shared buffer immediately) ---

void MetalShader::setInt(const std::string& /*name*/, int /*value*/) {
}

void MetalShader::setFloat(const std::string& name, float value) {
    if (name == "u_material.shininess") this->m_uniforms.materialShininess = value;
    uploadUniforms();
}

void MetalShader::setVec2(const std::string& /*name*/, const Vec2& /*value*/) {
}

void MetalShader::setVec3(const std::string& name, const Vec3& value) {
    if (name == "u_lightPos") this->m_uniforms.lightPos = Vec4(value, 0.0f);
    else if (name == "u_viewPos") this->m_uniforms.viewPos = Vec4(value, 0.0f);
    uploadUniforms();
}

void MetalShader::setVec4(const std::string& name, const Vec4& value) {
    if (name == "u_lightColor") this->m_uniforms.lightColor = value;
    else if (name == "u_material.ambient") this->m_uniforms.materialAmbient = value;
    else if (name == "u_material.diffuse") this->m_uniforms.materialDiffuse = value;
    else if (name == "u_material.specular") this->m_uniforms.materialSpecular = value;
    else if (name == "u_material.emissive") this->m_uniforms.materialEmissive = value;
    uploadUniforms();
}

void MetalShader::setMat3(const std::string& name, const Mat3& value) {
    if (name == "u_normalMatrix") {
        // Pack glm::mat3 columns into float4 (Metal's float3x3 storage)
        this->m_uniforms.normalCol0 = Vec4(value[0], 0.0f);
        this->m_uniforms.normalCol1 = Vec4(value[1], 0.0f);
        this->m_uniforms.normalCol2 = Vec4(value[2], 0.0f);
    }
    uploadUniforms();
}

void MetalShader::setMat4(const std::string& name, const Mat4& value) {
    if (name == "u_viewProjection") this->m_uniforms.viewProjection = value;
    else if (name == "u_model") this->m_uniforms.model = value;
    uploadUniforms();
}

} // namespace GE::Graphics::Metal

#endif // __APPLE__

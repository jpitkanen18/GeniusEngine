#include "Factory.hpp"
#include "opengl/GLBuffer.hpp"
#include "opengl/GLShader.hpp"
#include "opengl/GLTexture.hpp"

#ifdef __APPLE__
#include "metal/MetalBuffer.hpp"
#include "metal/MetalShader.hpp"
#include "metal/MetalTexture.hpp"
#endif

#ifdef GE_HAS_VULKAN
#include "vulkan/VulkanBuffer.hpp"
#include "vulkan/VulkanShader.hpp"
#include "vulkan/VulkanTexture.hpp"
#endif

namespace GE::Graphics
{

    Backend Factory::s_backend = Backend::OpenGL;

    std::shared_ptr<IVertexBuffer> Factory::createVertexBuffer()
    {
        switch (s_backend)
        {
#ifdef __APPLE__
        case Backend::Metal:
            return std::make_shared<Metal::MetalVertexBuffer>();
#endif
#ifdef GE_HAS_VULKAN
        case Backend::Vulkan:
            return std::make_shared<Vulkan::VulkanVertexBuffer>();
#endif
        case Backend::OpenGL:
        default:
            return std::make_shared<GL::GLVertexBuffer>();
        }
    }

    std::shared_ptr<IIndexBuffer> Factory::createIndexBuffer()
    {
        switch (s_backend)
        {
#ifdef __APPLE__
        case Backend::Metal:
            return std::make_shared<Metal::MetalIndexBuffer>();
#endif
#ifdef GE_HAS_VULKAN
        case Backend::Vulkan:
            return std::make_shared<Vulkan::VulkanIndexBuffer>();
#endif
        case Backend::OpenGL:
        default:
            return std::make_shared<GL::GLIndexBuffer>();
        }
    }

    std::shared_ptr<IVertexArray> Factory::createVertexArray()
    {
        switch (s_backend)
        {
#ifdef __APPLE__
        case Backend::Metal:
            return std::make_shared<Metal::MetalVertexArray>();
#endif
#ifdef GE_HAS_VULKAN
        case Backend::Vulkan:
            return std::make_shared<Vulkan::VulkanVertexArray>();
#endif
        case Backend::OpenGL:
        default:
            return std::make_shared<GL::GLVertexArray>();
        }
    }

    std::shared_ptr<IShader> Factory::createShader()
    {
        switch (s_backend)
        {
#ifdef __APPLE__
        case Backend::Metal:
            return std::make_shared<Metal::MetalShader>();
#endif
#ifdef GE_HAS_VULKAN
        case Backend::Vulkan:
            return std::make_shared<Vulkan::VulkanShader>();
#endif
        case Backend::OpenGL:
        default:
            return std::make_shared<GL::GLShader>();
        }
    }

    std::shared_ptr<ITexture> Factory::createTexture()
    {
        switch (s_backend)
        {
#ifdef __APPLE__
        case Backend::Metal:
            return std::make_shared<Metal::MetalTexture>();
#endif
#ifdef GE_HAS_VULKAN
        case Backend::Vulkan:
            return std::make_shared<Vulkan::VulkanTexture>();
#endif
        case Backend::OpenGL:
        default:
            return std::make_shared<GL::GLTexture>();
        }
    }

} // namespace GE::Graphics

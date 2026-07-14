#pragma once
// Metal Context Implementation
// Uses GLFW for window management + Metal for rendering

#include "../GAPI.hpp"

#ifdef __APPLE__

// Forward declarations for Objective-C types
#ifdef __OBJC__
@protocol MTLDevice;
@protocol MTLCommandQueue;
@protocol MTLRenderCommandEncoder;
@protocol MTLCommandBuffer;
@class CAMetalLayer;
@class MTKView;
#else
typedef void *id;
#endif

struct GLFWwindow;

namespace GE::Graphics::Metal
{

	class MetalContext : public IContext
	{
	public:
		MetalContext() = default;
		~MetalContext() override { shutdown(); }

		bool init(int width, int height, const std::string &title) override;
		void shutdown() override;
		void beginFrame() override;
		void endFrame() override;
		bool shouldClose() override;
		void setClearColor(const Color &color) override;
		void setViewport(int x, int y, int width, int height) override;
		void setDepthTest(bool enabled) override;
		void setBlending(bool enabled) override;
		void *getNativeWindow() override { return this->m_window; }

		std::string getDeviceName() const override { return this->m_deviceName; }
		std::string getAPIVersion() const override { return "Metal 3"; }

		GLFWwindow *getGLFWWindow() { return this->m_window; }
		int getWidth() const { return this->m_width; }
		int getHeight() const { return this->m_height; }

		// Metal-specific accessors
		void *getDevice() { return this->m_device; }
		void *getCommandQueue() { return this->m_commandQueue; }
		void *getCurrentEncoder() { return this->m_currentEncoder; }
		void *getCurrentCommandBuffer() { return this->m_currentCommandBuffer; }

	private:
		GLFWwindow *m_window = nullptr;
		int m_width = 0;
		int m_height = 0;
		Color m_clearColor{0.02f, 0.02f, 0.05f, 1.0f};

		// Opaque Metal handles (actual ObjC objects behind void*)
		void *m_device = nullptr;
		void *m_commandQueue = nullptr;
		void *m_layer = nullptr;
		void *m_depthTexture = nullptr;
		void *m_currentDrawable = nullptr;
		void *m_currentCommandBuffer = nullptr;
		void *m_currentEncoder = nullptr;
		void *m_currentRenderPassDesc = nullptr;

		bool m_depthTestEnabled = true;
		bool m_blendingEnabled = true;

		void createDepthTexture();

		std::string m_deviceName;
	};

} // namespace GE::Graphics::Metal

#endif // __APPLE__

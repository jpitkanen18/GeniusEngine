#pragma once
// Metal backend global state (device, encoder pointers)
// Set by MetalContext, consumed by MetalShader/Buffer/Texture/UI

#ifdef __APPLE__

namespace GE::Graphics::Metal
{

	class MetalState
	{
	public:
		static void setDevice(void *device) { s_device = device; }
		static void *getDevice() { return s_device; }

		static void setEncoder(void *encoder) { s_encoder = encoder; }
		static void *getEncoder() { return s_encoder; }

		static void setCommandBuffer(void *buf) { s_commandBuffer = buf; }
		static void *getCommandBuffer() { return s_commandBuffer; }

		static void setRenderPassDescriptor(void *rpd) { s_rpd = rpd; }
		static void *getRenderPassDescriptor() { return s_rpd; }

	private:
		static inline void *s_device = nullptr;
		static inline void *s_encoder = nullptr;
		static inline void *s_commandBuffer = nullptr;
		static inline void *s_rpd = nullptr;
	};

} // namespace GE::Graphics::Metal

#endif

#pragma once
// GeniusEngine GAPI Interface
// Abstract base for all graphics API backends

#include "../core/Types.hpp"

namespace GE::Graphics
{

	// Enum for supported backends
	enum class Backend
	{
		OpenGL,
		Vulkan,
		Metal,
	};

	inline std::string backendName(Backend b)
	{
		switch (b)
		{
		case Backend::OpenGL:
			return "OpenGL";
		case Backend::Vulkan:
			return "Vulkan";
		case Backend::Metal:
			return "Metal";
		}
		return "Unknown";
	}

	// Abstract graphics context
	class IContext
	{
	public:
		virtual ~IContext() = default;
		virtual bool init(int width, int height, const std::string &title) = 0;
		virtual void shutdown() = 0;
		virtual void beginFrame() = 0;
		virtual void endFrame() = 0;
		virtual bool shouldClose() = 0;
		virtual void setClearColor(const Color &color) = 0;
		virtual void setViewport(int x, int y, int width, int height) = 0;
		virtual void setDepthTest(bool enabled) = 0;
		virtual void setBlending(bool enabled) = 0;
		virtual void *getNativeWindow() = 0;

		// Device info (populated after init)
		virtual std::string getDeviceName() const { return "Unknown"; }
		virtual std::string getDriverVersion() const { return ""; }
		virtual std::string getAPIVersion() const { return ""; }
	};

} // namespace GE::Graphics

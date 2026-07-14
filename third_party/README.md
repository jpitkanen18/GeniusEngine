# Third-Party Dependencies

GeniusEngine relies on the following third-party libraries:

## Bundled (in this directory)

### GLAD (OpenGL Loader)
- Source: https://glad.dav1d.de/
- Profile: Core, OpenGL 4.1
- Place generated files in: `glad/include/glad/glad.h`, `glad/include/KHR/khrplatform.h`, `glad/src/glad.c`

### Dear ImGui
- Source: https://github.com/ocornut/imgui
- Clone/copy into: `imgui/`
- Required files: imgui.cpp, imgui_draw.cpp, imgui_tables.cpp, imgui_widgets.cpp, imgui.h, imgui_internal.h
- Backends needed: backends/imgui_impl_glfw.cpp, backends/imgui_impl_glfw.h, backends/imgui_impl_opengl3.cpp, backends/imgui_impl_opengl3.h

## System (install via package manager)

### GLFW 3
- macOS: `brew install glfw`
- Linux: `apt install libglfw3-dev`
- Windows: Download from https://www.glfw.org/

### GLM
- macOS: `brew install glm`
- Linux: `apt install libglm-dev`
- Header-only math library

## Setup Script

Run `./setup_third_party.sh` to automatically download and set up GLAD and ImGui.

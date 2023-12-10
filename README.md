# Richelieu Renderer
A renderer built on top of the Vulkan Graphics API.

## References
- [VulkanSDK](https://vulkan.lunarg.com/)
- [GLFW](https://github.com/glfw/glfw)
- [glm](https://github.com/g-truc/glm/tree/master)
- [stb_image](https://github.com/nothings/stb)
- [tiny_obj_loader](https://github.com/tinyobjloader/tinyobjloader)
- [Dear ImGui](https://github.com/ocornut/imgui)

## Installation
### Project Cloning
- Use `git-lfs` to clone binary files correctly.
- Use `git submodule` to install the necessary third-party dependencies for the Vulkan renderer.

``` bash
# git clone https://github.com/Karm0tr1ne/Richelieu.git
# cd Richelieu/
git lfs pull
git submodule update --init --recursive
```
### Vulkan Installation
Please install Vulkan SDK from [Vulkan-SDK website](https://www.lunarg.com/vulkan-sdk/), and make sure Vulkan-related environment variables (`VULKAN_SDK`) are properly configured.

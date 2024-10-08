# vulkan_window_linux_port
Vulkan window written in C++ for Linux

![Vulkan Window](./screenshots/vulkan_window.webp)

## Dependencies
Install Vulkan dependencies
```
sudo apt install vulkan-tools vulkan-sdk libvulkan-dev vulkan-validationlayers-dev spirv-tools
```

Install GLFW dependencies
```
sudo apt install libglfw3-dev
```

Install GLM dependencies
```
sudo apt install libglm-dev
```

Install Xxf68vm and Xi libraries if not installed
```
sudo apt install libxxf86vm-dev libxi-dev
```

## Check the Vulkan Install Works
Run this command and if there is no output, then you are good. Check the command below if that does not work.
```
vulkaninfo --summary | grep "WARNING"
```

For NVIDIA, run this command. If there is not output, then you are good.
```
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/nvidia_icd.json vulkaninfo --summary | grep "WARNING"
```

For non NVIDIA cards, point to the json file for your GPU for the VK_ICD_FILENAMES environmental variable.

## Resources
My code comments are based on the information from the **Vulkan Tutorial** website.
Followed the instruction from the *Introduction* section and finish up to the *Drawing a triangle* section.
- https://vulkan-tutorial.com/
- [How can I fix my vulkan, why does my vulkaninfo fail?](https://askubuntu.com/questions/1484510/how-can-i-fix-my-vulkan-why-does-my-vulkaninfo-fail)

### Sections of the Vulkan Tutorial website I followed:
- [Introduction](https://vulkan-tutorial.com/Introduction)
- [Overview](https://vulkan-tutorial.com/Overview)
- [Development environment](https://vulkan-tutorial.com/Development_environment)
- Drawing a triangle
    - [Setup](https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code)
    - [Presentation](https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Window_surface)
    - [Graphics pipeline basics](https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Introduction)
    - [Drawing](https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Framebuffers)
    - [Swap chain recreation](https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation)

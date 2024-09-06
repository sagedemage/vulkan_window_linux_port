#!/bin/sh

# Run with nvidia_icd json file
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/nvidia_icd.json
./VulkanWindow

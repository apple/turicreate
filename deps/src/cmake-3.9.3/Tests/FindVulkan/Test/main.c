#include <vulkan/vulkan.h>

int main()
{
  VkInstanceCreateInfo instanceCreateInfo = {};
  instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

  VkApplicationInfo applicationInfo = {};
  applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  applicationInfo.apiVersion = VK_API_VERSION_1_0;
  applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  applicationInfo.pApplicationName = "CMake Test application";
  applicationInfo.pEngineName = "CMake Test Engine";

  instanceCreateInfo.pApplicationInfo = &applicationInfo;

  VkInstance instance = VK_NULL_HANDLE;
  vkCreateInstance(&instanceCreateInfo, NULL, &instance);

  // We can't assert here because in general vkCreateInstance will return an
  // error if no driver is found - but if we get here, FindVulkan is working

  if (instance != VK_NULL_HANDLE) {
    vkDestroyInstance(instance, NULL);
  }

  return 0;
}

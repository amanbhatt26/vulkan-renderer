#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <iostream>
#include <vector>
#include<optional>
#include<set>
#include<cstdint>
#include<limits>
#include<algorithm>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" }; 

#ifdef NDEBUG
bool enableValidationLayers = false;
#else
bool enableValidationLayers = true;
#endif

const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

struct SwapChainSupportDetails {
	
	VkSurfaceCapabilitiesKHR surfaceCapabilites;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};
 
struct QueueFamilyIndices {

	std::optional<uint32_t> graphicsFamilyIndex;
	std::optional<uint32_t> presentFamilyIndex;

	bool isComplete() {
		return graphicsFamilyIndex.has_value() && presentFamilyIndex.has_value();
	}

};

class MyApplication {

public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanUp();

	}
private:
	void initWindow() {

		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Tutorial", nullptr, nullptr);
	}

	void initVulkan() {

		
		createInstance();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		
	}

	void createInstance() {
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext = nullptr;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;
		
		uint32_t glfwExtensionsCount = 0;
		const char ** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

		uint32_t supportedExtensionsCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionsCount, nullptr);
		std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionsCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionsCount, supportedExtensions.data());
		std::printf("%d, extensions are supported \n", supportedExtensionsCount); 
		/*for (auto se : supportedExtensions) {
			std::printf("----> %s \n", se.extensionName);
		}*/

		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("Required Validation Layer is not supported.");
		}
		

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.pApplicationInfo = &appInfo;
		if (enableValidationLayers) {
			
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
			std::printf("Validation Layers Enabled.\n");
		}
		else {
			createInfo.enabledLayerCount = 0;
			createInfo.ppEnabledLayerNames = nullptr;
		}

		createInfo.enabledExtensionCount = glfwExtensionsCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;

		if (vkCreateInstance(
			&createInfo,
			nullptr,
			&instance) != VK_SUCCESS) {
			
			throw std::runtime_error("failed to create Instance!"); 
			
		}

		std::cout << "Instance Created" << std::endl;
		
	}

	void createSurface() {

		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("Cannot create window surface");
		}

	}

	void pickPhysicalDevice() {

		physicalDevice = VK_NULL_HANDLE; 

		uint32_t physicalDeviceCount;
		vkEnumeratePhysicalDevices(instance,&physicalDeviceCount, nullptr);
		std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount); 
		vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

		for (auto pd : physicalDevices) {
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(pd, &properties);
			std::printf("***** %s ", properties.deviceName);

			if (isDeviceSuitable(pd) && physicalDevice == VK_NULL_HANDLE) {

				physicalDevice = pd;
				std::printf("<<<<<< selected"); 
			}

			std::printf("\n");
		}

		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("No Suitable Physical Device Found");
		}

	}

	bool checkPhysicalDeviceExtensionsSupport(VkPhysicalDevice device) {
		
		uint32_t supportedExtensionsCount;

		vkEnumerateDeviceExtensionProperties(
			device,
			nullptr,
			&supportedExtensionsCount,
			nullptr);

		std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionsCount);
		vkEnumerateDeviceExtensionProperties(
			device,
			nullptr,
			&supportedExtensionsCount,
			supportedExtensions.data());

		for (auto de : deviceExtensions) {
			bool extensionFound = false;
			for (auto se : supportedExtensions) {
				if (strcmp(de, se.extensionName) == 0) {
					extensionFound = true;
				}
			}

			if (!extensionFound) return false;
		}

		return true;

	}

	void createLogicalDevice() {

		device = VK_NULL_HANDLE;


		float priorities[] = { 1.0f };

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		VkPhysicalDeviceFeatures enabledDeviceFeatures = {};
		

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos; 
		std::set<uint32_t> queueFamilyIndices = { indices.graphicsFamilyIndex.value(), indices.presentFamilyIndex.value() }; 

		for (auto qfIndex : queueFamilyIndices) {

			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.pNext = nullptr;
			queueCreateInfo.flags = 0;
			queueCreateInfo.queueFamilyIndex = qfIndex;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = priorities;

			queueCreateInfos.push_back(queueCreateInfo);
		}
		 

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pNext = nullptr;
		deviceCreateInfo.flags = 0;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		deviceCreateInfo.pEnabledFeatures = &enabledDeviceFeatures;


		if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("Cannot create Logical Device");
		}

		std::printf("Logical Device Created \n");

		graphicsQueue = VK_NULL_HANDLE;
		vkGetDeviceQueue(device, indices.graphicsFamilyIndex.value(), 0, &graphicsQueue);
		if (graphicsQueue == VK_NULL_HANDLE) {
			throw std::runtime_error("Cannot retrieve handle for Graphics Queue");
		}

		presentQueue = VK_NULL_HANDLE;
		vkGetDeviceQueue(device, indices.presentFamilyIndex.value(), 0, &presentQueue);
		if (presentQueue == VK_NULL_HANDLE) {
			throw std::runtime_error("Cannot retrieve handle for Graphics Queue");
		}


	}

	bool isDeviceSuitable(VkPhysicalDevice device) {

		QueueFamilyIndices indices = findQueueFamilies(device);
		bool extensionSupport = checkPhysicalDeviceExtensionsSupport(device);

		bool swapChainAdequate = false;
		if (extensionSupport) {

			SwapChainSupportDetails details = querySwapChainSupportDetails(device);
			swapChainAdequate = !details.presentModes.empty() && !details.formats.empty();

			
		}

		return indices.isComplete() && extensionSupport && swapChainAdequate;

	}

	SwapChainSupportDetails querySwapChainSupportDetails(VkPhysicalDevice device) {

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surfaceCapabilities);

		uint32_t formatsCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatsCount, nullptr);
		std::vector<VkSurfaceFormatKHR> formats(formatsCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatsCount, formats.data());

		uint32_t presentModesCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModesCount, nullptr);
		std::vector<VkPresentModeKHR> presentModes(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModesCount, presentModes.data());

		SwapChainSupportDetails details = {};
		details.surfaceCapabilites = surfaceCapabilities;
		details.formats = formats;
		details.presentModes = presentModes;

		return details;

	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		uint32_t i = 0;
		for (auto qf : queueFamilies) {
			if (qf.queueFlags & VK_QUEUE_GRAPHICS_BIT) {

				indices.graphicsFamilyIndex = i;

			}
			VkBool32 surfaceSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &surfaceSupport);

			if (surfaceSupport == VK_TRUE) {
				indices.presentFamilyIndex = i; 
			}

			if (indices.isComplete()) break;

			i++;
		}

		return indices;
	}

	bool checkValidationLayerSupport() {

		uint32_t supportedValidationLayersCount;
		vkEnumerateInstanceLayerProperties(&supportedValidationLayersCount, nullptr);

		std::vector<VkLayerProperties> supportedValidationLayers(supportedValidationLayersCount);
		vkEnumerateInstanceLayerProperties(&supportedValidationLayersCount, supportedValidationLayers.data());

		for (auto layer : validationLayers) {
			bool layerSupported = false;
			for (auto sLayer : supportedValidationLayers) {
				if (strcmp(layer, sLayer.layerName) == 0) {
					layerSupported = true;
					break;
				}
			}

			if (!layerSupported) {
				return false;
			}
		}

		return true;

	}

	void createSwapChain() {

		SwapChainSupportDetails supportDetails = querySwapChainSupportDetails(physicalDevice);

		VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
		swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainCreateInfo.surface = surface;
		
		swapChainCreateInfo.minImageCount = supportDetails.surfaceCapabilites.minImageCount + 1;
		if (
			supportDetails.surfaceCapabilites.maxImageCount > 0 &&
			swapChainCreateInfo.minImageCount > supportDetails.surfaceCapabilites.maxImageCount
			) {
			
			swapChainCreateInfo.minImageCount = supportDetails.surfaceCapabilites.maxImageCount;
		}

		swapChainCreateInfo.imageArrayLayers = 1;
		swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueIndices[] = { indices.graphicsFamilyIndex.value(), indices.presentFamilyIndex.value() };

		if (queueIndices[0] != queueIndices[1]) {
			swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapChainCreateInfo.queueFamilyIndexCount = 2;
			swapChainCreateInfo.pQueueFamilyIndices = queueIndices;
		}
		else {
			swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapChainCreateInfo.queueFamilyIndexCount = 0;
			swapChainCreateInfo.pQueueFamilyIndices = nullptr;

		}

		swapChainCreateInfo.preTransform = supportDetails.surfaceCapabilites.currentTransform;
		swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainCreateInfo.clipped = VK_TRUE;
		swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		swapChainCreateInfo.imageExtent = chooseSwapChainExtent(supportDetails.surfaceCapabilites);
		swapChainCreateInfo.presentMode = chooseSwapChainPresentMode(supportDetails.presentModes);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapChainFormat(supportDetails.formats);
		swapChainCreateInfo.imageFormat = surfaceFormat.format;
		swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;

		if (vkCreateSwapchainKHR(device, &swapChainCreateInfo, nullptr, &swapChain) != VK_SUCCESS) {

			throw std::runtime_error("Cannot create SwapChain");
		}

		std::printf("Swap Chain Created!\n");

		uint32_t imagesCount;
		vkGetSwapchainImagesKHR(device, swapChain, &imagesCount, nullptr);
		swapChainImages.resize(imagesCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imagesCount, swapChainImages.data());


		swapChainImageFormat = swapChainCreateInfo.imageFormat;
		swapChainImageExtent = swapChainCreateInfo.imageExtent;		
		
	}

	VkSurfaceFormatKHR chooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR> &formats) {
		for (const auto& availableFormat : formats) {
			if (
				availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
				&& availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
			) {
				return availableFormat;
			}
		}

		return formats[0];

	}

	VkExtent2D chooseSwapChainExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}

	}

	VkPresentModeKHR chooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& presentModes) {

		
		for (const auto& availablePresentMode : presentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
		

	}

	void createImageViews() {

		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImageViews.size(); i++) {

			VkImageViewCreateInfo imageViewCreateInfo = {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.image = swapChainImages[i];
			imageViewCreateInfo.format = swapChainImageFormat;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			VkComponentMapping components = {};
			components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components = components;
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("Cannot create image view");
			}
		}

	}


	void mainLoop() {

		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}

	}

	void cleanUp() {
		for (auto iv : swapChainImageViews) {
			vkDestroyImageView(device, iv, nullptr);
		}
		vkDestroySwapchainKHR(device, swapChain, nullptr);
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	GLFWwindow* window;
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainImageExtent;
};
int main() {

	MyApplication app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
   
}

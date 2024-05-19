#include "common-stuff.h"
#include "vulkan/vulkan_core.h"

#include <stdbool.h>
#include <string.h>

#include <windows.h>

const VkAllocationCallbacks* get_glob_vk_alloc(){
  return nullptr;
}

DEF_SLICE(VkLayerProperties);
DEF_SLICE(VkExtensionProperties);
typedef const char* Cstr;
DEF_SLICE(Cstr);
bool test_vulkan_layer_presence(struct VulkanLayer* to_test,
				const VkLayerPropertiesSlice avail_layers){
  to_test->available = false;
  for (int i = 0; i < avail_layers.count; ++i) {
    if (strcmp(to_test->layer_name, avail_layers.data[i].layerName) ==
	0) {
      to_test->available = true;
      break;
    }
  }
  return (to_test->available) || !(to_test->required);
}


int test_vulkan_extension_presence(
				   struct VulkanExtension* to_test,
				   const VkExtensionPropertiesSlice avail_extensions){
  to_test->available = false;
  for (int i = 0; i < avail_extensions.count; ++i) {
    if (strcmp(to_test->extension_name,
	       avail_extensions.data[i].extensionName) == 0) {
      to_test->available = true;
      break;
    }
  }
  return (to_test->available) || !(to_test->required);
}



/* int create_instance(StackAllocator* stk_allocr, */
/*                     size_t stk_allocr_offset, */
/*                     const VkAllocationCallbacks* alloc_callbacks, */
/*                     VkInstance* p_vk_instance, */
/*                     struct VulkanLayer* instance_layers, */
/*                     size_t layers_count, */
/*                     struct VulkanExtension* instance_extensions, */
/*                     size_t extensions_count) { */
OptInstance create_instance(AllocInterface allocr,
			    VulkanLayerSlice instance_layers,
			    VulkanExtensionSlice instance_extensions,
			    void* p_next_chain){
  VkResult result = VK_SUCCESS;
  OptInstance output = {0};
  VkApplicationInfo app_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "Application",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "Null Engine",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_3,
  };

  // Setup instance layers , + check for availability

  uint32_t avail_layer_count;
  vkEnumerateInstanceLayerProperties(&avail_layer_count, NULL);

  VkLayerPropertiesSlice available_layers =
    SLICE_ALLOC(VkLayerProperties, avail_layer_count, allocr);

  /* if (nullptr == available_layers.data) */
  /*     return CREATE_INSTANCE_LAYER_CHECK_ALLOC_ERROR; */

  jmp_on_true_expr(nullptr == available_layers.data,
	      layer_alloc_error,
	      output.code = CREATE_INSTANCE_LAYER_CHECK_ALLOC_ERROR;);

  vkEnumerateInstanceLayerProperties(&avail_layer_count,
				     available_layers.data);
  bool found = true;
  //size_t layers_count = instance_layers.count;
  //for (int i = 0; i < instance_layers.count; ++i) {
  for_slice(instance_layers, i){
    found = found &&
      test_vulkan_layer_presence(instance_layers.data + i,
				 available_layers);
    if (!instance_layers.data[i].available) {
      struct VulkanLayer layer = instance_layers.data[i];
      instance_layers.data[i--] = instance_layers.data[instance_layers.count - 1];
      instance_layers.data[--instance_layers.count] = layer;
    }
  }
  //Free the available layers okay

  jmp_on_true_expr(!found, layer_not_found,
	      output.code = CREATE_INSTANCE_LAYER_NOT_FOUND;);
  /* if (!found){ */
  /*   SLICE_FREE(available_layers, allocr); */
  /*   return  */
  /* } */

  //const char** layers_list = available_layers.data;
  CstrSlice layers_list = init_Cstr_slice((const char**)available_layers.data,
					  instance_layers.count);
  for_slice(instance_layers, i){
    //for (int i = 0; i < instance_layers.count; ++i)
    layers_list.data[i] = instance_layers.data[i].layer_name;
  }
  // Setup instance extensions {opt, check for availability}

  uint32_t avail_extension_count;
  vkEnumerateInstanceExtensionProperties(
					 NULL, &avail_extension_count, NULL);

  VkExtensionPropertiesSlice available_extensions =
    SLICE_ALLOC(VkExtensionProperties, avail_extension_count, allocr);
  jmp_on_true_expr(nullptr == available_extensions.data,
	      extension_alloc_error,
	      output.code =  CREATE_INSTANCE_EXTENSION_CHECK_ALLOC_ERROR;);

  vkEnumerateInstanceExtensionProperties(
					 NULL, &avail_extension_count, available_extensions.data);
  found = true;
  //for (int i = 0; i < extensions_count; ++i) {
  for_slice(instance_extensions, i){
    found = found &&
      test_vulkan_extension_presence(instance_extensions.data + i,
				     available_extensions);
    if (!instance_extensions.data[i].available) {
      struct VulkanExtension extension = instance_extensions.data[i];
      instance_extensions.data[i--] =
	instance_extensions.data[instance_extensions.count - 1];
      instance_extensions.data[--instance_extensions.count] = extension;
    }
  }
  // free(available_extensions);


  jmp_on_true_expr(!found, extension_not_found,
	      output.code = CREATE_INSTANCE_EXTENSION_NOT_FOUND;);
  CstrSlice extension_list = init_Cstr_slice((Cstr*)available_extensions.data,
					     instance_extensions.count);
  //const char** extension_list = available_extensions;
  //for (int i = 0; i < extensions_count; ++i)
  for_slice(extension_list, i)
    extension_list.data[i] = instance_extensions.data[i].extension_name;

  // VkInstanceCreateInfo create_info = {
  //	.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
  //	.pApplicationInfo = &app_info,
  // };

  
  VkInstanceCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &app_info,
    .enabledExtensionCount = extension_list.count,
    .ppEnabledExtensionNames = extension_list.data,
    .enabledLayerCount = layers_list.count,
    .ppEnabledLayerNames = layers_list.data,
    .pNext = p_next_chain,
  };

  result = vkCreateInstance(&create_info, get_glob_vk_alloc(), &output.value);

  if (result != VK_SUCCESS){
    output.code = CREATE_INSTANCE_FAILED;
  }

 extension_not_found:
  SLICE_FREE(available_extensions, allocr);
 extension_alloc_error:
 layer_not_found:
  SLICE_FREE(available_layers, allocr);
 layer_alloc_error:
  return output;
}

/* void clear_instance(const VkAllocationCallbacks* alloc_callbacks, */
/*                     VkInstance* p_vk_instance, int err_codes) { */
OptInstance clear_instance(OptInstance vk_instance){

  switch (vk_instance.code) {
  case CREATE_INSTANCE_OK:
    vkDestroyInstance(vk_instance.value, get_glob_vk_alloc());
  case CREATE_INSTANCE_FAILED:
  case CREATE_INSTANCE_EXTENSION_NOT_FOUND:
  case CREATE_INSTANCE_EXTENSION_CHECK_ALLOC_ERROR:
  case CREATE_INSTANCE_LAYER_NOT_FOUND:
  case CREATE_INSTANCE_LAYER_CHECK_ALLOC_ERROR:
  case CREATE_INSTANCE_TOP_FAIL_CODE:
    vk_instance = (OptInstance){0};
  }
  return vk_instance;
}

DEF_SLICE(VkSurfaceFormatKHR);
DEF_SLICE(VkPresentModeKHR);

/* int choose_swapchain_details(StackAllocator* stk_allocr, */
/*                              size_t stk_offset, */
/*                              ChooseSwapchainDetsParam param) { */
OptSwapchainDetails choose_swapchain_details(AllocInterface allocr,
					     ChooseSwapchainDetsParam param){
  OptSwapchainDetails output = {0};
  // Query for the swap_support of swapchain and window surface
  VkSurfaceCapabilitiesKHR capabilities;
  jmp_on_true_expr((vkGetPhysicalDeviceSurfaceCapabilitiesKHR(param.phy_device,
							 param.surface,
							 &capabilities) !=
	       VK_SUCCESS),
	      exit,
	      output.code = CHOOSE_SWAPCHAIN_DETAILS_CAPABILITY_GET_FAIL;);
  //return CHOOSE_SWAPCHAIN_DETAILS_CAPABILITY_GET_FAIL;

  uint32_t formats_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(
				       param.phy_device, param.surface, &formats_count, NULL);
  jmp_on_true_expr(0 == formats_count,
	      exit,
	      output.code = CHOOSE_SWAPCHAIN_DETAILS_NO_SURFACE_FORMAT;);

  uint32_t present_modes_count = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
					    param.phy_device, param.surface, &present_modes_count, NULL);

  jmp_on_true_expr(0 == present_modes_count,
	      exit,
	      output.code= CHOOSE_SWAPCHAIN_DETAILS_NO_PRESENT_MODES;);


  if (param.return_format_n_count) {
    output.value.img_format.format = VK_FORMAT_UNDEFINED;

    VkSurfaceFormatKHRSlice surface_formats =
      SLICE_ALLOC(VkSurfaceFormatKHR, formats_count, allocr);

    if (surface_formats.data != nullptr){
      vkGetPhysicalDeviceSurfaceFormatsKHR(param.phy_device, param.surface,
					   &formats_count,
					   surface_formats.data);
      // Choose the settings
      // Choose best if availabe else the first format
      output.value.img_format = surface_formats.data[0];
      for_slice(surface_formats, i){
	//for (uint32_t i = 0; i < formats_count; ++i) {
	if (surface_formats.data[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
	    surface_formats.data[i].colorSpace ==
	    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
	  output.value.img_format = surface_formats.data[i];
	  break;
	}
      }
      SLICE_FREE(surface_formats, allocr);
    }
    else{
      output.code = CHOOSE_SWAPCHAIN_DETAILS_INT_ALLOC_ERR;
    }
    // Choose a img count
    output.value.min_img_count = capabilities.minImageCount + 3;
    if (capabilities.maxImageCount != 0)
      output.value.min_img_count = 
	_min(output.value.min_img_count, capabilities.maxImageCount);
  }



  if (param.return_present_mode) {

    VkPresentModeKHRSlice present_modes =
      SLICE_ALLOC(VkPresentModeKHR, present_modes_count, allocr);

    if(present_modes.data != nullptr){
      vkGetPhysicalDeviceSurfacePresentModesKHR(param.phy_device, param.surface,
						&present_modes_count,
						present_modes.data);
      // Choose present mode
      output.value.present_mode = VK_PRESENT_MODE_FIFO_KHR;
      //for (uint32_t i = 0; i < present_modes_count; ++i) {
      for_slice(present_modes, i){
	//if(present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
	if (present_modes.data[i] == VK_PRESENT_MODE_MAILBOX_KHR)
	  output.value.present_mode = present_modes.data[i];
      }
      SLICE_FREE(present_modes, allocr);
    }
    else{
      output.code = CHOOSE_SWAPCHAIN_DETAILS_INT_ALLOC_ERR;
    }
  }

  if(param.return_transform)
    output.value.transform_flags = capabilities.currentTransform;

  if(param.return_extent)
    output.value.surface_extent = capabilities.currentExtent;

 exit:
  return output;
  //return CHOOSE_SWAPCHAIN_DETAILS_OK;
}
DEF_SLICE(VkQueueFamilyProperties);
//Helper function for choosing physical device
struct PhyDeviceTest{
  bool accepted;
  u32 graphics_family;
  u32 present_family;
  u32 compute_family;
  VkFormat depth_stencil_format;
  VkSurfaceFormatKHR img_format;
  uint32_t min_img_count;
};
static struct PhyDeviceTest
test_physical_device(AllocInterface allocr,
		     VkPhysicalDevice phy_dev,
		     VkSurfaceKHR surface,
		     VulkanLayerSlice dev_layers,
		     VulkanExtensionSlice dev_extensions){

  struct PhyDeviceTest output = {.accepted = true};

    //Initialize to 0 here, so later can safely free it
    VkLayerPropertiesSlice available_layers = {0};
  VkExtensionPropertiesSlice available_extensions = {0};

  bool found;
  // test if suitable
  if(dev_layers.count > 0){

    // Check if device layers are supported
    uint32_t avail_layers_count;
    vkEnumerateDeviceLayerProperties(phy_dev,
				     &avail_layers_count, NULL);

    available_layers = SLICE_ALLOC(VkLayerProperties, avail_layers_count, allocr);
    /* VkLayerProperties* available_layers = stack_allocate( */
    /*     stk_allocr, &local_stk, */
    /*     (avail_layers_count * sizeof *available_layers), 1); */
    jmp_on_true_expr(nullptr == available_layers.data,
		skip_layers,
		output.accepted = false);

    vkEnumerateDeviceLayerProperties(phy_dev, &avail_layers_count,
				     available_layers.data);
    found = true;
    //for (int i = 0; i < layers_count; ++i) {
    for_slice(dev_layers, i){
      found = found &&
	test_vulkan_layer_presence(dev_layers.data + i,
				   available_layers);
    }
    if (!found)
      output.accepted = false;
  }
 skip_layers:
  SLICE_FREE(available_layers, allocr);
  
  // Check if device extensions are supported
  if(dev_extensions.count > 0){
    uint32_t avail_extension_count;
    vkEnumerateDeviceExtensionProperties(phy_dev, NULL,
					 &avail_extension_count, NULL);

    /* VkExtensionProperties* available_extensions = stack_allocate( */
    /* 								  stk_allocr, &local_stk, */
    /* 								  (avail_extension_count * sizeof *available_extensions), */
    /* 								  1); */

    available_extensions = SLICE_ALLOC(VkExtensionProperties,
				       avail_extension_count, allocr);
    jmp_on_true_expr(nullptr == available_extensions.data,
		skip_extensions,
		output.accepted = false);

    vkEnumerateDeviceExtensionProperties(phy_dev, NULL,
					 &avail_extension_count,
					 available_extensions.data);

    found = true;
    //for (int i = 0; i < extensions_count; ++i) {
    for_slice(dev_extensions, i){
      found = found &&
	test_vulkan_extension_presence(dev_extensions.data + i,
				       available_extensions);
    }
  }
 skip_extensions:
  SLICE_FREE(available_extensions, allocr);

   
   
  // Check device properties and features
  VkPhysicalDeviceProperties device_props;
  vkGetPhysicalDeviceProperties(phy_dev, &device_props);

  VkPhysicalDeviceFeatures device_feats;
  vkGetPhysicalDeviceFeatures(phy_dev, &device_feats);

  // Here test for non queue requirements
  // if (!device_feats.geometryShader)
  //	continue;

  // Check for availability of required queue family / rate here

  uint32_t family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(phy_dev,
					   &family_count, NULL);
   
  VkQueueFamilyPropertiesSlice families =
    SLICE_ALLOC(VkQueueFamilyProperties, family_count, allocr);
  /* VkQueueFamilyProperties* families = */
  /*     stack_allocate(stk_allocr, &local_stk, */
  /*                    (family_count * sizeof *families), 1); */
  bool graphics_avail = false;
  bool present_avail = false;
  bool compute_avail = false;

  if(nullptr != families.data){
    vkGetPhysicalDeviceQueueFamilyProperties(phy_dev, &family_count, families.data);
    // One by one check for suitability

    //for (int jj = 0; jj < family_count; ++jj) {
    for_slice(families, jj){
      int j = family_count - jj - 1;
      //j = jj;

      VkBool32 present_capable;
      vkGetPhysicalDeviceSurfaceSupportKHR(phy_dev, j,
					   surface,
					   &present_capable);
      if ((families.data[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
	  !graphics_avail) {
	output.graphics_family = j;
	graphics_avail = true;
      }
      if ((families.data[j].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
	  !compute_avail) {
	output.compute_family = j;
	compute_avail = true;
      }
      if (present_capable && !present_avail) {
	output.present_family = j;
	present_avail = true;
      }

      // break if all is available
      if (graphics_avail && present_avail && compute_avail)
	break;
    }
  }
    
  SLICE_FREE(families, allocr);
 
  // Check if all is filled
  if (!graphics_avail || !present_avail || !compute_avail){
    //continue;
    output.accepted = false;
  }

  // Choose swapchain details

  OptSwapchainDetails opt_swap =
    choose_swapchain_details(allocr,
			     (ChooseSwapchainDetsParam){
			       .phy_device = phy_dev,
			       .surface = surface,
			       .return_format_n_count = true
			     });
	
  /* if (choose_swapchain_details(stk_allocr, local_stk, */
  /*                              choose_swap) < */
  /*     CHOOSE_SWAPCHAIN_DETAILS_OK) */
  /*   continue; */
  if(opt_swap.code < 0){
    output.accepted = false;
  }
  else{
    output.img_format = opt_swap.value.img_format;
    output.min_img_count = opt_swap.value.min_img_count;
  }

  output.depth_stencil_format =
    choose_image_format
    (phy_dev,
     SLICE_FROM_ARRAY
     (VkFormat,
      ((VkFormat[]){VK_FORMAT_D32_SFLOAT,
		   VK_FORMAT_D32_SFLOAT_S8_UINT,
		   VK_FORMAT_D24_UNORM_S8_UINT })),
     VK_IMAGE_TILING_OPTIMAL,
     VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

  if(output.depth_stencil_format == VK_FORMAT_UNDEFINED)
    output.accepted = false;

  return output;
}

/* int create_device(StackAllocator* stk_allocr, */
/*                   size_t stk_allocr_offset, */
/*                   VkAllocationCallbacks* alloc_callbacks, */
/*                   CreateDeviceParam param, */
/*                   struct VulkanLayer* device_layers, */
/*                   size_t layers_count, */
/*                   struct VulkanExtension* device_extensions, */
/*                   size_t extensions_count) { */
DEF_SLICE(VkPhysicalDevice);
OptVulkanDevice create_device(const AllocInterface allocr,
			      CreateDeviceParam param,
			      VulkanLayerSlice device_layers,
			      VulkanExtensionSlice device_extensions,
			      void* p_next_chain){

  VkResult result = VK_SUCCESS;
  OptVulkanDevice output = {0};
  //param.p_vk_device->phy_device = VK_NULL_HANDLE;
  output.value.phy_device = VK_NULL_HANDLE;
    
  uint32_t phy_device_count = 0;
  vkEnumeratePhysicalDevices(param.vk_instance, &phy_device_count,
			     NULL);

  if (phy_device_count == 0){
    output.code = CREATE_DEVICE_NO_GPU;
    return output;
  }

  VkPhysicalDeviceSlice phy_devices =
    SLICE_ALLOC(VkPhysicalDevice, phy_device_count, allocr);
  /* VkPhysicalDevice* phy_devices = */
  /*     stack_allocate(stk_allocr, &stk_allocr_offset, */
  /*                    (phy_device_count * sizeof *phy_devices), 1); */

    
  if (nullptr == phy_devices.data){
    output.code = CREATE_DEVICE_PHY_DEVICE_TEST_ALLOC_FAIL;
    return output;
  }
	
  vkEnumeratePhysicalDevices(param.vk_instance, &phy_device_count,
			     phy_devices.data);


  //for (int i = 0; i < phy_device_count; ++i) {
  for_slice(phy_devices, i){

    struct PhyDeviceTest test_results =
      test_physical_device(allocr, phy_devices.data[i],
			   param.chosen_surface,
			   device_layers,
			   device_extensions);

    if(test_results.accepted){
      output.value.phy_device = phy_devices.data[i];
      output.value.graphics.family = test_results.graphics_family;
      output.value.present.family = test_results.present_family;
      output.value.compute.family = test_results.compute_family;
      if(param.p_depth_stencil_format)
	*param.p_depth_stencil_format = test_results.depth_stencil_format;
      if(param.p_img_format)
	*param.p_img_format = test_results.img_format;
      if(param.p_min_img_count)
	*param.p_min_img_count = test_results.min_img_count;
      break;
    }
  }
  SLICE_FREE(phy_devices, allocr);

  if (output.value.phy_device == VK_NULL_HANDLE){
    output.code = CREATE_DEVICE_NO_SUITABLE_GPU;
    return output;
  }

  // Rearrange the layers and extensions
  //for (int i = 0; i < layers_count; ++i) {
  for_slice(device_layers, i){
    if (!device_layers.data[i].available) {
      struct VulkanLayer layer = device_layers.data[i];
      device_layers.data[i--] = device_layers.data[device_layers.count - 1];
      device_layers.data[--device_layers.count] = layer;
    }
  }

  CstrSlice layers_list = SLICE_ALLOC(Cstr, device_layers.count, allocr);
  if(nullptr == layers_list.data){
    output.code = CREATE_DEVICE_PHY_DEVICE_INT_ALLOC_FAIL;
    //return output;
  }
  else{

    
    /* const char** layers_list = */
    /*   stack_allocate(stk_allocr, &stk_allocr_offset, */
    /* 		     layers_count * sizeof(const char*), 1); */
    //for (int i = 0; i < layers_count; ++i)
    for_slice(layers_list, i)
      layers_list.data[i] = device_layers.data[i].layer_name;
  }
  //Now for extensions
  //for (int i = 0; i < extensions_count; ++i) {
  for_slice(device_extensions, i){
    if (!device_extensions.data[i].available) {
      struct VulkanExtension extension = device_extensions.data[i];
      device_extensions.data[i--] =
	device_extensions.data[device_extensions.count - 1];
      device_extensions.data[--device_extensions.count] = extension;
    }
  }
  CstrSlice extensions_list = SLICE_ALLOC(Cstr, device_extensions.count, allocr);
  if(nullptr == extensions_list.data){
    output.code = CREATE_DEVICE_PHY_DEVICE_INT_ALLOC_FAIL;
  }
  else{
    /* const char** extensions_list = */
    /*     stack_allocate(stk_allocr, &stk_allocr_offset, */
    /*                    extensions_count * sizeof(const char*), 1); */
    //for (int i = 0; i < extensions_count; ++i)
    for_slice(extensions_list, i){
      extensions_list.data[i] = device_extensions.data[i].extension_name;
    }
  }

  float queue_priorities = 1.f;
  VkDeviceQueueCreateInfo queue_create_infos[3];
  u32 queue_create_count = 0;
  queue_create_infos[queue_create_count] = (VkDeviceQueueCreateInfo){
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueCount = 1,
    .queueFamilyIndex = output.value.graphics.family,
    .pQueuePriorities = &queue_priorities
  };
  queue_create_count++;
  // Make a nested if for this case, it's handleable anyways
  if (output.value.present.family != output.value.graphics.family) {
    queue_create_infos[queue_create_count] = queue_create_infos[0];
    queue_create_infos[queue_create_count].queueFamilyIndex =
      output.value.present.family;
    queue_create_count++;
  }
  if((output.value.compute.family != output.value.present.family) &&
     (output.value.compute.family != output.value.graphics.family)){
    queue_create_infos[queue_create_count] = queue_create_infos[0];
    queue_create_infos[queue_create_count].queueFamilyIndex =
      output.value.compute.family;
    queue_create_count++;
  }

  // When required, create the device features, layers check and
  // enable code here

  VkDeviceCreateInfo dev_create_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pQueueCreateInfos = queue_create_infos,
    .queueCreateInfoCount = queue_create_count,
    .enabledExtensionCount = extensions_list.count,
    .ppEnabledExtensionNames = extensions_list.data,
    .enabledLayerCount = layers_list.count,
    .ppEnabledLayerNames = layers_list.data,
    .pNext = p_next_chain
  };

  result =
    vkCreateDevice(output.value.phy_device,
		   &dev_create_info,
		   get_glob_vk_alloc(),
		   &output.value.device);
  if (result != VK_SUCCESS){
    output.code = CREATE_DEVICE_FAILED;
  }
  else{

    OptVulkanQueue graphics = create_vulkan_queue(output.value.device,
						  output.value.graphics.family,
						  0);
    OptVulkanQueue compute = create_vulkan_queue(output.value.device,
						 output.value.compute.family,
						 0);
    OptVulkanQueue present = create_vulkan_queue(output.value.device,
						 output.value.present.family,
						 0);
    if((graphics.code < 0) || (compute.code < 0) || (present.code < 0)){
      output.code = CREATE_DEVICE_QUEUE_ITEM_FAIL;
      compute = clear_vulkan_queue(compute, output.value.device);
      graphics = clear_vulkan_queue(graphics, output.value.device);
      present = clear_vulkan_queue(present, output.value.device);
    }
    output.value.compute = compute.value;
    output.value.graphics = graphics.value;
    output.value.present = present.value;
  }

  SLICE_FREE(extensions_list, allocr);
  SLICE_FREE(layers_list, allocr);
  return output;
}

OptVulkanDevice clear_device(OptVulkanDevice vk_device){
  
  switch (vk_device.code) {
  case CREATE_DEVICE_OK:
    (void)clear_vulkan_queue((OptVulkanQueue){.value = vk_device.value.compute},
			     vk_device.value.device);
    
    (void)clear_vulkan_queue((OptVulkanQueue){.value = vk_device.value.graphics},
			     vk_device.value.device);
    
    (void)clear_vulkan_queue((OptVulkanQueue){.value = vk_device.value.present},
			     vk_device.value.device);
  case CREATE_DEVICE_QUEUE_ITEM_FAIL:    
    vkDestroyDevice(vk_device.value.device, get_glob_vk_alloc());
  case CREATE_DEVICE_FAILED:
  case CREATE_DEVICE_PHY_DEVICE_INT_ALLOC_FAIL:
  case CREATE_DEVICE_NO_SUITABLE_GPU:
  case CREATE_DEVICE_PHY_DEVICE_TEST_ALLOC_FAIL:
  case CREATE_DEVICE_NO_GPU:

  case CREATE_DEVICE_TOP_FAIL_CODE:
    vk_device = (OptVulkanDevice){0};
  }
  return vk_device;
}


OptVulkanQueue create_vulkan_queue(VkDevice device, u32 family_inx, u32 queue_inx){
  OptVulkanQueue queue = {0};

  vkGetDeviceQueue(device, family_inx, queue_inx, &queue.value.vk_obj);
  if(VK_NULL_HANDLE == queue.value.vk_obj){
    queue.code = CREATE_VULKAN_QUEUE_GET_FAIL;
    return queue;
  }

  queue.value.family = family_inx;
  OptCommandPool cmd_pool = create_command_pool(device, family_inx);
  if(cmd_pool.code == CREATE_COMMAND_POOL_FAILED){
    queue.code = CREATE_VULKAN_QUEUE_CMD_POOL_FAIL;
    return queue;
  }
  assert(cmd_pool.code == CREATE_COMMAND_POOL_OK);
  queue.value.im_pool = cmd_pool.value;
  VkResult res = VK_SUCCESS;

  res = vkAllocateCommandBuffers(device, &(VkCommandBufferAllocateInfo){
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
      .commandPool = cmd_pool.value,
    }, &queue.value.im_buffer);
  
  if(VK_SUCCESS != res){
    queue.code = CREATE_VULKAN_QUEUE_CMD_BUFFER_FAIL;
    return queue;
  }
  res = vkCreateFence(device, &(VkFenceCreateInfo){
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    },
    get_glob_vk_alloc(), &queue.value.im_fence);

  if(VK_SUCCESS != res){
    queue.code = CREATE_VULKAN_QUEUE_FENCE_CREATE_FAIL;
  }
  return queue;
}

OptVulkanQueue clear_vulkan_queue(OptVulkanQueue queue, VkDevice device){
  switch(queue.code){
  case CREATE_VULKAN_QUEUE_OK:
    
    vkDestroyFence(device, queue.value.im_fence, get_glob_vk_alloc());
  case CREATE_VULKAN_QUEUE_FENCE_CREATE_FAIL:
  case CREATE_VULKAN_QUEUE_CMD_BUFFER_FAIL:

    clear_command_pool((OptCommandPool){.value = queue.value.im_pool}, device);
  case CREATE_VULKAN_QUEUE_CMD_POOL_FAIL:
  case CREATE_VULKAN_QUEUE_GET_FAIL:
    
  case CREATE_VULKAN_QUEUE_TOP_FAIL_CODE:
    queue = (OptVulkanQueue){.code = CREATE_VULKAN_QUEUE_TOP_FAIL_CODE};
  }
  return queue;
}

/* int create_command_pool(StackAllocator* stk_allocr, size_t stk_offset, */
/*                         const VkAllocationCallbacks* alloc_callbacks, */
/*                         CreateCommandPoolParam param) { */
OptCommandPool create_command_pool(VkDevice device,
				   uint32_t queue_family_inx){
  VkResult result = VK_SUCCESS;
  OptCommandPool output = {0};
  output.code = CREATE_COMMAND_POOL_OK;

  VkCommandPoolCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = queue_family_inx,
  };
  result = vkCreateCommandPool(device, &create_info,
			       get_glob_vk_alloc(), &output.value);

  if (result != VK_SUCCESS)
    output.code = CREATE_COMMAND_POOL_FAILED;

  return output;
}

/* void clear_command_pool(const VkAllocationCallbacks* alloc_callbacks, */
/*                         ClearCommandPoolParam param, int err_code) { */
OptCommandPool clear_command_pool(OptCommandPool cmd_pool, VkDevice device){
  switch (cmd_pool.code) {
  case CREATE_COMMAND_POOL_OK:
    vkDestroyCommandPool(device, cmd_pool.value,
			 get_glob_vk_alloc());
  case CREATE_COMMAND_POOL_FAILED:
    cmd_pool = (OptCommandPool){0};
  }
  return cmd_pool;
}


bool immediate_command_begin(VkDevice device, VulkanQueue queue){
  VkCommandBufferBeginInfo cmd_begin_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  VkResult res = vkBeginCommandBuffer(queue.im_buffer,
				      &cmd_begin_info);		

  return res == VK_SUCCESS;
}
bool immediate_command_end(VkDevice device, VulkanQueue queue){
  VkResult res = VK_SUCCESS;  
  //End recordig
  res = vkEndCommandBuffer(queue.im_buffer);
  if(res != VK_SUCCESS)
    return false;
  //Reset fence
  res = vkResetFences(device, 1, &queue.im_fence);
  if(res != VK_SUCCESS)
    return false;
  //Submit to queue
  res = vkQueueSubmit2(queue.vk_obj, 1, &(VkSubmitInfo2){
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
      .commandBufferInfoCount = 1,
      .pCommandBufferInfos = &(VkCommandBufferSubmitInfo){
	.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
	.commandBuffer = queue.im_buffer,
      },
    }, queue.im_fence);
  if(res != VK_SUCCESS)
    return false;
  //Wait for fence

  res = vkWaitForFences(device, 1, &queue.im_fence, VK_TRUE, UINT64_MAX);

  return res == VK_SUCCESS;
}

VkFormat choose_image_format(VkPhysicalDevice phy_device,
			     VkFormatSlice format_candidates,
                             VkImageTiling img_tiling, VkFormatFeatureFlags format_flags){

/* VkFormat choose_image_format(VkPhysicalDevice phy_device, */
/*                              VkFormat* format_candidates, */
/*                              size_t count, VkImageTiling img_tiling, */
/*                              VkFormatFeatureFlags format_flags) { */
  //for (size_t i = 0; i < count; ++i) {
  for_slice(format_candidates, i){
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(phy_device, format_candidates.data[i], &properties);
    if (img_tiling == VK_IMAGE_TILING_LINEAR &&
	((properties.linearTilingFeatures & format_flags) ==
	 format_flags))
      return format_candidates.data[i];
    if (img_tiling == VK_IMAGE_TILING_OPTIMAL &&
	((properties.optimalTilingFeatures & format_flags) ==
	 format_flags))
      return format_candidates.data[i];
  }
  return VK_FORMAT_UNDEFINED;
}

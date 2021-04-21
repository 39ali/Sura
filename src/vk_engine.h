
#pragma once
#pragma warning(disable : 26812)
#include "VkBootstrap.h"
#include "vk_deletionQueue.h"
#include "vk_initializers.h"
#include "vk_renderable.h"
#include <unordered_map>
#include <vk_mesh.h>
#include <vk_types.h>

constexpr unsigned int FRAME_OVERLAP = 2;


struct GPUCameraData {
    glm::mat4 view; 
    glm::mat4 projection;
    glm::mat4 viewProj; 
} ;

struct FrameData {

  VkSemaphore present_semaphore;
  VkSemaphore render_semaphore;
  VkFence render_fence;

  VkCommandPool command_pool;
  VkCommandBuffer command_buffer;

  //cameraData 
  AllocatedBuffer  cameraData;  
  VkDescriptorSet descriptorSet;

  AllocatedBuffer objectBuffer; 
  VkDescriptorSet objectDescriptor;
};

struct GPUObjectData {
    glm::mat4 modelMatrix; 
};


struct GPUSceneData {
    glm::vec4 fogColor; 
    glm::vec4 fogDistances;
    glm::vec4 ambientColor; 
    glm::vec4 sunlightDirection; 
    glm::vec4 sunlightColor; 
}
;

class VulkanEngine {
public:
  struct SDL_Window *m_window{nullptr};

// initializes everything in the engine
  void init();

  // shuts down the engine
  void cleanup();

  // draw loop
  void draw();

  // run main loop
  void run();

private:
  void init_vulkan();
  void init_allocator();
  void init_depth_buffer();
  void init_swapchain();
  void init_commands();
  void init_render_pass();
  void init_framebuffers();
  void init_sync_structs();
  bool load_shader_module(const char *file_path, VkShaderModule *out);
  void init_pipelines();

  // meshes
  void load_meshes();
  void upload_mesh(Mesh &mesh);
  void init_triangle_mesh_pipeline(vkinit::PipelineBuilder &builder);
  void init_scene();
  void draw_objects(VkCommandBuffer cmd, RenderObject *first, size_t count);


  size_t alignUniformBufferSize(size_t originalSize);

  Material *create_material(VkPipeline pipeline, VkPipelineLayout layout,
                            const std::string &name);
  Material *get_material(const std::string &name);
  Mesh *get_mesh(const std::string &name);
  void draw_objects(VkCommandBuffer cmd, RenderObject *first, int count);

  FrameData &get_current_frame();


  AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage); 
  void init_descriptors(); 


private:
  bool m_isInitialized{false};
  int m_frameNumber{0};

  GPUSceneData m_sceneData; 
  AllocatedBuffer m_sceneDataBuffer; 

  VkExtent2D m_windowExtent{1290, 720};
  VkInstance m_instance;
  VkDebugUtilsMessengerEXT m_debug_messenger;
  VkPhysicalDevice m_physical_device;
  VkDevice m_device;
  VkSurfaceKHR m_surface;

  // swapchain
  VkSwapchainKHR m_swapchain;
  VkFormat m_swapchain_image_format;
  std::vector<VkImage> m_swapchain_images;
  std::vector<VkImageView> m_swapchain_image_views;

  // depth
  VkFormat m_depth_format;
  VkImageView m_depth_image_view;
  AllocatedImage m_depth_image;

  // queues
  VkQueue m_graphics_queue;
  uint32_t m_graphics_queue_family;

  // renderpass
  VkRenderPass m_render_pass;
  std::vector<VkFramebuffer> m_framebuffers;

  // frameData
  FrameData m_frames[FRAME_OVERLAP];

  // pipelines
  VkPipelineLayout m_triangle_pipeline_layout;
  VkPipeline m_triangle_pipeline;

  VkPipeline m_mesh_pipeline;
  VkPipelineLayout m_mesh_pipeline_layout;

  // amd allocator
  VmaAllocator m_allocator;

  //
  VkDescriptorSetLayout m_descriptorSetLayout; 
  VkDescriptorPool m_descriptorPool; 

  VkDescriptorSetLayout m_objectSetLayout; 
    
  VkPhysicalDeviceProperties m_physicalDeviceProperties; 


  // meshes
  Mesh m_triangle_mesh;
  Mesh m_monkey;

  // deletion queue of vulkan objects
  DeletionQueue m_main_deletion_queue;

  // renderables
  std::vector<RenderObject> m_renderables;
  std::unordered_map<std::string, Material> m_materials;
  std::unordered_map<std::string, Mesh> m_meshes;
};

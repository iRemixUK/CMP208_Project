#ifndef _ABFW_PLATFORM_VITA_H
#define _ABFW_PLATFORM_VITA_H

#include <gef.h>
#include <maths/matrix44.h>
#include <system/platform.h>
#include <gxm.h>


/*	Define the width and height to render at the native resolution on ES2
	hardware.
*/
#define DISPLAY_WIDTH				960
#define DISPLAY_HEIGHT				544
#define DISPLAY_STRIDE_IN_PIXELS	1024

/*	Define the libgxm color format to render to.  This should be kept in sync
	with the display format to use with the SceDisplay library.
*/
#define DISPLAY_COLOR_FORMAT		SCE_GXM_COLOR_FORMAT_A8B8G8R8
#define DISPLAY_PIXEL_FORMAT		SCE_DISPLAY_PIXELFORMAT_A8B8G8R8

/*	Define the number of back buffers to use with this sample.  Most applications
	should use a value of 2 (double buffering) or 3 (triple buffering).
*/
#define DISPLAY_BUFFER_COUNT		3

/*	Define the maximum number of queued swaps that the display queue will allow.
	This limits the number of frames that the CPU can get ahead of the GPU,
	and is independent of the actual number of back buffers.  The display
	queue will block during sceGxmDisplayQueueAddEntry if this number of swaps
	have already been queued.
*/
#define DISPLAY_MAX_PENDING_SWAPS	2

/*	Define the MSAA mode.  This can be changed to 4X on ES1 hardware to use 4X
	multi-sample anti-aliasing, and can be changed to 4X or 2X on ES2 hardware.
*/
#define MSAA_MODE					SCE_GXM_MULTISAMPLE_NONE

/*	Set this macro to 1 to manually allocate the memblock for the render target.
*/
#define MANUALLY_ALLOCATE_RT_MEMBLOCK		0

// Helper macro to align a value
#define ALIGN(x, a)					(((x) + ((a) - 1)) & ~((a) - 1))

namespace gef
{
	// forward declarations
	class Sprite;
	class TextureVita;
	class Mesh;
	class Renderer3D;

	// Data structure for clear geometry
	struct ClearVertex
	{
		float x;
		float y;
	};


	/*	Data structure to pass through the display queue.  This structure is
		serialized during sceGxmDisplayQueueAddEntry, and is used to pass
		arbitrary data to the display callback function, called from an internal
		thread once the back buffer is ready to be displayed.

		In this example, we only need to pass the base address of the buffer.
	*/
	struct DisplayData
	{
		void *address;
	};

	class PlatformVita : public Platform
	{
	public:
		PlatformVita();
		~PlatformVita();

		bool Update();
		float GetFrameTime();
		void PreRender();
		void PostRender();
		void Clear() const;

		std::string FormatFilename(const std::string& filename) const;
		std::string FormatFilename(const char* filename) const;
		virtual SpriteRenderer* CreateSpriteRenderer();
		File* CreateFile() const;
		AudioManager* CreateAudioManager() const;
		// TouchInputManager* CreateTouchInputManager() const;
		Texture* CreateTexture(const ImageData& image_data) const;
		Mesh* CreateMesh();
		// SonyControllerInputManager* CreateSonyControllerInputManager() const;
		InputManager* CreateInputManager();
		Renderer3D* CreateRenderer3D();
		Matrix44 PerspectiveProjectionFov(const float fov, const float aspect_ratio, const float near_distance, const float far_distance) const;
		Matrix44 PerspectiveProjectionFrustum(const float left, const float right, const float top, const float bottom, const float near_distance, const float far_distance) const;
		Matrix44 OrthographicFrustum(const float left, const float right, const float top, const float bottom, const float near_distance, const float far_distance) const;

		SceGxmShaderPatcherId RegisterShaderProgram(const SceGxmProgram* program);
		void DrawClearScreen() const;
		void BeginScene() const;
		void EndScene() const;

		virtual void InitTouchInputManager();
		virtual void ReleaseTouchInputManager();
		virtual VertexBuffer* CreateVertexBuffer() const;
		virtual IndexBuffer* CreateIndexBuffer() const;
		virtual ShaderInterface* CreateShaderInterface() const;
		virtual const char* GetShaderDirectory() const;
		virtual const char* GetShaderFileExtension() const;

		virtual DepthBuffer* CreateDepthBuffer(UInt32 width, UInt32 height) const;

		inline SceGxmContext* context() const { return context_; }
		inline SceGxmShaderPatcher* shader_patcher() const { return shader_patcher_; }
	private:
		RenderTarget* CreateRenderTarget(const Int32 width, const Int32 height) const;

		Int32 Init();
		void CleanUp();

		void InitGxmLibrary();
		void CreateRendereringContext();
		void CreateRenderTarget();
		void AllocateDisplayBuffers();
		void AllocateDepthBuffer();
		void CreateShaderPatcher();
		void InitClearScreen();


		SceUID vdmRingBufferUid;
		SceUID vertexRingBufferUid;
		SceUID fragmentRingBufferUid;
		SceUID fragmentUsseRingBufferUid;
		void* hostMem;
		SceGxmContext *context_;
		SceGxmRenderTarget *renderTarget;
		SceUID displayBufferUid[DISPLAY_BUFFER_COUNT];
		void *displayBufferData[DISPLAY_BUFFER_COUNT];
		SceGxmSyncObject *displayBufferSync[DISPLAY_BUFFER_COUNT];
		SceGxmColorSurface displaySurface[DISPLAY_BUFFER_COUNT];
		SceUID depthBufferUid;
		SceGxmDepthStencilSurface depthSurface;
		SceGxmShaderPatcher *shader_patcher_;
		SceUID patcherVertexUsseUid;
		SceUID patcherFragmentUsseUid;
		SceUID patcherBufferUid;


		SceGxmShaderPatcherId clearVertexProgramId;
		SceGxmShaderPatcherId clearFragmentProgramId;
		SceUID clearVerticesUid;
		SceUID clearIndicesUid;
		SceGxmVertexProgram *clearVertexProgram;
		SceGxmFragmentProgram *clearFragmentProgram;
		ClearVertex * clearVertices;
		uint16_t * clearIndices;
		const SceGxmProgramParameter *fragment_shader_clear_colour_;


		uint32_t backBufferIndex;
		uint32_t frontBufferIndex;

		uint64_t frame_time_start_;
	};

}

#ifdef __cplusplus
extern "C"
{
#endif

void *graphicsAlloc(SceKernelMemBlockType type, uint32_t size, uint32_t alignment, uint32_t attribs, SceUID *uid);
void graphicsFree(SceUID uid);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _ABFW_PLATFORM_VITA_H

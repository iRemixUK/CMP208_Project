#include <platform/vita/graphics/render_target_vita.h>
#include <platform/vita/system/platform_vita.h>
#include <platform/vita/graphics/texture_vita.h>
#include <libdbg.h>

namespace gef
{
	RenderTarget* RenderTarget::Create(const Platform& platform, const Int32 width, const Int32 height)
	{
		return new RenderTargetVita(platform, width, height);
	}

	RenderTargetVita::RenderTargetVita(const Platform& platform, const Int32 width, const Int32 height) :
		RenderTarget(platform, width, height),
		render_target_(NULL),
		depth_buffer_uid_(0)
	{
		Int32 err = SCE_OK;

		TextureVita* texture_vita = new TextureVita(platform, width, height);
		texture_ = texture_vita;

		err = sceGxmColorSurfaceInit(&colour_surface_,
			SCE_GXM_COLOR_FORMAT_A8B8G8R8,
			SCE_GXM_COLOR_SURFACE_LINEAR,
			SCE_GXM_COLOR_SURFACE_SCALE_NONE,
			SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT, 
			width, 
			height,
			width, //ceil(width, 64), //GRAPHICS_UTIL_DEFAULT_DISPLAY_STRIDE_IN_PIXELS, //
			texture_vita->texture_data());
		SCE_DBG_ASSERT(err == SCE_OK);
#if 0
		// create the depth/stencil surface
		const uint32_t alignedWidth = ALIGN(width, SCE_GXM_TILE_SIZEX);
		const uint32_t alignedHeight = ALIGN(height, SCE_GXM_TILE_SIZEY);
		uint32_t sampleCount = alignedWidth*alignedHeight;
		uint32_t depthStrideInSamples = alignedWidth;

		// allocate it
		void *depthBufferData = graphicsAlloc(
			SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
			4*sampleCount,
			SCE_GXM_DEPTHSTENCIL_SURFACE_ALIGNMENT,
			SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
			&depth_buffer_uid_);

		// create the SceGxmDepthStencilSurface structure
		err = sceGxmDepthStencilSurfaceInit(
			&depth_surface_,
			SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24,
			SCE_GXM_DEPTH_STENCIL_SURFACE_TILED,
			depthStrideInSamples,
			depthBufferData,
			NULL);
		SCE_DBG_ASSERT(err == SCE_OK);
#endif

		// set up parameters
		SceGxmRenderTargetParams renderTargetParams;
		memset(&renderTargetParams, 0, sizeof(SceGxmRenderTargetParams));
		renderTargetParams.flags				= 0;
		renderTargetParams.width				= width;
		renderTargetParams.height				= height;
		renderTargetParams.scenesPerFrame		= 1;
		renderTargetParams.multisampleMode		= SCE_GXM_MULTISAMPLE_NONE;
		renderTargetParams.multisampleLocations	= 0;
		renderTargetParams.driverMemBlock		= SCE_UID_INVALID_UID;

		// create the render target
		err = sceGxmCreateRenderTarget(&renderTargetParams, &render_target_);
		SCE_DBG_ASSERT(err == SCE_OK);
	}

	RenderTargetVita::~RenderTargetVita()
	{
		sceGxmDestroyRenderTarget(render_target_);

		graphicsFree(depth_buffer_uid_);
	}

	void RenderTargetVita::Begin(const Platform& platform)
	{
#if 0
		Int32 err = SCE_OK;

		const PlatformVita& platform_vita = static_cast<const PlatformVita&>(platform);

		err = sceGxmBeginScene(
			platform_vita.context(),
			0,
			render_target_,
			NULL,
			NULL,
			NULL,
			&colour_surface_,
			&depth_surface_);
		SCE_DBG_ASSERT(err == SCE_OK);
#endif
	}

	void RenderTargetVita::End(const Platform& platform)
	{
#if 0
		Int32 err = SCE_OK;
		const PlatformVita& platform_vita = static_cast<const PlatformVita&>(platform);

		// end the scene on the main render target, submitting rendering work to the GPU
		err = sceGxmEndScene(platform_vita.context(), NULL, NULL);
		SCE_DBG_ASSERT(err == SCE_OK);
#endif
	}
}
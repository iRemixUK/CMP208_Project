#include "depth_buffer_vita.h"
#include <platform/vita/system/platform_vita.h>
#include <libdbg.h>




namespace gef
{
	DepthBuffer* DepthBuffer::Create(const Platform& platform, UInt32 width, UInt32 height)
	{
		return new DepthBufferVita(platform, width, height);
	}

	DepthBufferVita::DepthBufferVita(const Platform& platform, UInt32 width, UInt32 height) :
		DepthBuffer(width, height)
	{
		Int32 err = SCE_OK;

		// create the depth/stencil surface
		const uint32_t alignedWidth = ALIGN(width, SCE_GXM_TILE_SIZEX);
		const uint32_t alignedHeight = ALIGN(height, SCE_GXM_TILE_SIZEY);
		uint32_t sampleCount = alignedWidth*alignedHeight;
		uint32_t depthStrideInSamples = alignedWidth;

		// allocate it
		void *depthBufferData = graphicsAlloc(
			SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
			4 * sampleCount,
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
	}

	DepthBufferVita::~DepthBufferVita()
	{
		Release();
	}

	void DepthBufferVita::Release()
	{
		graphicsFree(depth_buffer_uid_);
	}
}
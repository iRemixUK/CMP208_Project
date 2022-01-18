#ifndef _GEF_GRAPHICS_VITA_DEPTH_BUFFER_VITA_H
#define _GEF_GRAPHICS_VITA_DEPTH_BUFFER_VITA_H

#include <graphics/depth_buffer.h>
#include <gxm.h>

namespace gef
{
	class Platform;

	class DepthBufferVita : public DepthBuffer
	{
	public:
		DepthBufferVita(const Platform& platform, UInt32 width, UInt32 height);
		~DepthBufferVita();

		void Release();

		inline const SceGxmDepthStencilSurface* depth_stencil_surface() const { return &depth_surface_; }

	protected:
		SceGxmDepthStencilSurface depth_surface_;
		SceUID depth_buffer_uid_;
	};
}

#endif // _GEF_GRAPHICS_VITA_DEPTH_BUFFER_VITA_H
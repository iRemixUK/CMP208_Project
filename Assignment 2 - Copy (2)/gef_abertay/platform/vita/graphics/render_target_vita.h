#ifndef _RENDER_TARGET_VITA_H
#define _RENDER_TARGET_VITA_H

#include <graphics/render_target.h>
#include <gxm.h>

namespace gef
{
	class TextureVita;

	class RenderTargetVita : public RenderTarget
	{
	public:
		RenderTargetVita(const Platform& platform, const Int32 width, const Int32 height);
		~RenderTargetVita();

		void Begin(const Platform& platform);
		void End(const Platform& platform);

		const Texture* texture() const
		{
			return texture_;
		}

		inline const SceGxmRenderTarget* render_target() const { return render_target_; }
		inline const SceGxmColorSurface* colour_surface() const { return &colour_surface_; }
		inline const SceGxmDepthStencilSurface* depth_surface() const { return &depth_surface_; }

	private:
		SceGxmRenderTarget *render_target_;
		SceGxmColorSurface colour_surface_;
		SceGxmDepthStencilSurface depth_surface_;
		SceUID depth_buffer_uid_;
	};
}



#endif // _RENDER_TARGET_VITA_H
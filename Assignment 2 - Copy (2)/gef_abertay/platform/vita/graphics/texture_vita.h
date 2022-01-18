#ifndef _ABFW_TEXTURE_VITA_H
#define _ABFW_TEXTURE_VITA_H

#include <graphics/texture.h>
#include <gef.h>
#include <gxm.h>

namespace gef
{
	class Platform;

	class TextureVita : public Texture
	{
	public:
		TextureVita();
		TextureVita(UInt8* gxt_data);
		TextureVita(const Platform& platform, const ImageData& image_data);
		TextureVita(const Platform& platform, const Int32 width, const Int32 height);
		~TextureVita();

		void Bind(const Platform& platform, const int texture_stage_num) const;
		void Unbind(const Platform& platform, const int texture_stage_num) const;

		inline void set_texture(const SceGxmTexture& texture) { texture_ = texture; }
		inline const SceGxmTexture& texture() const { return texture_; }
		inline uint8_t* texture_data() { return texture_data_; }
	private:
//		UInt32 width_;
//		UInt32 height_;

		SceUID texture_uid_;
		uint8_t *texture_data_;
		SceGxmTexture texture_;
		SceGxmContext* context_;
	};
}
#endif // _ABFW_TEXTURE_VITA_H
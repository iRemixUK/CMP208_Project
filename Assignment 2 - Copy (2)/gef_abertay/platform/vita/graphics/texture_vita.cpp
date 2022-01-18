#include <platform/vita/system/platform_vita.h>
#include <platform/vita/graphics/texture_vita.h>
#include <graphics/image_data.h>
#include <gxt.h>
#include <kernel.h>
#include <gxm.h>
#include <string.h>

namespace gef
{
	Texture* Texture::Create(const Platform& platform, const ImageData& image_data)
	{
		return new TextureVita(platform, image_data);
	}

	TextureVita::TextureVita() :
		texture_uid_(0),
		texture_data_(NULL)
	{
	}

	TextureVita::TextureVita(UInt8* gxt_data)
	{
		int err = SCE_OK;

		// validate gxt
		const void *gxt = gxt_data;
		SCE_DBG_ASSERT(sceGxtCheckData(gxt) == SCE_OK);

		// get the size of the texture data
		const uint32_t data_size = sceGxtGetDataSize(gxt);

		// allocate memory
		texture_data_ = (uint8_t *)graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, data_size, SCE_GXM_TEXTURE_ALIGNMENT, SCE_GXM_MEMORY_ATTRIB_READ, &texture_uid_);

		// copy texture data
		const void *dataSrc = sceGxtGetDataAddress(gxt);
		memcpy(texture_data_, dataSrc, data_size);

		// set up the texture control words
		err = sceGxtInitTexture(&texture_, gxt, texture_data_, 0);
		SCE_DBG_ASSERT(err == SCE_OK);

		// set linear filtering
		err = sceGxmTextureSetMagFilter(
			&texture_,
			SCE_GXM_TEXTURE_FILTER_LINEAR);
		SCE_DBG_ASSERT(err == SCE_OK);
		err = sceGxmTextureSetMinFilter(
			&texture_,
			SCE_GXM_TEXTURE_FILTER_LINEAR);
		SCE_DBG_ASSERT(err == SCE_OK);
	}

	TextureVita::TextureVita(const class Platform& platform, const ImageData& image_data)
	{
		int err = SCE_OK;

		// get the size of the texture data
		const uint32_t data_size = image_data.width()*image_data.height()*4;

		// allocate memory
		texture_data_ = (uint8_t *)graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, data_size, SCE_GXM_TEXTURE_ALIGNMENT, SCE_GXM_MEMORY_ATTRIB_READ, &texture_uid_);
		memcpy(texture_data_, image_data.image(), data_size);

		// set up the texture control words
		SceGxmErrorCode texture_init_err = sceGxmTextureInitLinear(&texture_, texture_data_, SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR, image_data.width(), image_data.height(), 1);
		SCE_DBG_ASSERT(texture_init_err == SCE_OK);

//		UInt32 width = sceGxmTextureGetWidth(&texture_);
//		UInt32 height = sceGxmTextureGetHeight(&texture_);

		// set linear filtering
		err = sceGxmTextureSetMagFilter(
			&texture_,
			SCE_GXM_TEXTURE_FILTER_LINEAR);
		SCE_DBG_ASSERT(err == SCE_OK);
		err = sceGxmTextureSetMinFilter(
			&texture_,
			SCE_GXM_TEXTURE_FILTER_LINEAR);
		SCE_DBG_ASSERT(err == SCE_OK);
	}

	TextureVita::TextureVita(const Platform& platform, const Int32 width, const Int32 height)
	{
		int err = SCE_OK;

		// get the size of the texture data
		const uint32_t data_size = width*height*4;

		// allocate memory
		texture_data_ = (uint8_t *)graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, data_size, SCE_GXM_TEXTURE_ALIGNMENT, SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE, &texture_uid_);

		// set up the texture control words
		SceGxmErrorCode texture_init_err = sceGxmTextureInitLinear(&texture_, texture_data_, SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR, width, height, 1);
		SCE_DBG_ASSERT(texture_init_err == SCE_OK);

//		UInt32 width = sceGxmTextureGetWidth(&texture_);
//		UInt32 height = sceGxmTextureGetHeight(&texture_);

		// set linear filtering
		err = sceGxmTextureSetMagFilter(
			&texture_,
			SCE_GXM_TEXTURE_FILTER_LINEAR);
		SCE_DBG_ASSERT(err == SCE_OK);
		err = sceGxmTextureSetMinFilter(
			&texture_,
			SCE_GXM_TEXTURE_FILTER_LINEAR);
		SCE_DBG_ASSERT(err == SCE_OK);
	}


	TextureVita::~TextureVita()
	{
		graphicsFree(texture_uid_);
	}

	void TextureVita::Bind(const Platform& platform, const int texture_stage_num) const
	{
		const PlatformVita& platform_vita = static_cast<const PlatformVita&>(platform);
		const SceGxmTexture* texture = &texture_;

		// make textures wrap
		sceGxmTextureSetUAddrMode(const_cast<SceGxmTexture*>(texture), SCE_GXM_TEXTURE_ADDR_REPEAT);
		sceGxmTextureSetVAddrMode(const_cast<SceGxmTexture*>(texture), SCE_GXM_TEXTURE_ADDR_REPEAT);

		sceGxmSetFragmentTexture(platform_vita.context(), texture_stage_num, texture);
	}

	void TextureVita::Unbind(const Platform& platform, const int texture_stage_num) const
	{
		// not used
	}
}
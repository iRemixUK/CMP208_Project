#include <platform/vita/graphics/sprite_renderer_vita.h>
#include <platform/vita/graphics/texture_vita.h>
#include <graphics/sprite.h>
#include <maths/matrix44.h>
#include <platform/vita/system/platform_vita.h>
#include <gxt.h>
#include <libdbg.h>
#include <display.h>
#include <system/platform.h>
#include <graphics/vertex_buffer.h>
#include <graphics/shader_interface.h>


extern const SceGxmProgram _binary_textured_sprite_v_gxp_start;
extern const SceGxmProgram _binary_textured_sprite_f_gxp_start;

namespace gef
{
	SpriteRenderer* SpriteRenderer::Create(Platform& platform)
	{
		return new SpriteRendererVita(platform);
	}

	SpriteRendererVita::SpriteRendererVita(Platform& platform) :
	SpriteRenderer(platform),
	colouredSpriteIndicesUid(0),
	default_texture_(NULL)
	{
		colouredSpriteIndices = (uint16_t *)graphicsAlloc(
			SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
			4*sizeof(uint16_t),
			2,
			SCE_GXM_MEMORY_ATTRIB_READ,
			&colouredSpriteIndicesUid);


		colouredSpriteIndices[0] = 0;
		colouredSpriteIndices[1] = 1;
		colouredSpriteIndices[2] = 2;
		colouredSpriteIndices[3] = 3;

		vertex_buffer_ = gef::VertexBuffer::Create(platform_);

		float vertices[] = {
			-0.5f, -0.5f, 0.0f,
			0.5f, -0.5f, 0.0f,
			-0.5f, 0.5f, 0.0f,
			0.5f, 0.5f, 0.0f };

		vertex_buffer_->Init(platform, vertices, 6, sizeof(float) * 3);
		platform_.AddVertexBuffer(vertex_buffer_);

		default_texture_ = Texture::CreateCheckerTexture(16, 1, platform);
		platform_.AddTexture(default_texture_);

		platform_.AddShader(&default_shader_);

		projection_matrix_ = platform_.OrthographicFrustum(0, platform_.width(), 0, platform_.height(), -1, 1);
		SetShader(NULL);
	}

	SpriteRendererVita::~SpriteRendererVita()
	{
		delete default_texture_;

		graphicsFree(colouredSpriteIndicesUid);
	}

	void SpriteRendererVita::Begin(bool clear)
	{
		platform_.BeginScene();
		if (clear)
			platform_.Clear();

		vertex_buffer_->Bind(platform_);

		if (shader_ == &default_shader_)
		{
			default_shader_.SetSceneData(projection_matrix_);
			default_shader_.device_interface()->UseProgram();
			default_shader_.device_interface()->SetVertexFormat();
		}
	}

	void SpriteRendererVita::End()
	{
		vertex_buffer_->Unbind(platform_);

		const PlatformVita& platform_vita = static_cast<const PlatformVita&>(platform_);
		platform_vita.EndScene();
	}

	void SpriteRendererVita::DrawSprite(const Sprite& sprite)
	{
		if (shader_ == &default_shader_)
		{
			const Texture* texture = sprite.texture();
			if (!texture)
				texture = default_texture_;
			default_shader_.SetSpriteData(sprite, texture);
			default_shader_.device_interface()->SetVariableData();

			default_shader_.device_interface()->BindTextureResources(platform_);

		}

		const PlatformVita& platform_vita = static_cast<const PlatformVita&>(platform_);
		sceGxmDraw(platform_vita.context(), SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, SCE_GXM_INDEX_FORMAT_U16, colouredSpriteIndices, 4);


		if (shader_ == &default_shader_)
		{
			default_shader_.device_interface()->UnbindTextureResources(platform_);
		}
	}
}
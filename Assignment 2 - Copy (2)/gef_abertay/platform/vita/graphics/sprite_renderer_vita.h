#ifndef _ABFW_SPRITE_RENDERER_VITA_H
#define _ABFW_SPRITE_RENDERER_VITA_H

#include <graphics/sprite_renderer.h>
#include <graphics/default_sprite_shader.h>
#include <maths/matrix44.h>
#include <gxm.h>

namespace gef
{
	// forward declarations
	class Platform;
	class TextureVita;
	class VertexBuffer;

class SpriteRendererVita : public SpriteRenderer
{
public:
	SpriteRendererVita(Platform& platform);
	~SpriteRendererVita();

	void Begin(bool clear);
	void End();
	void DrawSprite(const Sprite& sprite);
private:
//	SceGxmShaderPatcherId colouredSpriteVertexProgramId;
//	SceGxmShaderPatcherId colouredSpriteFragmentProgramId;
//	SceUID colouredSpriteVerticesUid;
	SceUID colouredSpriteIndicesUid;
//	SceGxmVertexProgram *colouredSpriteVertexProgram;
//	SceGxmFragmentProgram *colouredSpriteFragmentProgram;
//	struct SpriteVertex * colouredSpriteVertices;
	uint16_t * colouredSpriteIndices;
//	const SceGxmProgramParameter *wvpParam;
//	const SceGxmProgramParameter *spriteDataParam;


	// default texture
	Texture* default_texture_;
	VertexBuffer* vertex_buffer_;
};

}

#endif // _ABFW_SPRITE_RENDERER_VITA_H
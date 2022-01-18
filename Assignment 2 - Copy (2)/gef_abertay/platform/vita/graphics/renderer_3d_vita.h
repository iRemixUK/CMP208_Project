#ifndef _ABFW_RENDERER_3D_VITA_H
#define _ABFW_RENDERER_3D_VITA_H

#include <graphics/renderer_3d.h>
#include <graphics/default_3d_shader.h>
#include <gxm.h>
#include <graphics/primitive.h>

namespace gef
{
	class Platform;
	class MeshInstance;
	class ShaderVita;
	class TextureVita;

	class Renderer3DVita : public Renderer3D
	{
	public:
		Renderer3DVita(Platform& platform);
		~Renderer3DVita();
		void CleanUp();

		void Begin(bool clear);
		void End();
		void DrawMesh(const class MeshInstance& mesh_instance);
		void DrawMesh(const Mesh& mesh, const gef::Matrix44& matrix);
//		void ClearZBuffer();

		//void DrawPrimitive(const  MeshInstance& mesh_instance, Int32 primitive_index, Int32 num_indices);
		void SetFillMode(FillMode fill_mode);
		void SetDepthTest(DepthTest depth_test);

		void SetPrimitiveType(gef::PrimitiveType type);
		void DrawPrimitive(const IndexBuffer* index_buffer, int num_indices);

	protected:
		static const SceGxmPrimitiveType primitive_types[NUM_PRIMITIVE_TYPES];

		Texture* default_texture_;
		SceGxmPrimitiveType current_primitive_type_;
	};
}

#endif // _ABFW_RENDERER_3D_VITA_H
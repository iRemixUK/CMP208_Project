#include <platform/vita/graphics/renderer_3d_vita.h>
#include <platform/vita/system/platform_vita.h>
#include <platform/vita/graphics/default_3d_shader_vita.h>
#include <platform/vita/graphics/skinned_mesh_shader_vita.h>
#include <platform/vita/graphics/texture_vita.h>
#include <graphics/mesh.h>
#include <graphics/shader_interface.h>
#include <graphics/vertex_buffer.h>
#include <platform/vita/graphics/index_buffer_vita.h>
#include <graphics/primitive.h>
#include <graphics/material.h>

//#include <platform/vita/graphics/primitive_vita.h>
#include <graphics/mesh_instance.h>
#include <graphics/colour.h>
#include <gxm.h>

namespace gef
{
	Renderer3D* Renderer3D::Create(Platform& platform)
	{
		return new Renderer3DVita(platform);
	}

    const SceGxmPrimitiveType Renderer3DVita::primitive_types[NUM_PRIMITIVE_TYPES] =
    {
        SCE_GXM_PRIMITIVE_TRIANGLES,
        SCE_GXM_PRIMITIVE_TRIANGLE_STRIP,
        SCE_GXM_PRIMITIVE_LINES,
    };

    class Platform;

    Renderer3DVita::Renderer3DVita(Platform& platform) :
        Renderer3D(platform)
    {
        default_texture_ = Texture::CreateCheckerTexture(16, 1, platform);
        platform_.AddTexture(default_texture_);

        platform_.AddShader(&default_shader_);
        shader_ = &default_shader_;

        projection_matrix_.SetIdentity();
    }

    Renderer3DVita::~Renderer3DVita()
    {
        CleanUp();
    }

    void Renderer3DVita::CleanUp()
    {
        DeleteNull(default_texture_);
    }


    void Renderer3DVita::Begin(bool clear)
    {
        const PlatformVita& platform_vita = static_cast<const PlatformVita&>(platform());
        platform_vita.BeginScene();

        if(clear)
            platform_vita.Clear();
    }

    void Renderer3DVita::End()
    {
        const PlatformVita& platform_vita = static_cast<const PlatformVita&>(platform());
        platform_vita.EndScene();
    }

	void Renderer3DVita::DrawMesh(const  MeshInstance& mesh_instance)
	{
		// set up the shader data for default shader
		if (shader_ == &default_shader_)
			default_shader_.SetSceneData(default_shader_data_, view_matrix_, projection_matrix_);

		const Mesh* mesh = mesh_instance.mesh();
		if (mesh != NULL)
		{
			set_world_matrix(mesh_instance.transform());

			const VertexBuffer* vertex_buffer = mesh->vertex_buffer();
			//ShaderGL* shader_GL = static_cast<ShaderGL*>(shader_);

			if (vertex_buffer && shader_)
			{
				shader_->SetMeshData(mesh_instance);

				shader_->device_interface()->UseProgram();
				vertex_buffer->Bind(platform_);

				// vertex format must be set after the vertex buffer is bound
				shader_->device_interface()->SetVertexFormat();

				for (UInt32 primitive_index = 0; primitive_index<mesh->num_primitives(); ++primitive_index)
				{
					const Primitive* primitive = mesh->GetPrimitive(primitive_index);
					const IndexBuffer* index_buffer = primitive->index_buffer();
					if (primitive->type() != UNDEFINED && index_buffer)
					{
						// default texture
						const Material* material;
						if (override_material_)
							material = override_material_;
						else
							material = primitive->material();


						//only set default shader data if current shader is the default shader
						shader_->SetMaterialData(material);

						// GRC FIXME - probably want to split variable data into scene, object, primitive[material?] based
						// rather than set all variables per primitive
						shader_->device_interface()->SetVariableData();
						shader_->device_interface()->BindTextureResources(platform());


						index_buffer->Bind(platform_);

						const PlatformVita& platform_vita = static_cast<const PlatformVita&>(platform_);
						const IndexBufferVita* index_buffer_vita = static_cast<const IndexBufferVita*>(primitive->index_buffer());
						sceGxmDraw(platform_vita.context(), primitive_types[primitive->type()], index_buffer_vita->index_format(), index_buffer_vita->graphics_data(), index_buffer_vita->num_indices());



						index_buffer->Unbind(platform_);
						shader_->device_interface()->UnbindTextureResources(platform());
					}
				}

				vertex_buffer->Unbind(platform_);
				shader_->device_interface()->ClearVertexFormat();
			}
		}
	}

	void Renderer3DVita::DrawMesh(const Mesh& mesh, const gef::Matrix44& matrix)
	{
		// set up the shader data for default shader
		if (shader_ == &default_shader_)
			default_shader_.SetSceneData(default_shader_data_, view_matrix_, projection_matrix_);

		{
			set_world_matrix(matrix);

			const VertexBuffer* vertex_buffer = mesh.vertex_buffer();
			//ShaderGL* shader_GL = static_cast<ShaderGL*>(shader_);

			if (vertex_buffer && shader_)
			{
				shader_->SetMeshData(matrix);

				shader_->device_interface()->UseProgram();
				vertex_buffer->Bind(platform_);

				// vertex format must be set after the vertex buffer is bound
				shader_->device_interface()->SetVertexFormat();

				for (UInt32 primitive_index = 0; primitive_index<mesh.num_primitives(); ++primitive_index)
				{
					const Primitive* primitive = mesh.GetPrimitive(primitive_index);
					const IndexBuffer* index_buffer = primitive->index_buffer();
					if (primitive->type() != UNDEFINED && index_buffer)
					{
						// default texture
						const Material* material;
						if (override_material_)
							material = override_material_;
						else
							material = primitive->material();


						//only set default shader data if current shader is the default shader
						shader_->SetMaterialData(material);

						// GRC FIXME - probably want to split variable data into scene, object, primitive[material?] based
						// rather than set all variables per primitive
						shader_->device_interface()->SetVariableData();
						shader_->device_interface()->BindTextureResources(platform());

						SetPrimitiveType(primitive->type());

						index_buffer->Bind(platform_);

						DrawPrimitive(index_buffer, index_buffer->num_indices());

						index_buffer->Unbind(platform_);
						shader_->device_interface()->UnbindTextureResources(platform());
					}
				}

				shader_->device_interface()->ClearVertexFormat();
				vertex_buffer->Unbind(platform_);
			}
		}
	}

    //void Renderer3DVita::DrawPrimitive(const  MeshInstance& mesh_instance, Int32 primitive_index, Int32 num_indices)
    //{

    //}

	void Renderer3DVita::SetFillMode(FillMode fill_mode)
	{
		const PlatformVita& platform_vita = static_cast<const PlatformVita&>(platform_);
		switch (fill_mode)
		{
		case kSolid:
			sceGxmSetFrontPolygonMode(platform_vita.context(), SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
			break;
		case kWireframe:
			sceGxmSetFrontPolygonMode(platform_vita.context(), SCE_GXM_POLYGON_MODE_TRIANGLE_LINE);
			break;
		case kLines:
			sceGxmSetFrontPolygonMode(platform_vita.context(), SCE_GXM_POLYGON_MODE_LINE);
			break;
		}
	}

    void Renderer3DVita::SetDepthTest(DepthTest depth_test)
    {

    }

	void Renderer3DVita::SetPrimitiveType(gef::PrimitiveType type)
	{
		current_primitive_type_ = primitive_types[type];
	}

	void Renderer3DVita::DrawPrimitive(const IndexBuffer* index_buffer, int num_indices)
	{
		const PlatformVita& platform_vita = static_cast<const PlatformVita&>(platform_);
		const IndexBufferVita* index_buffer_vita = static_cast<const IndexBufferVita*>(index_buffer);
		sceGxmDraw(platform_vita.context(), current_primitive_type_, index_buffer_vita->index_format(), index_buffer_vita->graphics_data(), num_indices);
	}
}

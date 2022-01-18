#include <platform/vita/graphics/skinned_mesh_shader_vita.h>
#include <platform/vita/graphics/renderer_3d_vita.h>
#include <cstddef>
#include <libdbg.h>
#include <graphics/mesh.h>
#include <platform/vita/system/platform_vita.h>
#include <graphics/skinned_mesh_shader_data.h>

extern const SceGxmProgram _binary_skinning_shader_v_gxp_start;
extern const SceGxmProgram _binary_skinning_shader_f_gxp_start;

namespace gef
{
	SkinnedMeshShaderVita::SkinnedMeshShaderVita(SceGxmShaderPatcher* shader_patcher, const SkinnedMeshShaderData* const shader_data) :
			ShaderVita(shader_patcher, &_binary_skinning_shader_v_gxp_start, &_binary_skinning_shader_f_gxp_start),
//			vertex_shader_parameters_(NULL),
//			fragment_shader_parameters_(NULL),
			shader_data_(shader_data)

	{
		const SceGxmProgram *const vertex_program	= &_binary_skinning_shader_v_gxp_start;
		const SceGxmProgram *const fragment_program	= &_binary_skinning_shader_f_gxp_start;

		const SceGxmProgramParameter *vertex_param_input_pos_attribute = sceGxmProgramFindParameterByName(vertex_program, "input_position");
		SCE_DBG_ASSERT(vertex_param_input_pos_attribute && (sceGxmProgramParameterGetCategory(vertex_param_input_pos_attribute) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE));
		const SceGxmProgramParameter *vertex_param_input_norm_attribute = sceGxmProgramFindParameterByName(vertex_program, "input_normal");
		SCE_DBG_ASSERT(vertex_param_input_norm_attribute && (sceGxmProgramParameterGetCategory(vertex_param_input_norm_attribute) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE));
		const SceGxmProgramParameter *vertex_param_input_uv_attribute = sceGxmProgramFindParameterByName(vertex_program, "input_uv");
		SCE_DBG_ASSERT(vertex_param_input_uv_attribute && (sceGxmProgramParameterGetCategory(vertex_param_input_uv_attribute) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE));
		const SceGxmProgramParameter *vertex_param_input_blendindices_attribute = sceGxmProgramFindParameterByName(vertex_program, "input_blendindices");
		SCE_DBG_ASSERT(vertex_param_input_blendindices_attribute && (sceGxmProgramParameterGetCategory(vertex_param_input_blendindices_attribute) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE));
		const SceGxmProgramParameter *vertex_param_input_blendweights_attribute = sceGxmProgramFindParameterByName(vertex_program, "input_blendweights");
		SCE_DBG_ASSERT(vertex_param_input_blendweights_attribute && (sceGxmProgramParameterGetCategory(vertex_param_input_blendweights_attribute) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE));

		vertex_shader_parameters_wvp_ = sceGxmProgramFindParameterByName(vertex_program, "shader_data_wvp");
		SCE_DBG_ASSERT(vertex_shader_parameters_wvp_ && (sceGxmProgramParameterGetCategory(vertex_shader_parameters_wvp_) == SCE_GXM_PARAMETER_CATEGORY_UNIFORM));
		vertex_shader_parameters_world_ = sceGxmProgramFindParameterByName(vertex_program, "shader_data_world");
		SCE_DBG_ASSERT(vertex_shader_parameters_world_ && (sceGxmProgramParameterGetCategory(vertex_shader_parameters_world_) == SCE_GXM_PARAMETER_CATEGORY_UNIFORM));
		vertex_shader_parameters_light_position_ = sceGxmProgramFindParameterByName(vertex_program, "shader_data_light_position");
		SCE_DBG_ASSERT(vertex_shader_parameters_light_position_ && (sceGxmProgramParameterGetCategory(vertex_shader_parameters_light_position_) == SCE_GXM_PARAMETER_CATEGORY_UNIFORM));
		vertex_shader_parameters_bone_matrices_ = sceGxmProgramFindParameterByName(vertex_program, "bone_matrices");
		SCE_DBG_ASSERT(vertex_shader_parameters_bone_matrices_ && (sceGxmProgramParameterGetCategory(vertex_shader_parameters_bone_matrices_) == SCE_GXM_PARAMETER_CATEGORY_UNIFORM));

		fragment_shader_parameters_ambient_light_colour_ = sceGxmProgramFindParameterByName(fragment_program, "shader_data_ambient_light_colour");
		SCE_DBG_ASSERT(fragment_shader_parameters_ambient_light_colour_ && (sceGxmProgramParameterGetCategory(fragment_shader_parameters_ambient_light_colour_) == SCE_GXM_PARAMETER_CATEGORY_UNIFORM));
		fragment_shader_parameters_light_colour_ = sceGxmProgramFindParameterByName(fragment_program, "shader_data_light_colour");
		SCE_DBG_ASSERT(fragment_shader_parameters_light_colour_ && (sceGxmProgramParameterGetCategory(fragment_shader_parameters_light_colour_) == SCE_GXM_PARAMETER_CATEGORY_UNIFORM));

		Int32 err = SCE_OK;

		// create shaded triangle vertex format
		SceGxmVertexAttribute vertex_attributes[5];
		vertex_attributes[0].streamIndex = 0;
		vertex_attributes[0].offset = 0;
		vertex_attributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		vertex_attributes[0].componentCount = 3;
		vertex_attributes[0].regIndex = sceGxmProgramParameterGetResourceIndex(vertex_param_input_pos_attribute);

		vertex_attributes[1].streamIndex = 0;
		vertex_attributes[1].offset = 12;
		vertex_attributes[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		vertex_attributes[1].componentCount = 3;
		vertex_attributes[1].regIndex = sceGxmProgramParameterGetResourceIndex(vertex_param_input_norm_attribute);

		vertex_attributes[2].streamIndex = 0;
		vertex_attributes[2].offset = 24;
		vertex_attributes[2].format = SCE_GXM_ATTRIBUTE_FORMAT_U8;
		vertex_attributes[2].componentCount = 4;
		vertex_attributes[2].regIndex = sceGxmProgramParameterGetResourceIndex(vertex_param_input_blendindices_attribute);

		vertex_attributes[3].streamIndex = 0;
		vertex_attributes[3].offset = 28;
		vertex_attributes[3].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		vertex_attributes[3].componentCount = 4;
		vertex_attributes[3].regIndex = sceGxmProgramParameterGetResourceIndex(vertex_param_input_blendweights_attribute);

		vertex_attributes[4].streamIndex = 0;
		vertex_attributes[4].offset = 44;
		vertex_attributes[4].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		vertex_attributes[4].componentCount = 2;
		vertex_attributes[4].regIndex = sceGxmProgramParameterGetResourceIndex(vertex_param_input_uv_attribute);


		SceGxmVertexStream vertex_streams[1];
		vertex_streams[0].stride = sizeof(gef::Mesh::SkinnedVertex);
		vertex_streams[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_32BIT;

		// create shaded triangle shaders
		err = sceGxmShaderPatcherCreateVertexProgram(
			shader_patcher,
			vertex_program_id_,
			vertex_attributes,
			5,
			vertex_streams,
			1,
			&vertex_program_);
		SCE_DBG_ASSERT(err == SCE_OK);


		// blended fragment program
		SceGxmBlendInfo	blendInfo;
		blendInfo.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
		blendInfo.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
		blendInfo.colorSrc = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
		blendInfo.colorDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendInfo.alphaSrc = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
		blendInfo.alphaDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendInfo.colorMask = SCE_GXM_COLOR_MASK_ALL;

		err = sceGxmShaderPatcherCreateFragmentProgram(
			shader_patcher,
			fragment_program_id_,
			SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
			MSAA_MODE,
			&blendInfo,
			sceGxmVertexProgramGetProgram(vertex_program_),
			&fragment_program_);
		SCE_DBG_ASSERT(err == SCE_OK);
	}

	SkinnedMeshShaderVita::~SkinnedMeshShaderVita()
	{
	}

	void SkinnedMeshShaderVita::SetConstantBuffers(SceGxmContext* context, const void* data)
	{

		const Renderer3DVita* renderer = static_cast<const Renderer3DVita*>(data);


		// Copy the matrices into the constant buffer.
//		vertex_shader_data_.world.Transpose(renderer->world_matrix());
//		vertex_shader_data_.wvp.Transpose(renderer->world_matrix()*renderer->view_matrix()*renderer->projection_matrix());
		vertex_shader_data_.world=renderer->world_matrix();
		vertex_shader_data_.wvp=renderer->world_matrix()*renderer->view_matrix()*renderer->projection_matrix();
//		vertex_shader_data_.wvp=renderer->projection_matrix()*renderer->view_matrix()*renderer->world_matrix();

		fragment_shader_data_.ambient_light_colour = shader_data_->ambient_light_colour().GetRGBAasVector4();
		// light position
		for(UInt32 light_num=0;light_num < MAX_NUM_POINT_LIGHTS; ++light_num)
		{
			Vector4 light_position;
			Colour light_colour;
			if(light_num < shader_data_->GetNumPointLights())
			{
				const PointLight& point_light = shader_data_->GetPointLight(light_num);
				light_position = point_light.position();
				light_colour = point_light.colour();
			}
			else
			{
				// no light data
				// set this light to a light with no colour
				light_position = Vector4(0.0f, 0.0f, 0.0f);
				light_colour = Colour(0.0f, 0.0f, 0.0f);
			}
			vertex_shader_data_.light_position[light_num] = Vector4(light_position.x(), light_position.y(), light_position.z(), 1.f);
			fragment_shader_data_.light_colour[light_num] = light_colour.GetRGBAasVector4();
		}

		// bone matrices
		if(shader_data_->bone_matrices())
			memcpy(bone_matrices_data_.bone_matrices, &(*shader_data_->bone_matrices())[0], shader_data_->bone_matrices()->size()*sizeof(Matrix44));

		// set the vertex program constants
		void *vertex_shader_data_buffer;
		sceGxmReserveVertexDefaultUniformBuffer(context, &vertex_shader_data_buffer);
		sceGxmSetUniformDataF(vertex_shader_data_buffer, vertex_shader_parameters_wvp_, 0, 16, vertex_shader_data_.wvp.float_ptr());
		sceGxmSetUniformDataF(vertex_shader_data_buffer, vertex_shader_parameters_world_, 0, 16, vertex_shader_data_.world.float_ptr());
		sceGxmSetUniformDataF(vertex_shader_data_buffer, vertex_shader_parameters_light_position_, 0, 16, vertex_shader_data_.light_position[0].float_ptr());
		sceGxmSetUniformDataF(vertex_shader_data_buffer, vertex_shader_parameters_bone_matrices_, 0, 16*NUM_BONE_MATRICES, bone_matrices_data_.bone_matrices[0].float_ptr());

		// set the fragment program constants
		void *fragment_shader_data_buffer;
		sceGxmReserveFragmentDefaultUniformBuffer(context, &fragment_shader_data_buffer);
		sceGxmSetUniformDataF(fragment_shader_data_buffer, fragment_shader_parameters_ambient_light_colour_, 0, 4, fragment_shader_data_.ambient_light_colour.float_ptr());
		sceGxmSetUniformDataF(fragment_shader_data_buffer, fragment_shader_parameters_light_colour_, 0, 16, fragment_shader_data_.light_colour[0].float_ptr());
	}
}
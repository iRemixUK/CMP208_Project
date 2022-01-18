#include <platform/vita/graphics/shader_interface_vita.h>
#include <platform/vita/system/platform_vita.h>
#include <libdbg.h>
#include <graphics/texture.h>

namespace gef
{
	ShaderInterface* ShaderInterface::Create(const Platform& platform)
	{
		const PlatformVita& platform_vita = reinterpret_cast<const PlatformVita&>(platform);
		return new ShaderInterfaceVita(platform_vita.context(), platform_vita.shader_patcher());
	}

	ShaderInterfaceVita::ShaderInterfaceVita(SceGxmContext* context, SceGxmShaderPatcher* shader_patcher) :
		context_(context),
		shader_patcher_(shader_patcher)
	{

	}

	ShaderInterfaceVita::~ShaderInterfaceVita()
	{
		sceGxmShaderPatcherReleaseFragmentProgram(shader_patcher_, fragment_program_);
		sceGxmShaderPatcherReleaseVertexProgram(shader_patcher_, vertex_program_);

		sceGxmShaderPatcherUnregisterProgram(shader_patcher_, fragment_program_id_);
		sceGxmShaderPatcherUnregisterProgram(shader_patcher_, vertex_program_id_);
	}

	bool ShaderInterfaceVita::CreateProgram()
	{
		//
		// allocate memory to store local copy of variable data
		//
		AllocateVariableData();

		const SceGxmProgram *const vertex_program = (SceGxmProgram *)vs_shader_source_;
		const SceGxmProgram *const fragment_program = (SceGxmProgram *)ps_shader_source_;
		Int32 err = SCE_OK;


		// register programs with the patcher
		err = sceGxmShaderPatcherRegisterProgram(shader_patcher_, vertex_program, &vertex_program_id_);
		SCE_DBG_ASSERT(err == SCE_OK);
		err = sceGxmShaderPatcherRegisterProgram(shader_patcher_, fragment_program, &fragment_program_id_);
		SCE_DBG_ASSERT(err == SCE_OK);

		// go through all shader variables and find them in the shader program
		for (std::vector<ShaderVariable>::const_iterator shader_variable = vertex_shader_variables_.begin(); shader_variable != vertex_shader_variables_.end(); ++shader_variable)
		{
			const SceGxmProgramParameter* shader_parameter = sceGxmProgramFindParameterByName(vertex_program, shader_variable->name.c_str());
			SCE_DBG_ASSERT(shader_parameter && (sceGxmProgramParameterGetCategory(shader_parameter) == SCE_GXM_PARAMETER_CATEGORY_UNIFORM));
			vertex_program_parameters_.push_back(shader_parameter);
		}
		for (std::vector<ShaderVariable>::const_iterator shader_variable = pixel_shader_variables_.begin(); shader_variable != pixel_shader_variables_.end(); ++shader_variable)
		{
			const SceGxmProgramParameter* shader_parameter = sceGxmProgramFindParameterByName(fragment_program, shader_variable->name.c_str());
			SCE_DBG_ASSERT(shader_parameter && (sceGxmProgramParameterGetCategory(shader_parameter) == SCE_GXM_PARAMETER_CATEGORY_UNIFORM));
			fragment_program_parameters_.push_back(shader_parameter);
		}

		SceGxmVertexStream vertex_streams[1];
		vertex_streams[0].stride = vertex_size_;

		// GRC FIXME - all indices are 32 bit at the moment
		vertex_streams[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_32BIT;

		// create vertex program
		err = sceGxmShaderPatcherCreateVertexProgram(
			shader_patcher_,
			vertex_program_id_,
			&vertex_attributes_[0],
			vertex_attributes_.size(),
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
			shader_patcher_,
			fragment_program_id_,
			SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
			MSAA_MODE,
			&blendInfo,
			sceGxmVertexProgramGetProgram(vertex_program_),
			&fragment_program_);
		SCE_DBG_ASSERT(err == SCE_OK);

		// vertex attributes are not needed after the program has been created
		vertex_attributes_.clear();

		return true;
	}
	void ShaderInterfaceVita::CreateVertexFormat()
	{
		const SceGxmProgram *const vertex_program = (const SceGxmProgram *)vs_shader_source_;
		for (int shader_parameter_num = 0; shader_parameter_num < parameters_.size(); ++shader_parameter_num)
		{
			const ShaderParameter& shader_parameter = parameters_[shader_parameter_num];
			SceGxmVertexAttribute vertex_attribute;

			const SceGxmProgramParameter *vertex_param_attribute = sceGxmProgramFindParameterByName(vertex_program, shader_parameter.name.c_str());
			SCE_DBG_ASSERT(vertex_param_attribute && (sceGxmProgramParameterGetCategory(vertex_param_attribute) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE));

			vertex_attribute.streamIndex = 0;
			vertex_attribute.offset = shader_parameter.byte_offset;
			vertex_attribute.format = GetVertexAttributeFormat(shader_parameter.type);
			vertex_attribute.componentCount = GetVertexAttributeComponentCount(shader_parameter.type);
			vertex_attribute.regIndex = sceGxmProgramParameterGetResourceIndex(vertex_param_attribute);

			vertex_attributes_.push_back(vertex_attribute);
		}


	}

	void ShaderInterfaceVita::UseProgram()
	{
		sceGxmSetVertexProgram(context_, vertex_program_);
		sceGxmSetFragmentProgram(context_, fragment_program_);
	}

	void ShaderInterfaceVita::SetVariable(std::vector<ShaderVariable>& variables, UInt8* variables_data, Int32 variable_index, const void* value, Int32 variable_count)
	{
		ShaderVariable& shader_variable = variables[variable_index];
		void* variable_data = &static_cast<UInt8*>(variables_data)[shader_variable.byte_offset];

		if (shader_variable.type == kMatrix44)
		{
			if (variable_count == -1)
				variable_count = shader_variable.count;

			const Matrix44* src_matrices = static_cast<const Matrix44*>(value);
			Matrix44* dest_matrices = static_cast<Matrix44*>(variable_data);
			for (Int32 matrix_num = 0; matrix_num < variable_count; ++matrix_num, ++src_matrices, ++dest_matrices)
				dest_matrices->Transpose(*src_matrices);
		}
		else
			ShaderInterface::SetVariable(variables, variables_data, variable_index, value, variable_count);
	}


	void ShaderInterfaceVita::SetVariableData()
	{
		void *vertex_shader_data_buffer;
		sceGxmReserveVertexDefaultUniformBuffer(context_, &vertex_shader_data_buffer);

		std::vector<const SceGxmProgramParameter*>::const_iterator vertex_shader_parameter = vertex_program_parameters_.begin();
		for (std::vector<ShaderVariable>::const_iterator shader_variable = vertex_shader_variables_.begin(); shader_variable != vertex_shader_variables_.end(); ++shader_variable, ++vertex_shader_parameter)
		{
			const UInt8* data = &vertex_shader_variable_data_[shader_variable->byte_offset];

			// GRC FIXME - assuming all variables are float type
			const SceGxmProgramParameter* param = *vertex_shader_parameter;
			sceGxmSetUniformDataF(vertex_shader_data_buffer, param, 0, shader_variable->count*GetVertexAttributeComponentCount(shader_variable->type), (const float *)data);
		}

		void *fragment_shader_data_buffer;
		sceGxmReserveFragmentDefaultUniformBuffer(context_, &fragment_shader_data_buffer);

		std::vector<const SceGxmProgramParameter*>::const_iterator fragment_shader_parameter = fragment_program_parameters_.begin();
		for (std::vector<ShaderVariable>::const_iterator shader_variable = pixel_shader_variables_.begin(); shader_variable != pixel_shader_variables_.end(); ++shader_variable, ++fragment_shader_parameter)
		{
			const UInt8* data = &pixel_shader_variable_data_[shader_variable->byte_offset];

			// GRC FIXME - assuming all variables are float type
			sceGxmSetUniformDataF(fragment_shader_data_buffer, *fragment_shader_parameter, 0, shader_variable->count*GetVertexAttributeComponentCount(shader_variable->type), (const float *)data);
		}
	}

	void ShaderInterfaceVita::SetVertexFormat()
	{

	}

	void ShaderInterfaceVita::ClearVertexFormat()
	{
	}

	void ShaderInterfaceVita::BindTextureResources(const Platform& platform) const
	{
		Int32 texture_stage_num = 0;
		for (std::vector<TextureSampler>::const_iterator texture_sampler = texture_samplers_.begin(); texture_sampler != texture_samplers_.end(); ++texture_sampler)
		{
			if (texture_sampler->texture)
				texture_sampler->texture->Bind(platform, texture_stage_num);
			else if (platform.default_texture())
				platform.default_texture()->Bind(platform, texture_stage_num);
            texture_stage_num++;
		}
	}

	void ShaderInterfaceVita::UnbindTextureResources(const Platform& platform) const
	{
	}

	UInt8 ShaderInterfaceVita::GetVertexAttributeFormat(VariableType type)
	{
		UInt8 attribute_type = SCE_GXM_ATTRIBUTE_FORMAT_UNTYPED;
		switch (type)
		{
		case kUByte4:
			attribute_type = SCE_GXM_ATTRIBUTE_FORMAT_U8;
			break;
		case kFloat:
			attribute_type = SCE_GXM_ATTRIBUTE_FORMAT_F32;
			break;
		case kVector2:
			attribute_type = SCE_GXM_ATTRIBUTE_FORMAT_F32;
			break;
		case kVector3:
			attribute_type = SCE_GXM_ATTRIBUTE_FORMAT_F32;
			break;
		case kVector4:
			attribute_type = SCE_GXM_ATTRIBUTE_FORMAT_F32;
			break;
		case kMatrix44:
			attribute_type = SCE_GXM_ATTRIBUTE_FORMAT_F32;
			break;

		}

		return attribute_type;
	}

	Int32 ShaderInterfaceVita::GetVertexAttributeComponentCount(VariableType type)
	{
		Int32 component_count = 0;
		switch (type)
		{

		case kUByte4:
			component_count = 4;
			break;
		case kFloat:
			component_count = 1;
			break;
		case kVector2:
			component_count = 2;
			break;
		case kVector3:
			component_count = 3;
			break;
		case kVector4:
			component_count = 4;
			break;
		case kMatrix44:
			component_count = 16;
			break;
		}

		return component_count;
	}

}

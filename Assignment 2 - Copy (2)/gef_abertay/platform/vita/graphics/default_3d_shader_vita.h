#ifndef _ABFW_DEFAULT_3D_SHADER_VITA_H
#define _ABFW_DEFAULT_3D_SHADER_VITA_H

#include <platform/vita/graphics/shader_vita.h>
#include <gxm.h>
#include <maths/matrix44.h>
#include <maths/vector4.h>

#define MAX_NUM_POINT_LIGHTS 4

namespace gef
{
	class Default3DShaderData;

	class Default3DShaderVita : public ShaderVita
	{
	public:
		Default3DShaderVita(SceGxmShaderPatcher* shader_patcher, const Default3DShaderData* const shader_data);
		~Default3DShaderVita();

		void SetConstantBuffers(SceGxmContext* context, const void* data);

	private:
		struct Default3DShaderData_VS
		{
			Matrix44 wvp;
			Matrix44 world;
			Vector4 light_position[MAX_NUM_POINT_LIGHTS];
		};

		struct Default3DShaderData_FS
		{
			Vector4 ambient_light_colour;
			Vector4 light_colour[MAX_NUM_POINT_LIGHTS];
		};

		const SceGxmProgramParameter *vertex_shader_parameters_wvp_;
		const SceGxmProgramParameter *vertex_shader_parameters_world_;
		const SceGxmProgramParameter *vertex_shader_parameters_light_position_;
		const SceGxmProgramParameter *vertex_shader_parameters_bone_matrices_;
		const SceGxmProgramParameter *fragment_shader_parameters_ambient_light_colour_;
		const SceGxmProgramParameter *fragment_shader_parameters_light_colour_;

		const Default3DShaderData* const shader_data_;

		Default3DShaderData_VS vertex_shader_data_;
		Default3DShaderData_FS fragment_shader_data_;
	};
}

#endif // _ABFW_DEFAULT_3D_SHADER_VITA_H
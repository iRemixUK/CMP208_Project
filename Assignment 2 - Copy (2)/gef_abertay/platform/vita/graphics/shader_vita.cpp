#include <platform/vita/graphics/shader_vita.h>
#include <cstddef>
#include <libdbg.h>

namespace gef
{
	ShaderVita::ShaderVita(SceGxmShaderPatcher* shader_patcher, const SceGxmProgram *const vertex_program_gxp_, const SceGxmProgram *const fragment_program_gxp_) :
	vertex_program_id_(NULL),
	fragment_program_id_(NULL),
	vertex_program_(NULL),
	fragment_program_(NULL),
	shader_patcher_(shader_patcher)
	{
		Int32 err = SCE_OK;

		// register programs with the patcher
		err = sceGxmShaderPatcherRegisterProgram(shader_patcher, vertex_program_gxp_, &vertex_program_id_);
		SCE_DBG_ASSERT(err == SCE_OK);
		err = sceGxmShaderPatcherRegisterProgram(shader_patcher, fragment_program_gxp_, &fragment_program_id_);
		SCE_DBG_ASSERT(err == SCE_OK);
	}

	ShaderVita::~ShaderVita()
	{
		sceGxmShaderPatcherReleaseFragmentProgram(shader_patcher_, fragment_program_);
		sceGxmShaderPatcherReleaseVertexProgram(shader_patcher_, vertex_program_);

		sceGxmShaderPatcherUnregisterProgram(shader_patcher_, fragment_program_id_);
		sceGxmShaderPatcherUnregisterProgram(shader_patcher_, vertex_program_id_);
	}

	void ShaderVita::Set(SceGxmContext* context)
	{
		sceGxmSetVertexProgram(context, vertex_program_);
		sceGxmSetFragmentProgram(context, fragment_program_);
	}
}
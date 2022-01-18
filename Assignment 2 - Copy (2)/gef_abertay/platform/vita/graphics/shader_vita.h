#ifndef _ABFW_SHADER_VITA_H
#define _ABFW_SHADER_VITA_H

#include <gef.h>
#include <graphics/shader.h>

#include <gxm.h>

namespace gef
{
	class ShaderVita : public Shader
	{
	public:
		ShaderVita(SceGxmShaderPatcher* shader_patcher, const SceGxmProgram *const vertex_program_gxp_, const SceGxmProgram *const fragment_program_gxp_);
		~ShaderVita();
		virtual void Set(SceGxmContext* context);
		virtual void SetConstantBuffers(SceGxmContext* context, const void* data) = 0;

	protected:
		SceGxmShaderPatcherId vertex_program_id_;
		SceGxmShaderPatcherId fragment_program_id_;
		SceGxmVertexProgram *vertex_program_;
		SceGxmFragmentProgram *fragment_program_;
		SceGxmShaderPatcher* shader_patcher_;
	};
}
#endif // _ABFW_SHADER_VITA_H
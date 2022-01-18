#ifndef _GRAPHICS_VITA_SHADER_INTERFACE_VITA_H
#define _GRAPHICS_VITA_SHADER_INTERFACE_VITA_H

#include <graphics/shader_interface.h>
#include <gxm.h>

namespace gef
{
	class Platform;

	class ShaderInterfaceVita : public ShaderInterface
	{
	public:
		ShaderInterfaceVita(SceGxmContext* context, SceGxmShaderPatcher* shader_patcher);
		~ShaderInterfaceVita();

		bool CreateProgram();
		void CreateVertexFormat();

		void UseProgram();

		void SetVariableData();
		void SetVertexFormat();
		void ClearVertexFormat();

		void BindTextureResources(const Platform& platform) const;
		void UnbindTextureResources(const Platform& platform) const;

	protected:
//		void SetInputAssemblyElement(const ShaderParameter& shader_parameter, D3D11_INPUT_ELEMENT_DESC& element);
//		void CreateVertexShaderConstantBuffer();
//		void CreatePixelShaderConstantBuffer();
//		void CreateSamplerStates();

		void SetVariable(std::vector<ShaderVariable>& variables, UInt8* variables_data, Int32 variable_index, const void* value, Int32 variable_count = -1);

		UInt8 GetVertexAttributeFormat(VariableType type);
		Int32 GetVertexAttributeComponentCount(VariableType type);

		SceGxmContext* context_;
		SceGxmShaderPatcherId vertex_program_id_;
		SceGxmShaderPatcherId fragment_program_id_;
		SceGxmVertexProgram *vertex_program_;
		SceGxmFragmentProgram *fragment_program_;
		SceGxmShaderPatcher* shader_patcher_;


		std::vector<SceGxmVertexAttribute> vertex_attributes_;

		std::vector<const SceGxmProgramParameter*> vertex_program_parameters_;
		std::vector<const SceGxmProgramParameter*> fragment_program_parameters_;
	};
}

#endif // _GRAPHICS_D3D11_SHADER_INTERFACE_D3D11_H
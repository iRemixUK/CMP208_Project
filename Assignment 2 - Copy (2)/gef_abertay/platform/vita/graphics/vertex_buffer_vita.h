#ifndef _ABFW_VERTEX_BUFFER_VITA_H
#define _ABFW_VERTEX_BUFFER_VITA_H

#include <graphics/vertex_buffer.h>
#include <gxm.h>

namespace gef
{
	class VertexBufferVita : public VertexBuffer
	{
	public:
		VertexBufferVita();
		~VertexBufferVita();
		bool Init(const Platform& platform, const void* vertices, const UInt32 num_vertices, const UInt32 vertex_byte_size, const bool read_only = true);
		bool Update(const Platform& platform);

		void Bind(const Platform& platform) const;
		void Unbind(const Platform& platform) const;
	private:
		SceUID vertices_uid_;
		void* vertices_;
	};
}

#endif // _ABFW_VERTEX_BUFFER_D3D11_H
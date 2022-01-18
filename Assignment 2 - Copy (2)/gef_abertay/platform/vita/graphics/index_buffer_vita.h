#ifndef _ABFW_INDEX_BUFFER_VITA_H
#define _ABFW_INDEX_BUFFER_VITA_H

#include <graphics/index_buffer.h>
#include <gxm.h>

namespace gef
{
	class IndexBufferVita : public IndexBuffer
	{
	public:
		IndexBufferVita();
		~IndexBufferVita();

		bool Init(const Platform& platform, const void* indices, const UInt32 num_indices, const UInt32 index_byte_size, const bool read_only = true);
		void Bind(const Platform& platform) const;
		void Unbind(const Platform& platform) const;
		bool Update(const Platform& platform);

		inline SceGxmIndexFormat index_format() const { return index_format_; }
		inline void* graphics_data() const { return indices_; }
	private:
		void* indices_;
		SceUID indices_uid_;

		SceGxmIndexFormat index_format_;
	};
}

#endif // _ABFW_INDEX_BUFFER_VITA_H
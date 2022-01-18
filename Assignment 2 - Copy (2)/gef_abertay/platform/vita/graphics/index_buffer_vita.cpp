#include <platform/vita/graphics/index_buffer_vita.h>
#include <platform/vita/system/platform_vita.h>

namespace gef
{
	IndexBuffer* IndexBuffer::Create(Platform& platform)
	{
		return new IndexBufferVita();
	}

	IndexBufferVita::IndexBufferVita() :
		indices_(NULL),
		indices_uid_(0)
	{
	}

	IndexBufferVita::~IndexBufferVita()
	{
		graphicsFree(indices_uid_);
	}

	bool IndexBufferVita::Init(const Platform& platform, const void* indices, const UInt32 num_indices, const UInt32 index_byte_size, const bool read_only)
	{
		// GRC FIXME - read_only variable not honoured here
		// GRC FIXME - index_byte_size must be 4

		num_indices_ = num_indices;
		index_byte_size_ = index_byte_size;
		//		PlatformVita& platform_d3d = static_cast<PlatformVita&>(platform);
		bool success = true;

		indices_ = (uint32_t *)graphicsAlloc(
			SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
			num_indices_*index_byte_size_,
			4,
			SCE_GXM_MEMORY_ATTRIB_READ,
			&indices_uid_);

		switch (index_byte_size_)
		{
		case 2:
			index_format_ = SCE_GXM_INDEX_FORMAT_U16;
			break;
		case 4:
			index_format_ = SCE_GXM_INDEX_FORMAT_U32;
			break;
		}

		memcpy(indices_, indices, num_indices_*index_byte_size_);

		return success;
	}

	void IndexBufferVita::Bind(const Platform& platform) const
	{

	}

	void IndexBufferVita::Unbind(const Platform& platform) const
	{

	}

	bool IndexBufferVita::Update(const Platform& platform)
	{
		return true;
	}

}
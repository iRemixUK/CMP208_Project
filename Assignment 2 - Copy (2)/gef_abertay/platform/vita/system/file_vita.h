#ifndef _ABFW_FILE_VITA_H
#define _ABFW_FILE_VITA_H

#include <system/file.h>
#include <scetypes.h>
#include <fios2.h>

namespace gef
{

class FileVita : public File
{
public:

	FileVita();
	~FileVita();

	bool Open(const char* const filename);
	bool Exists(const char* const filename);

	bool Seek(const SeekFrom seek_from, Int32 offset/*, Int32* position = NULL*/);
	bool Read(void *buffer, const Int32 size, Int32& bytes_read);
	bool Read(void *buffer, const Int32 size, const Int32 offset, Int32& bytes_read);
	bool Close();
	bool GetSize(Int32 &size);




private:
	SceUID file_descriptor_;
	SceFiosFH fios_file_handle_;

	static const SceUID kInvalidHandle;
};

}

#endif // _ABFW_FILE_VITA_H
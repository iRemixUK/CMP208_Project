#include <platform/vita/system/file_vita.h>
#include <kernel.h>
#include <libdbg.h>
#include <cstdio>
#include <string>
#include <cassert>

// Specifies the size of the FIOS read size.
#define FIOS_READ_SIZE (10 * 1024 * 1024)

namespace gef
{
	File* File::Create()
	{
		return new FileVita();
	}


const SceUID FileVita::kInvalidHandle = -1;

FileVita::FileVita() :
file_descriptor_(kInvalidHandle),
fios_file_handle_(kInvalidHandle)
{
}

FileVita::~FileVita()
{
	// make sure file descriptor is invalid
	// if it's not then the file has not been closed properly
	SCE_DBG_ASSERT(fios_file_handle_ == kInvalidHandle);
	SCE_DBG_ASSERT(file_descriptor_ == kInvalidHandle);
}

bool FileVita::Open(const char* const filename)
{
	Int32 result = -1;
	char full_pathname[SCE_IO_MAX_PATH_LENGTH];

	std::snprintf(full_pathname, SCE_IO_MAX_PATH_LENGTH, "app0:/%s", filename);

	// Try to open the file with FIOS2
	{
		SceFiosOpenParams openParams = SCE_FIOS_OPENPARAMS_INITIALIZER;
		openParams.openFlags = SCE_FIOS_O_RDONLY;

		// Try loading the file from an archive
	//		const char *archiveMountPoint = PStreamFilePSP2::GetFIOS2ArchiveMountPoint();
		const char *archiveMountPoint = "/archive/mount/point";
//		const char *archiveMountPoint = NULL;
		if(archiveMountPoint)
		{
			size_t fullLength = strlen(filename) + 1 + strlen(archiveMountPoint) + 1;
			char *fullname = (char *)malloc(fullLength);
			std::snprintf(fullname, fullLength, "%s/%s", archiveMountPoint, filename);
			result = sceFiosFHOpenSync(NULL, &fios_file_handle_, fullname, &openParams);
		}

		// Fall back to loading the file as is
		if(result != SCE_OK)
		{
			result = sceFiosFHOpenSync(NULL, &fios_file_handle_, filename, &openParams);
			if(result != SCE_OK)
				fios_file_handle_ = kInvalidHandle;
		}
	}

	if(result != SCE_OK)
	{
		file_descriptor_ = sceIoOpen(full_pathname, SCE_O_RDONLY, 0);
		result = file_descriptor_ < 0;
	}

	if(result != SCE_OK)
	{
		fios_file_handle_ = kInvalidHandle;
		file_descriptor_ = kInvalidHandle;
		return false;
	}

	return true;
}

bool FileVita::Exists(const char* const filename)
{
	// not yet implmented
	assert(0);
	return true;
}

bool FileVita::Close()
{
	bool success = true;
	if(fios_file_handle_ >= 0)
	{
		sceFiosFHCloseSync(NULL, fios_file_handle_);
		fios_file_handle_ = -1;
		return true;
	}
	else if(file_descriptor_ >= 0)
	{
		if(sceIoClose(file_descriptor_) == SCE_OK)
			file_descriptor_ = kInvalidHandle;
		else
			success = false;
	}

	return success;
}

bool FileVita::GetSize(Int32 &size)
{
	if(fios_file_handle_ > 0)
	{
		size = sceFiosFHGetSize(fios_file_handle_);
	}
	else
	{
		SceIoStat status;
		if( sceIoGetstatByFd(file_descriptor_, &status) < 0 )
			return false;

		size = static_cast<Int32>(status.st_size);
	}

	return true;
}

bool FileVita::Seek(const SeekFrom seek_from, const Int32 offset/*, Int32* position*/)
{
	bool success;
	if(fios_file_handle_ >= 0)
	{
		SceFiosWhence from = SCE_FIOS_SEEK_SET;
		switch(seek_from)
		{
		case SF_Start:
			from = SCE_FIOS_SEEK_SET;
			break;
		case SF_Current:
			from = SCE_FIOS_SEEK_CUR;
			break;
		case SF_End:
			from = SCE_FIOS_SEEK_END;
			break;
		}

		SceFiosOffset newPos = sceFiosFHSeek(fios_file_handle_, offset, from);
		success = newPos >= 0;
	}
	else
	{
		Int32 from = SCE_SEEK_SET;
		switch(seek_from)
		{
		case SF_Start:
			from = SCE_SEEK_SET;
			break;
		case SF_Current:
			from = SCE_SEEK_CUR;
			break;
		case SF_End:
			from = SCE_SEEK_END;
			break;
		}
		SceOff return_value = sceIoLseek(file_descriptor_, offset, from);
		success = return_value >= 0;
	}


//	if(position)
//		*position = static_cast<Int32>(return_value);

	return success;
}

bool FileVita::Read(void *buffer, const Int32 size, Int32& bytes_read)
{
	bool success = true;
	Int32 count = size;
	bytes_read = 0;
	if(fios_file_handle_ >= 0)
	{
		// Iterate on sceFiosFHReadSync - there seems to be a limit on reads related to s_fios2ChunkStorage.
		Int32 bytesReadThisIteration = 1;		// Pretend we read something to get us started.
		while (count && bytesReadThisIteration)
		{
			UInt32 bytesToRead = count < FIOS_READ_SIZE ? count : FIOS_READ_SIZE;
			bytesReadThisIteration = sceFiosFHReadSync(NULL, fios_file_handle_, buffer, bytesToRead);
			if (bytesReadThisIteration < 0)
			{
//				PHYRE_SET_LAST_ERROR((void)0, "Error trying to read %d bytes from a file to %p, (handle 0x%x result: %d 0x%x)", bytesToRead, buffer, m_fiosFH, bytesReadThisIteration, bytesReadThisIteration);
				break;
			}

			SCE_DBG_ASSERT(bytesReadThisIteration <= bytesToRead);

			bytes_read += bytesReadThisIteration;
			buffer = (void*)((UInt32)buffer+ bytesReadThisIteration);
			count -= bytesReadThisIteration;
		}

		if(count != 0)
			success = false;


	}
	else
	{
		SceSSize return_value = sceIoRead(file_descriptor_, buffer, size);
		success =  return_value >= 0;
		if(success)
			bytes_read = return_value;
	}
	return success;
}

bool FileVita::Read(void *buffer, const Int32 size, const Int32 offset, Int32& bytes_read)
{
	SceSSize return_value = sceIoPread(file_descriptor_, buffer, size, offset);

	bytes_read = return_value;
	return return_value < 0 ? false : true;
}

}
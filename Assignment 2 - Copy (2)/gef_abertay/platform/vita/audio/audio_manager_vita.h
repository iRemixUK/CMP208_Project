#ifndef _ABFW_AUDIO_MANAGER_VITA_H
#define _ABFW_AUDIO_MANAGER_VITA_H

#include <audio/audio_manager.h>
#include <scetypes.h>
#include <ngs.h>
#include <vector>
#include <kernel.h>
#include <gef.h>

#define INVALID_SAMPLE_ID (-1)

#define DEFAULT_NUM_SIMULTANEOUS_VOICES		(4)

// Defines and variables for streamed voice (background music)
#define STREAM_BUFFER_SIZE	(4 * 1024)
#define NUM_STREAM_BUFFERS	(2)

// Flags for audio output mode
#define NGS_NO_OUTPUT		(0x00)
#define NGS_SEND_TO_DEVICE	(0x01)
#define NGS_WRITE_TO_FILE	(0x02)

// Flags for audio output config mode
#define NGS_SET_VOLUME      (0x00)

namespace gef
{

class SoundInfo 
{
public:
	SoundInfo();
	~SoundInfo();
	void CleanUp();

	void *pData;
	int  nNumBytes;
	int	 nNumChannels;
	int  nSampleRate;
	int  nNumSamples;
	int  nType;				// SCE_NGS_PLAYER_TYPE_ADPCM or SCE_NGS_PLAYER_TYPE_ADPCM or
							// for ATRAC9, this member holds the configData value
};


struct SamplePlayerCallbackData
{
	class AudioManagerVita* audio_manager;
	UInt32 voice_index;
};

struct MusicPlayerCallbackData
{
	class AudioManagerVita* audio_manager;
};

class AudioManagerVita : public AudioManager
{
public:
	AudioManagerVita(const UInt32 max_simultaneous_voices = DEFAULT_NUM_SIMULTANEOUS_VOICES);
	~AudioManagerVita();


	Int32 LoadSample(const char *strFileName, const Platform& platform);
	Int32 LoadMusic(const char *strFileName, const Platform& platform);
	Int32 PlayMusic();
	Int32 StopMusic();
	Int32 PlaySample(const Int32 sample_index, const bool looping = false);
	Int32 StopPlayingSampleVoice(const Int32 voice_index);
	void UnloadMusic();
	void UnloadSample(Int32 sample_num);
	void UnloadAllSamples();


	Int32 WriteAudioOut( const short *pBuffer );
	Int32 SetSamplePitch(const Int32 voice_index, float pitch);
	Int32 SetMusicPitch(float pitch);
	Int32 GetSampleVoiceVolumeInfo(const Int32 voice_index, struct VolumeInfo& volume_info);
	Int32 SetSampleVoiceVolumeInfo(const Int32 voice_index, const struct VolumeInfo& volume_info);
	Int32 GetMusicVolumeInfo(struct VolumeInfo& volume_info);
	Int32 SetMusicVolumeInfo(const struct VolumeInfo& volume_info);
	Int32 SetMasterVolume(float volume);
	Int32 LockMusicMutex();
	Int32 UnlockMusicMutex();

	inline const SoundInfo& music() { return music_; }
	inline Int32 num_stream_bytes_read() const { return num_stream_bytes_read_; }
	inline void set_num_stream_bytes_read(const Int32 bytes_read) { num_stream_bytes_read_ = bytes_read; }
	inline Int32 current_stream_buffer_index() const { return current_stream_buffer_index_; }
	inline void set_current_stream_buffer_index(const Int32 buffer_index) { current_stream_buffer_index_ = buffer_index; }
	inline SceNgsHVoice& music_voice() { return music_voice_; }
	inline SceNgsHVoice& master_voice() { return master_voice_; }
	inline void set_sample_voice_playing(const UInt32 voice_index, const bool is_playing) { sample_voice_playing_[voice_index] = is_playing; }
	bool sample_voice_playing(const UInt32 voice_index) { return sample_voice_playing_[voice_index]; }
	bool sample_voice_looping(const UInt32 voice_index) { return sample_voice_looping_[voice_index]; }
	inline bool audio_update_thread_running() const { return audio_update_thread_running_; }
	inline const SceNgsHSynSystem& system_handle() const { return system_handle_; }
public:
	int loadWAVFile( const char *strFileName, SoundInfo *pSound );
private:
	Int32 InitNGS();
	Int32 CleanUpNGS();
	int scanRIFFFileForChunk( const char *strChunkId, SceUID uid, SceUInt32 *puChunkSize );

	Int32 StartAudioUpdateThread();
	Int32 StopAudioUpdateThread();

	Int32 CreateRacks();
	Int32 CleanUpRacks();
	Int32 ConnectRacks( SceNgsHVoice hVoiceSource, SceNgsHVoice hVoiceDest, SceNgsHPatch* patch );
	Int32 SetPatchChannelVolumes(SceNgsHPatch hPatchHandle, const struct VolumeInfo& volume_info);


	Int32 GetVoiceHandles();
	Int32 SetupPlayerCallbacks();

	Int32 InitStreamBuffers();
	Int32 CleanUpStreamBuffers();

	Int32 InitPlayerStream( SceNgsHVoice hVoice, SceUInt32 nModuleId, SoundInfo *pSound );
	Int32 InitPlayer( SceNgsHVoice hVoice, SceUInt32 nModuleId, SoundInfo *pSound, const bool looping = false );

	Int32 PrepareAudioOut( Int32 nMode, Int32 nBufferGranularity, Int32 nSampleRate, const char *strFileName );
	void CleanUpAudioOut();
	Int32 ConfigAudioOut( Int32 config_cmd, Int32 flag, Int32 * params );

	Int32 SetPitch(SceNgsHVoice hVoice, float pitch);

	Int32 CreateMutex(SceKernelLwMutexWork* mutex, const char* name);
	Int32 CleanUpMutex(SceKernelLwMutexWork* mutex);
	Int32 LockMutex(SceKernelLwMutexWork* mutex);
	Int32 UnlockMutex(SceKernelLwMutexWork* mutex);



	std::vector<SoundInfo*> samples_;
	SoundInfo music_;
	SceUID audio_update_thread_id_;
	bool   audio_update_thread_running_;

	void* sample_rack_memory_;
	SceNgsHRack sample_rack_;
	void* music_rack_memory_;
	SceNgsHRack music_rack_;
	void* master_rack_memory_;
	SceNgsHRack master_rack_;

	SceNgsHPatch* sample_patch_;
	SceNgsHPatch music_patch_;

	SceNgsHVoice* sample_voices_;
	SceNgsHVoice music_voice_;
	SceNgsHVoice master_voice_;
	SamplePlayerCallbackData* sample_player_callback_data_;
	MusicPlayerCallbackData music_player_callback_data_;

	void             *system_memory_;
	SceNgsHSynSystem system_handle_;

	void *stream_buffers[NUM_STREAM_BUFFERS];
	Int32 num_stream_bytes_read_;
	Int32 current_stream_buffer_index_;

	bool* sample_voice_playing_;
	float* sample_voice_frequency_;
	bool* sample_voice_looping_;
	Int32* sample_voice_sample_num_;
	VolumeInfo* sample_voice_volume_info_;
	bool music_voice_playing_;
	VolumeInfo music_volume_info_;

	UInt32 max_simultaneous_voices_;

	// Variables for audio output (to device and/or file)
	Int32 audio_out_port_id_;
	SceUID audio_out_file_;
	Int32 audio_out_mode_;
	SceSize audio_out_num_bytes_per_update_;

	float master_volume_;

	SceKernelLwMutexWork music_voice_params_mutex_;
};

}

#endif // _ABFW_AUDIO_MANAGER_VITA_H


#include <platform/vita/audio/audio_manager_vita.h>
#include <system/Platform.h>
#include <kernel.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <libsysmodule.h>
#include <audioout.h>
#include <sceerror.h>
#include <libdbg.h>


#define FILE_READ_CHUNK_SIZE	(64 * 1024)
#define AUDIO_UPDATE_THREAD_STACK_SIZE	(8 * 1024)

// NUM_MODULES defines how many unique module types NGS will allow to be loaded
// this number refers to the number of module types, regardless of the number of instances
#define NUM_MODULES		(14)

// SYS_GRANULARITY defines how many samples are produced each update
#define SYS_GRANULARITY	(512)

// SYS_SAMPLE_RATE defines the sample rate for NGS
#define SYS_SAMPLE_RATE	(48000)

static void printParamError( SceNgsHVoice hVoice, SceUInt32 nModuleId )
{
	char str[128];

	sceNgsVoiceGetParamsOutOfRange( hVoice, nModuleId, str );
	printf( "PARAM ERROR is \"%s\"\n", str );
}

static Int32 AudioUpdateThread(SceSize args, void *argc)
{
	Int32   returnCode     = SCE_NGS_OK;
	Int32   nCurrentBuffer = 0;
	Int16 outputData[2][ SYS_GRANULARITY * 2 ];

	printf( "_AudioUpdateThread running\n" );

	gef::AudioManagerVita* audio_manager = *(gef::AudioManagerVita**)argc;

	while ( (volatile bool) audio_manager->audio_update_thread_running() )
	{
//		++s_nTickCount;

		// Perform NGS system update
		returnCode = sceNgsSystemUpdate( audio_manager->system_handle() );
		if ( returnCode != SCE_NGS_OK ) {
			printf( "_AudioUpdateThread: sceNgsSystemUpdate() failed: 0x%08X\n", returnCode );
			break;
		}

		// Get output data
		returnCode = sceNgsVoiceGetStateData(	audio_manager->master_voice(), SCE_NGS_MASTER_BUSS_OUTPUT_MODULE,
												outputData[nCurrentBuffer], sizeof(short) * SYS_GRANULARITY * 2 );
		if ( returnCode != SCE_NGS_OK ) {
			printf( "_AudioUpdateThread: sceNgsVoiceGetStateData() failed: 0x%08X\n", returnCode );
			break;
		}

		// Output audio to device and/or file
		returnCode = audio_manager->WriteAudioOut( outputData[nCurrentBuffer] );

		// Switch buffer
		nCurrentBuffer ^= 1;
	}

	printf( "_AudioUpdateThread exiting\n" );
	sceKernelExitDeleteThread( returnCode );

	return returnCode;

}

static void PlayerCallback( const SceNgsCallbackInfo *pCallbackInfo )
{
	gef::AudioManagerVita* audio_manager = static_cast<gef::SamplePlayerCallbackData*>(pCallbackInfo->pUserData)->audio_manager;
	UInt32 voice_index = static_cast<gef::SamplePlayerCallbackData*>(pCallbackInfo->pUserData)->voice_index;
	
	if(!audio_manager->sample_voice_looping(voice_index))
		audio_manager->set_sample_voice_playing(voice_index, false);
}

static void StreamCallback( const SceNgsCallbackInfo *pCallbackInfo )
{
	Int32                returnCode;
	SceNgsBufferInfo   buffInfo;
	SceNgsPlayerParams *pPcmParams;
	UInt32       uBytesToRead;

	gef::AudioManagerVita* audio_manager = static_cast<gef::MusicPlayerCallbackData*>(pCallbackInfo->pUserData)->audio_manager;

	returnCode = audio_manager->LockMusicMutex();
	if ( returnCode != SCE_NGS_OK )
	{
		printf( "streamCallback: LockMusicMutex() failed: 0x%08X\n", returnCode );
		return;
	}

	// Determine how many bytes to copy
	if ( audio_manager->music().nNumBytes - audio_manager->num_stream_bytes_read() < STREAM_BUFFER_SIZE ) {
		uBytesToRead = audio_manager->music().nNumBytes - audio_manager->num_stream_bytes_read();
	} else {
		uBytesToRead = STREAM_BUFFER_SIZE;
	}


	// Get player parameters
	returnCode = sceNgsVoiceLockParams(	audio_manager->music_voice(),
										SCE_NGS_VOICE_T1_PCM_PLAYER,
										SCE_NGS_PLAYER_PARAMS_STRUCT_ID,
										&buffInfo );
	if ( returnCode != SCE_NGS_OK )
	{
		printf( "streamCallback: sceNgsVoiceLockParams() failed: 0x%08X\n", returnCode );
		return;
	}
	pPcmParams = (SceNgsPlayerParams *)buffInfo.data;


	// Copy data
	memcpy( (void *)pPcmParams->buffs[audio_manager->current_stream_buffer_index()].pBuffer, (char *)audio_manager->music().pData + audio_manager->num_stream_bytes_read(), uBytesToRead );

	if( uBytesToRead < STREAM_BUFFER_SIZE ) {
		// reached the end of the file, wrap around
		memcpy( (char*)pPcmParams->buffs[audio_manager->current_stream_buffer_index()].pBuffer + uBytesToRead, (char*)audio_manager->music().pData, STREAM_BUFFER_SIZE - uBytesToRead );
		uBytesToRead = STREAM_BUFFER_SIZE;
	}

	audio_manager->set_current_stream_buffer_index((audio_manager->current_stream_buffer_index() + 1) % 2);
	audio_manager->set_num_stream_bytes_read(audio_manager->num_stream_bytes_read()+uBytesToRead);
	if ( audio_manager->num_stream_bytes_read() >= audio_manager->music().nNumBytes )
	{
		audio_manager->set_num_stream_bytes_read(audio_manager->num_stream_bytes_read() - audio_manager->music().nNumBytes);
	}


	// Update player parameters
	returnCode = sceNgsVoiceUnlockParams( audio_manager->music_voice(), SCE_NGS_VOICE_T1_PCM_PLAYER );
	if ( returnCode != SCE_NGS_OK ) {
		printf( "streamCallback: sceNgsVoiceUnlockParams() failed: 0x%08X\n", returnCode );
		if ( returnCode == SCE_NGS_ERROR_PARAM_OUT_OF_RANGE )
		{
			printParamError( audio_manager->music_voice(), SCE_NGS_VOICE_T1_PCM_PLAYER );
		}
		return;
	}

	returnCode = audio_manager->UnlockMusicMutex();
	if ( returnCode != SCE_NGS_OK )
	{
		printf( "streamCallback: LockMusicMutex() failed: 0x%08X\n", returnCode );
		return;
	}
}

namespace gef
{

	AudioManager* AudioManager::Create()
	{
		return new AudioManagerVita();
	}


SoundInfo::SoundInfo() :
	pData(NULL),
		nNumBytes(0),
		nNumChannels(0),
		nSampleRate(0),
		nNumSamples(0),
		nType(0)

{
}

SoundInfo::~SoundInfo()
{
	CleanUp();
}

void SoundInfo::CleanUp()
{
	free(pData);
	pData = NULL;
	nNumBytes = 0;
	nNumChannels = 0;
	nSampleRate = 0;
	nType = 0;
}




AudioManagerVita::AudioManagerVita(const UInt32 max_simultaneous_voices) :
audio_update_thread_running_(false),
sample_voices_(NULL),
sample_player_callback_data_(NULL),
sample_voice_playing_(NULL),
sample_voice_looping_(NULL),
sample_voice_volume_info_(NULL),
sample_voice_frequency_(NULL),
system_memory_(NULL),
sample_rack_memory_(NULL),
music_rack_memory_(NULL),
master_rack_memory_(NULL),
sample_patch_(NULL),
music_voice_playing_(false),
max_simultaneous_voices_(max_simultaneous_voices),
current_stream_buffer_index_(0)
{
	for(Int32 i=0; i < NUM_STREAM_BUFFERS; ++i)
		stream_buffers[i] = NULL;


	sample_voices_ = new SceNgsHVoice[max_simultaneous_voices_];
	sample_player_callback_data_ = new SamplePlayerCallbackData[max_simultaneous_voices_];
	sample_voice_playing_ = new bool[max_simultaneous_voices_];
	sample_voice_looping_ = new bool[max_simultaneous_voices_];
	sample_voice_volume_info_ = new VolumeInfo[max_simultaneous_voices_],
	sample_voice_frequency_ = new float[max_simultaneous_voices_];
	sample_patch_ = new SceNgsHPatch[max_simultaneous_voices_];
	sample_voice_sample_num_ = new Int32[max_simultaneous_voices_];
	for ( Int32 i=0; i < max_simultaneous_voices_; ++i )
	{
		sample_voice_looping_[i] = false;
		sample_voice_playing_[i] = false;
		sample_voice_sample_num_[i] = -1;
	}

	Int32 return_value = SCE_NGS_OK;

	return_value = InitNGS();
	return_value = CreateRacks();
	return_value = GetVoiceHandles();
	return_value = InitStreamBuffers();
	return_value = SetupPlayerCallbacks();
	return_value = CreateMutex(&music_voice_params_mutex_, "music_voice_params_mutex");

	// Connect racks
	for ( int i=0; i < max_simultaneous_voices_; ++i )
	{
		return_value = ConnectRacks( sample_voices_[i], master_voice_, &sample_patch_[i] );
	}
	return_value = ConnectRacks( music_voice_, master_voice_, &music_patch_ );


	// Play voices
	return_value = sceNgsVoicePlay( master_voice_ );

	return_value = PrepareAudioOut( NGS_SEND_TO_DEVICE, SYS_GRANULARITY, SYS_SAMPLE_RATE, NULL );

	// once everthing else has been initialised then start
	// the audio thread
	return_value = StartAudioUpdateThread();

	SCE_DBG_ASSERT(return_value == SCE_NGS_OK);
}


AudioManagerVita::~AudioManagerVita(void)
{
	StopAudioUpdateThread();

	CleanUpAudioOut();
	CleanUpMutex(&music_voice_params_mutex_);
	CleanUpStreamBuffers();
	CleanUpRacks();
	CleanUpNGS();

	// clean up loaded sample data
	for ( std::vector<SoundInfo*>::iterator sample_iter =  samples_.begin(); sample_iter != samples_.end();++sample_iter)
		delete *sample_iter;

	// music doesn't need explict call to clean it up as it is not dynamically allocated

	delete[] sample_voices_;
	delete[] sample_player_callback_data_;
	delete[] sample_voice_playing_;
	delete[] sample_voice_looping_;
	delete[] sample_voice_volume_info_;
	delete[] sample_voice_frequency_;
}

Int32 AudioManagerVita::InitNGS()
{
	Int32                    return_value = SCE_NGS_OK;
	SceNgsSystemInitParams init_params;
	size_t                 size;

	// initialize NGS lib
	return_value = sceSysmoduleLoadModule( SCE_SYSMODULE_NGS );
	if ( return_value != SCE_NGS_OK ) {
		printf( "initNGS: sceSysmoduleLoadModule(NGS) failed: 0x%08X\n", return_value );
		return return_value;
	}

	init_params.nMaxRacks    = 3;
	init_params.nMaxVoices   = max_simultaneous_voices_ + 2;
	init_params.nGranularity = SYS_GRANULARITY;
	init_params.nSampleRate  = SYS_SAMPLE_RATE;
	init_params.nMaxModules  = NUM_MODULES;

	// Determine memory requirement
	return_value = sceNgsSystemGetRequiredMemorySize( &init_params, &size );
	if ( return_value != SCE_NGS_OK ) {
		printf( "initNGS: sceNgsSystemGetRequiredMemorySize() failed: 0x%08X\n", return_value );
		return return_value;
	}

	// Allocate memory
	system_memory_ = memalign( SCE_NGS_MEMORY_ALIGN_SIZE, size );
	if ( system_memory_ == NULL )
	{
		printf( "initNGS: malloc() failed for %d bytes\n", size );
		return SCE_ERROR_ERRNO_ENOMEM;
	}

	// Initialise NGS and get system handle
	return_value = sceNgsSystemInit( system_memory_, size, &init_params, &system_handle_ );
	if ( return_value != SCE_NGS_OK )
	{
		printf( "initNGS: sceNgsSystemInit() failed: 0x%08X\n", return_value );
		return return_value;
	}
	printf( "...sceNgsSystemInit() OK\n" );

	return return_value;
}

Int32 AudioManagerVita::CleanUpNGS()
{
	Int32                    return_value = SCE_NGS_OK;

	// Shutdown NGS
	return_value = sceNgsSystemRelease( system_handle_ );
	if ( return_value != SCE_NGS_OK ) {
		printf( "shutdown: sceNgsSystemRelease() failed: 0x%08X\n", return_value );
		return return_value;
	}
	sceSysmoduleUnloadModule( SCE_SYSMODULE_NGS );

	free( system_memory_ );
	system_memory_ = NULL;

	return return_value;
}

Int32 AudioManagerVita::CreateRacks()
{
	Int32 return_value;

	const struct SceNgsVoiceDefinition *pT1VoiceDef;
	const struct SceNgsVoiceDefinition *pMasterBussVoiceDef;
	SceNgsRackDescription rackDesc;
	SceNgsBufferInfo      bufferInfo;

	// Get voice definitions
	pT1VoiceDef         = sceNgsVoiceDefGetTemplate1();
	pMasterBussVoiceDef = sceNgsVoiceDefGetMasterBuss();

	// Determine memory requirement for one-shot sounds rack
	rackDesc.nChannelsPerVoice   = 1;
	rackDesc.nVoices             = max_simultaneous_voices_;
	rackDesc.pVoiceDefn          = pT1VoiceDef;
	rackDesc.nMaxPatchesPerInput = 0;
	rackDesc.nPatchesPerOutput   = 1;

	return_value = sceNgsRackGetRequiredMemorySize( system_handle_, &rackDesc, &bufferInfo.size);
	if ( return_value != SCE_NGS_OK ) {
		printf( "createRacks: sceNgsRackGetRequiredMemorySize(one-shot) failed: 0x%08X\n", return_value );
		return return_value;
	}

	// Allocate memory for one-shot sounds rack
	sample_rack_memory_ = memalign( SCE_NGS_MEMORY_ALIGN_SIZE, bufferInfo.size );
	if ( sample_rack_memory_ == NULL ) {
		printf( "createRacks: malloc(one-shot_rack) failed for %d bytes\n", bufferInfo.size );
		return SCE_ERROR_ERRNO_ENOMEM;
	}
	memset( sample_rack_memory_, 0, bufferInfo.size );

	// Initialise one-shot sounds rack
	bufferInfo.data = sample_rack_memory_;
	return_value = sceNgsRackInit( system_handle_, &bufferInfo, &rackDesc, &sample_rack_ );
	if ( return_value != SCE_NGS_OK ) {
		printf( "createRacks: sceNgsRackInit(one-shot) failed: 0x%08X\n", return_value );
		return return_value;
	}


	// Determine memory requirement for music rack
	rackDesc.nChannelsPerVoice   = 2;
	rackDesc.nVoices             = 1;
	rackDesc.pVoiceDefn          = pT1VoiceDef;
	rackDesc.nMaxPatchesPerInput = 0;
	rackDesc.nPatchesPerOutput   = 1;

	return_value = sceNgsRackGetRequiredMemorySize( system_handle_, &rackDesc, &bufferInfo.size);
	if ( return_value != SCE_NGS_OK ) {
		printf( "createRacks: sceNgsRackGetRequiredMemorySize(music) failed: 0x%08X\n", return_value );
		return return_value;
	}

	// Allocate memory for music rack
	music_rack_memory_ = memalign( SCE_NGS_MEMORY_ALIGN_SIZE, bufferInfo.size );
	if ( music_rack_memory_ == NULL ) {
		printf( "createRacks: malloc(music_rack) failed for %d bytes\n", bufferInfo.size );
		return SCE_ERROR_ERRNO_ENOMEM;
	}
	memset( music_rack_memory_, 0, bufferInfo.size );

	// Initialise music rack
	bufferInfo.data = music_rack_memory_;
	return_value = sceNgsRackInit( system_handle_, &bufferInfo, &rackDesc, &music_rack_ );
	if ( return_value != SCE_NGS_OK ) {
		printf( "createRacks: sceNgsRackInit(music) failed: 0x%08X\n", return_value );
		return return_value;
	}


	// Determine memory requirement for master rack
	rackDesc.nChannelsPerVoice      = 2;
	rackDesc.nVoices                = 1;
	rackDesc.pVoiceDefn             = pMasterBussVoiceDef;
	rackDesc.nMaxPatchesPerInput    = max_simultaneous_voices_ + 1;
	rackDesc.nPatchesPerOutput      = 0;

	return_value = sceNgsRackGetRequiredMemorySize( system_handle_, &rackDesc, &bufferInfo.size);
	if ( return_value != SCE_NGS_OK ) {
		printf( "createRacks: sceNgsRackGetRequiredMemorySize(master) failed: 0x%08X\n", return_value );
		return return_value;
	}

	// Allocate memory for master rack
	master_rack_memory_ = memalign( SCE_NGS_MEMORY_ALIGN_SIZE, bufferInfo.size );
	if ( master_rack_memory_ == NULL ) {
		printf( "createRacks: malloc(master_rack) failed for %d bytes\n", bufferInfo.size );
		return SCE_ERROR_ERRNO_ENOMEM;
	}
	memset( master_rack_memory_, 0, bufferInfo.size );

	// Initialise master rack
	bufferInfo.data = master_rack_memory_;
	return_value = sceNgsRackInit( system_handle_, &bufferInfo, &rackDesc, &master_rack_ );
	if ( return_value != SCE_NGS_OK ) {
		printf( "createRacks: sceNgsRackInit(master) failed: 0x%08X\n", return_value );
		return return_value;
	}

	return SCE_NGS_OK;
}

Int32 AudioManagerVita::CleanUpRacks()
{
	Int32 return_value = SCE_NGS_OK;

	free(sample_rack_memory_);
	free(music_rack_memory_);
	free(master_rack_memory_);

	return return_value;
}

Int32 AudioManagerVita::GetVoiceHandles()
{
	Int32 return_value = SCE_NGS_OK;

	// Get voice handles
	for ( Int32 i=0; i < max_simultaneous_voices_; ++i ) {
		return_value = sceNgsRackGetVoiceHandle( sample_rack_, i, &sample_voices_[i] );
		if ( return_value != SCE_NGS_OK ) {
			printf( "init: sceNgsRackGetVoiceHandle(sample %d) failed: 0x%08X\n", i, return_value );
			return return_value;
		}
	}

	return_value = sceNgsRackGetVoiceHandle( music_rack_, 0, &music_voice_ );
	if ( return_value != SCE_NGS_OK ) {
		printf( "init: sceNgsRackGetVoiceHandle(music) failed: 0x%08X\n", return_value );
		return return_value;
	}

	return_value = sceNgsRackGetVoiceHandle( master_rack_, 0, &master_voice_ );
	if ( return_value != SCE_NGS_OK ) {
		printf( "init: sceNgsRackGetVoiceHandle(master) failed: 0x%08X\n", return_value );
		return return_value;
	}
	return return_value;
}

Int32 AudioManagerVita::SetupPlayerCallbacks()
{
	Int32 return_value = SCE_NGS_OK;

	// Setup player callbacks
	for ( UInt32 i=0; i < max_simultaneous_voices_; ++i )
	{
		SamplePlayerCallbackData* pPlayerCallbackData = &sample_player_callback_data_[i];
		pPlayerCallbackData->audio_manager = this;
		pPlayerCallbackData->voice_index = i;
		return_value = sceNgsVoiceSetModuleCallback( sample_voices_[i], SCE_NGS_VOICE_T1_PCM_PLAYER, PlayerCallback, (void *)pPlayerCallbackData );
		if ( return_value != SCE_NGS_OK ) {
			printf( "init: sceNgsVoiceSetModuleCallback(voice %d) failed: 0x%08X\n", i, return_value );
			return return_value;
		}
	}
	music_player_callback_data_.audio_manager = this;
	return_value = sceNgsVoiceSetModuleCallback( music_voice_, SCE_NGS_VOICE_T1_PCM_PLAYER, StreamCallback, &music_player_callback_data_ );
	if ( return_value != SCE_NGS_OK ) {
		printf( "init: sceNgsVoiceSetModuleCallback(music) failed: 0x%08X\n", return_value );
		return return_value;
	}
	printf( "....set Player Callbacks\n" );

	return return_value;
}

Int32 AudioManagerVita::InitStreamBuffers()
{
	Int32 return_value = SCE_NGS_OK;

	// Allocate memory for stream buffers
	for ( Int32 i = 0; i < NUM_STREAM_BUFFERS; ++i ) {
		stream_buffers[i] = malloc( STREAM_BUFFER_SIZE );
		if ( stream_buffers[i] == NULL ) {
			printf( "initPlayerStream: malloc(stream buffer %d) failed for %d bytes\n", i, STREAM_BUFFER_SIZE );
			return SCE_ERROR_ERRNO_ENOMEM;
		}
	}

	return return_value;
}


Int32 AudioManagerVita::CleanUpStreamBuffers()
{
	for ( Int32 i = 0; i < NUM_STREAM_BUFFERS; ++i ) {
		free(stream_buffers[i]);
		stream_buffers[i] = NULL;
	}

	return SCE_NGS_OK;
}

Int32 AudioManagerVita::InitPlayerStream( SceNgsHVoice hVoice, SceUInt32 nModuleId, SoundInfo *pSound )
{
	Int32                returnCode;
	SceNgsBufferInfo   bufferInfo;
	SceNgsPlayerParams *pPcmParams;

	memcpy( stream_buffers[0], pSound->pData, STREAM_BUFFER_SIZE );
	memcpy( stream_buffers[1], (char *)pSound->pData + STREAM_BUFFER_SIZE, STREAM_BUFFER_SIZE );
	num_stream_bytes_read_ = 2 * STREAM_BUFFER_SIZE;

	// Get player parameters
	returnCode = sceNgsVoiceLockParams( hVoice, nModuleId, SCE_NGS_PLAYER_PARAMS_STRUCT_ID, &bufferInfo );
	if ( returnCode != SCE_NGS_OK )	{
		printf( "initPlayerStream: sceNgsVoiceLockParams() failed: 0x%08X\n", returnCode );
		return returnCode;
	}

	// Set player parameters
	memset( bufferInfo.data, 0, bufferInfo.size);
	pPcmParams = (SceNgsPlayerParams *)bufferInfo.data;
	pPcmParams->desc.id     = SCE_NGS_PLAYER_PARAMS_STRUCT_ID;
	pPcmParams->desc.size   = sizeof(SceNgsPlayerParams);

	pPcmParams->fPlaybackFrequency          = (SceFloat32)pSound->nSampleRate;
	pPcmParams->fPlaybackScalar             = 1.0f;
	pPcmParams->nLeadInSamples              = 0;
	pPcmParams->nLimitNumberOfSamplesPlayed = 0;
	pPcmParams->nChannels                   = pSound->nNumChannels;
	if ( pSound->nNumChannels == 1 ) {
		pPcmParams->nChannelMap[0] = SCE_NGS_PLAYER_LEFT_CHANNEL;
		pPcmParams->nChannelMap[1] = SCE_NGS_PLAYER_LEFT_CHANNEL;
	} else {
		pPcmParams->nChannelMap[0] = SCE_NGS_PLAYER_LEFT_CHANNEL;
		pPcmParams->nChannelMap[1] = SCE_NGS_PLAYER_RIGHT_CHANNEL;
	}
	pPcmParams->nType            = pSound->nType;
	pPcmParams->nStartBuffer     = 0;
	pPcmParams->nStartByte       = 0;

	pPcmParams->buffs[0].pBuffer    = stream_buffers[0];
	pPcmParams->buffs[0].nNumBytes  = STREAM_BUFFER_SIZE;
	pPcmParams->buffs[0].nLoopCount = 0;
	pPcmParams->buffs[0].nNextBuff  = 1;
	pPcmParams->buffs[1].pBuffer    = stream_buffers[1];
	pPcmParams->buffs[1].nNumBytes  = STREAM_BUFFER_SIZE;
	pPcmParams->buffs[1].nLoopCount = 0;
	pPcmParams->buffs[1].nNextBuff  = 0;

	// Update player parameters
	returnCode = sceNgsVoiceUnlockParams( hVoice, nModuleId );
	if ( returnCode != SCE_NGS_OK ) {
		printf( "initPlayerStream: sceNgsVoiceUnlockParams() failed: 0x%08X\n", returnCode );
		if ( returnCode == SCE_NGS_ERROR_PARAM_OUT_OF_RANGE ) {
			printParamError( hVoice, nModuleId );
		}
		return returnCode;
	}

	return SCE_NGS_OK;
}

Int32 AudioManagerVita::InitPlayer( SceNgsHVoice hVoice, SceUInt32 nModuleId, SoundInfo *pSound, const bool looping )
{
	Int32                returnCode;
	SceNgsBufferInfo   bufferInfo;
	SceNgsPlayerParams *pPcmParams;

	// Get player parameters
	returnCode = sceNgsVoiceLockParams( hVoice, nModuleId, SCE_NGS_PLAYER_PARAMS_STRUCT_ID, &bufferInfo );
	if ( returnCode != SCE_NGS_OK ) {
		printf( "initPlayer: sceNgsVoiceLockParams() failed: 0x%08X\n", returnCode );
		return returnCode;
	}

	// Set player parameters
	memset( bufferInfo.data, 0, bufferInfo.size);
	pPcmParams = (SceNgsPlayerParams *)bufferInfo.data;
	pPcmParams->desc.id     = SCE_NGS_PLAYER_PARAMS_STRUCT_ID;
	pPcmParams->desc.size   = sizeof(SceNgsPlayerParams);

	pPcmParams->fPlaybackFrequency          = (SceFloat32)pSound->nSampleRate;
	pPcmParams->fPlaybackScalar             = 1.0f;
	pPcmParams->nLeadInSamples              = 0;
	pPcmParams->nLimitNumberOfSamplesPlayed = 0;
	pPcmParams->nChannels                   = pSound->nNumChannels;
	if ( pSound->nNumChannels == 1 ) {
		pPcmParams->nChannelMap[0] = SCE_NGS_PLAYER_LEFT_CHANNEL;
		pPcmParams->nChannelMap[1] = SCE_NGS_PLAYER_LEFT_CHANNEL;
	} else {
		pPcmParams->nChannelMap[0] = SCE_NGS_PLAYER_LEFT_CHANNEL;
		pPcmParams->nChannelMap[1] = SCE_NGS_PLAYER_RIGHT_CHANNEL;
	}
	pPcmParams->nType            = pSound->nType;
	pPcmParams->nStartBuffer     = 0;
	pPcmParams->nStartByte       = 0;

	pPcmParams->buffs[0].pBuffer    = pSound->pData;
	pPcmParams->buffs[0].nNumBytes  = pSound->nNumBytes;
	pPcmParams->buffs[0].nLoopCount = looping ? SCE_NGS_PLAYER_LOOP_CONTINUOUS : 0;
	pPcmParams->buffs[0].nNextBuff  = SCE_NGS_PLAYER_NO_NEXT_BUFFER;

	// Update player parameters
	returnCode = sceNgsVoiceUnlockParams( hVoice, nModuleId );
	if ( returnCode != SCE_NGS_OK ) {
		printf( "initPlayer: sceNgsVoiceUnlockParams() failed: 0x%08X\n", returnCode );
		if ( returnCode == SCE_NGS_ERROR_PARAM_OUT_OF_RANGE ) {
			printParamError( hVoice, nModuleId );
		}
		return returnCode;
	}

	return SCE_NGS_OK;
}

Int32 AudioManagerVita::StartAudioUpdateThread()
{
	Int32 return_value;

	audio_update_thread_id_ = sceKernelCreateThread(	"Audio Update Thread", AudioUpdateThread,
													SCE_KERNEL_HIGHEST_PRIORITY_USER,
													AUDIO_UPDATE_THREAD_STACK_SIZE, 0,
													SCE_KERNEL_CPU_MASK_USER_2, SCE_NULL );
	if ( audio_update_thread_id_ < 0 ) {
		printf( "startAudioUpdateThread: sceKernelCreateThread() failed: %08x\n", audio_update_thread_id_ );
		return audio_update_thread_id_;
	}

	AudioManagerVita* audio_manager = this;
	return_value = sceKernelStartThread( audio_update_thread_id_, sizeof(audio_manager), &audio_manager );
	if ( return_value < 0 ) {
		printf( "startAudioUpdateThread: sceKernelStartThread() failed: %08x\n", return_value );
		return return_value;
	}
	audio_update_thread_running_ = true;
	printf( "startAudioUpdateThread: started AudioUpdateThread\n" );


	return SCE_NGS_OK;
}

Int32 AudioManagerVita::StopAudioUpdateThread()
{
	Int32 return_value = SCE_NGS_OK;
	SceInt32 n32ThreadExitStatus;

	// Stop audio update thread
	audio_update_thread_running_ = false;
	return_value = sceKernelWaitThreadEnd( audio_update_thread_id_, &n32ThreadExitStatus, SCE_NULL );
	if ( return_value != SCE_NGS_OK ) {
		printf( "shutdown: sceKernelWaitThreadEnd() failed: 0x%08X\n", return_value );
		return return_value;
	}
	printf( "shutdown: _AudioUpdateThread exit'd (exit status = %d)\n", n32ThreadExitStatus );
	return return_value;
}


Int32 AudioManagerVita::LoadSample(const char *strFileName, const Platform& platform)
{
	SoundInfo* sound_info = new SoundInfo();
	std::string full_pathname = platform.FormatFilename(strFileName);

	if(loadWAVFile(full_pathname.c_str(), sound_info) == SCE_NGS_OK)
	{
		samples_.push_back(sound_info);
		return samples_.size()-1;
	}
	else
	{
		delete sound_info;
		return INVALID_SAMPLE_ID;
	}
}

Int32 AudioManagerVita::loadWAVFile( const char *strFileName, SoundInfo *pSound )
{
	Int32       return_value = SCE_NGS_OK;
	SceUID    uid;
	char      pReadBuffer[8];
	SceUInt32 uChunkSize;
	SceSize   szBytesLeft;
	char      *pCurrentPos;

	uid = sceIoOpen( strFileName, SCE_O_RDONLY, 0 );
	if ( uid < 0 ) {
		printf( "loadWAVFile: sceIoOpen() failed: 0x%08X\n", uid );
		return uid;
	}

	/* Check file is a RIFF file */
	return_value = sceIoRead( uid, pReadBuffer, 4 );
	if ( return_value != 4 ) {
		printf( "loadWAVFile: sceIoRead() failed: 0x%08X\n", return_value );
		sceIoClose( uid );
		return ( ( return_value < 0 ) ? return_value : SCE_ERROR_ERRNO_ENOMEM );
	} else if ( strncmp( pReadBuffer, "RIFF", 4 ) != 0 ) {
		printf( "loadWAVFile: invalid file format - not RIFF\n" );
		sceIoClose( uid );
		return -1;
	}

	/* Check file is a WAV file */
	return_value = sceIoRead( uid, pReadBuffer, 8 );
	if ( return_value != 8 ) {
		printf( "loadWAVFile: sceIoRead() failed: 0x%08X\n", return_value );
		sceIoClose( uid );
		return -1;
	} else if ( strncmp( &pReadBuffer[4], "WAVE", 4 ) != 0 ) {
		printf( "loadWAVFile: invalid file format - not WAVE\n" );
		sceIoClose( uid );
		return -1;
	}

	/* Scan file for "fmt " chunk */
	return_value = scanRIFFFileForChunk( "fmt ", uid, &uChunkSize );
	if ( return_value != SCE_NGS_OK ) {
		printf( "loadWAVFile: invalid file format - no fmt chunk\n" );
		sceIoClose( uid );
		return -1;
	}

	/* Found "fmt " chunk, read 8 bytes (#channels is read+2, sample rate is read+4) */
	return_value = sceIoRead( uid, pReadBuffer, 8 );
	if ( return_value != 8 ) {
		printf( "loadWAVFile: sceIoRead() failed: 0x%08X\n", return_value );
		sceIoClose( uid );
		return -1;
	}

	pSound->nNumChannels = *( (SceInt16 *)(void *)(pReadBuffer + 2) );
	if ( (pSound->nNumChannels != 0x01) && (pSound->nNumChannels != 0x02) ) {
		printf( "loadWAVFile: invalid file format - number of channels\n" );
		sceIoClose( uid );
		return -1;
	}
	pSound->nSampleRate = *( (SceUInt32 *)(void *)(pReadBuffer + 4) );


	/* Return to start of chunks, and scan for "data" chunk */
	return_value = sceIoLseek( uid, 12, SCE_SEEK_SET );
	if ( return_value < 0 ) {
		printf( "loadWAVFile: sceIoLseek() failed: 0x%08X\n", return_value );
		sceIoClose( uid );
		return -1;
	}
	return_value = scanRIFFFileForChunk( "data", uid, &uChunkSize );
	if ( return_value != SCE_NGS_OK ) {
		printf( "loadWAVFile: invalid file format - no data chunk\n" );
		sceIoClose( uid );
		return -1;
	}

	/* Found "data" chunk, allocate memory and read PCM data */
	pSound->nNumBytes = uChunkSize;
	pSound->pData = malloc( pSound->nNumBytes );
	if ( pSound->pData == NULL ) {
		printf( "loadWAVFile: malloc() failed for %d bytes\n", pSound->nNumBytes );
		sceIoClose( uid );
		return SCE_ERROR_ERRNO_ENOMEM;
	}

	/* Read file */
	pCurrentPos = static_cast<char*>(pSound->pData);
	szBytesLeft = uChunkSize;
	while ( szBytesLeft > FILE_READ_CHUNK_SIZE ) {
		return_value = sceIoRead( uid, pCurrentPos, FILE_READ_CHUNK_SIZE );
		if ( return_value != FILE_READ_CHUNK_SIZE ) {
			printf( "loadWAVFile: sceIoRead(data chunk) failed: 0x%08X\n", return_value );
			sceIoClose( uid );
			return ( ( return_value < 0 ) ? return_value : SCE_ERROR_ERRNO_ENOMEM );
		}
		pCurrentPos += FILE_READ_CHUNK_SIZE;
		szBytesLeft -= FILE_READ_CHUNK_SIZE;
	}
	if ( szBytesLeft > 0 ) {
		return_value = sceIoRead( uid, pCurrentPos, szBytesLeft );
		if ( return_value != szBytesLeft ) {
			printf( "loadWAVFile: sceIoRead(data chunk-end) failed: 0x%08X\n", return_value );
			sceIoClose( uid );
			return ( ( return_value < 0 ) ? return_value : SCE_ERROR_ERRNO_ENOMEM );
		}
	}
	sceIoClose(uid);

	pSound->nType = SCE_NGS_PLAYER_TYPE_PCM;

	return SCE_NGS_OK;
}

Int32 AudioManagerVita::scanRIFFFileForChunk( const char *strChunkId, SceUID uid, SceUInt32 *puChunkSize )
{
	Int32       return_value;
	bool      bFoundChunk = false;
	union {
		char      c[4];
		SceUInt32 ui;
	} readBuffer;


	*puChunkSize = 0;
	while ( !bFoundChunk ) {
		return_value = sceIoLseek( uid, *puChunkSize, SEEK_CUR );
		if ( return_value < 0 ) {
			printf( "scanRIFFFileForChunk: sceIoLseek() failed: 0x%08X\n", return_value );
			sceIoClose( uid );
			return -1;
		}
		return_value = sceIoRead( uid, readBuffer.c, 4 );
		if ( return_value != 4 ) {
			printf( "scanRIFFFileForChunk: sceIoRead() failed: 0x%08X\n", return_value );
			sceIoClose( uid );
			return -1;
		}

		bFoundChunk = ( strncmp( readBuffer.c, strChunkId, 4 ) == 0 );

		return_value = sceIoRead( uid, readBuffer.c, 4 );
		if ( return_value != 4 ) {
			printf( "scanRIFFFileForChunk: sceIoRead() failed: 0x%08X\n", return_value );
			sceIoClose( uid );
			return -1;
		}
		*puChunkSize = readBuffer.ui;
	}

	return SCE_NGS_OK;
}

/********************************************************************************************/

Int32 AudioManagerVita::PrepareAudioOut( Int32 nMode, Int32 nBufferGranularity, Int32 nSampleRate, const char *strFileName )
{
	if ( nMode & NGS_SEND_TO_DEVICE ) {
		audio_out_port_id_ = sceAudioOutOpenPort(	SCE_AUDIO_OUT_PORT_TYPE_MAIN,
													nBufferGranularity,						//grain size
													nSampleRate,							//output frequency
													SCE_AUDIO_OUT_PARAM_FORMAT_S16_STEREO);	//format

		if ( audio_out_port_id_ < 0 ) {
			printf( "prepareAudioOut: sceAudioOutOpenPort() failed: 0x%08X\n", audio_out_port_id_ );
			return audio_out_port_id_;
		}

		Int32 volume[2] = { SCE_AUDIO_VOLUME_0dB, SCE_AUDIO_VOLUME_0dB };
		sceAudioOutSetVolume( audio_out_port_id_, (SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH), volume );

		printf( "prepareAudioOut: sending output to audio port %d\n", audio_out_port_id_ );
	}

	// Open file for output
	if ( nMode & NGS_WRITE_TO_FILE ) {
		if ( !strFileName ) {
			nMode = nMode & NGS_SEND_TO_DEVICE;
		} else {
			audio_out_file_ = sceIoOpen( strFileName, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, SCE_STM_RWU );
			if ( audio_out_file_ < 0 ) {
				printf( "prepareAudioOut: sceIoOpen() failed: 0x%08X\n", audio_out_file_ );
				return audio_out_file_;
			}
			printf( "prepareAudioOut: writing output to \"%s\"\n", strFileName );
		}
	}
	audio_out_mode_      = nMode;
	audio_out_num_bytes_per_update_ = sizeof(short) * nBufferGranularity * 2;

	return SCE_NGS_OK;
}

Int32 AudioManagerVita::WriteAudioOut( const short *pBuffer )
{
	Int32 returnCode;

	// Send audio to output port
	if ( audio_out_mode_ & NGS_SEND_TO_DEVICE ) {
		returnCode = sceAudioOutOutput( audio_out_port_id_, pBuffer );
		if ( returnCode < 0 ) {
			printf( "writeAudioOut: sceAudioOutOutput() failed: 0x%08X\n", returnCode );
			return returnCode;
		}
	}

	// Write audio to file
	if ( audio_out_mode_ & NGS_WRITE_TO_FILE ) {
		returnCode = sceIoWrite( audio_out_file_, pBuffer, audio_out_num_bytes_per_update_ );
		if ( returnCode != audio_out_num_bytes_per_update_ ) {
			printf( "writeAudioOut: sceIoWrite() failed: 0x%08X\n", returnCode );
			return returnCode;
		}
	}

	return SCE_NGS_OK;
}

void AudioManagerVita::CleanUpAudioOut( void )
{
	Int32 returnCode;

	if ( audio_out_mode_ & NGS_SEND_TO_DEVICE ) {
		// Wait for output of all registered audio data
		returnCode = sceAudioOutOutput( audio_out_port_id_, NULL );
		if ( returnCode < 0 ) {
			printf( "shutdownAudioOut: sceAudioOutOutput(port %d, NULL) failed: 0x%08X\n", audio_out_port_id_, returnCode );
		}

		returnCode = sceAudioOutReleasePort( audio_out_port_id_ );
		if ( returnCode < 0 ) {
			printf( "shutdownAudioOut: sceAudioOutReleasePort() failed: 0x%08X\n", returnCode );
		}
	}

	if ( audio_out_mode_ & NGS_WRITE_TO_FILE ) {
		returnCode = sceIoClose( audio_out_file_ );
		if ( returnCode < 0 ) {
			printf( "shutdownAudioOut: sceIoClose() failed: 0x%08X\n", returnCode );
		}
	}
}

Int32 AudioManagerVita::ConfigAudioOut( Int32 config_cmd, Int32 flag, Int32 * params )
{
	Int32 returnCode = SCE_NGS_OK;

	switch (config_cmd){
		case NGS_SET_VOLUME:
			returnCode = sceAudioOutSetVolume( audio_out_port_id_, flag, params );
			if ( returnCode < 0){
				printf( "configAudioOut: sceAudioOutSetVolume() failed: 0x%08X\n", returnCode );
			}
			else{
				printf( "configAudioOut: sending output to audio port %d\n", audio_out_port_id_ );
			}
			break;
		default:
			printf( "configAudioOut: Invalid configuration command: 0x%08X\n", config_cmd );
			returnCode = SCE_NGS_ERROR_INVALID_PARAM;
			break;

	}

	return returnCode;
}





Int32 AudioManagerVita::LoadMusic(const char *strFileName, const Platform& platform)
{
	std::string full_pathname = platform.FormatFilename(strFileName);
	return loadWAVFile(full_pathname.c_str(), &music_);
}

Int32 AudioManagerVita::PlayMusic()
{
	Int32       return_value = SCE_NGS_OK;

	// Initialise music voice
	return_value = InitPlayerStream( music_voice_, SCE_NGS_VOICE_T1_PCM_PLAYER, &music_ );
	if(return_value == SCE_NGS_OK)
	{
		return_value = sceNgsVoicePlay( music_voice_ );
		if(return_value == SCE_NGS_OK)
			music_voice_playing_ = true;
	}

	return return_value;
}

Int32 AudioManagerVita::StopMusic()
{
	Int32       return_value = SCE_NGS_OK;

	return_value = sceNgsVoiceKill( music_voice_ );
	if(return_value == SCE_NGS_OK)
	music_voice_playing_ = false;

	return return_value;
}

Int32 AudioManagerVita::PlaySample(const Int32 sample_index, const bool looping)
{
	if(sample_index < samples_.size())
	{
		// check that we have valid sample data
		// if not, return
		if(!samples_[sample_index] || !samples_[sample_index]->pData)
			return -1;

		UInt32 voice_index=0;
		// try and find a free voice
		for(; voice_index<max_simultaneous_voices_;++voice_index)
		{
			if(!sample_voice_playing_[voice_index])
				break;
		}

		if(voice_index == max_simultaneous_voices_)
		{
			// no free voices
			return -1;
		}
		else
		{
			Int32 return_value = SCE_NGS_OK;
			return_value = InitPlayer( sample_voices_[voice_index], SCE_NGS_VOICE_T1_PCM_PLAYER, samples_[sample_index], looping );
			if ( return_value != SCE_NGS_OK )
				return -1;

			return_value = sceNgsVoicePlay( sample_voices_[voice_index] );
			if ( return_value != SCE_NGS_OK )
			{
				printf( "process: sceNgsVoicePlay(%d) failed: 0x%08X\n", voice_index, return_value );
				return -1;
			}
			sample_voice_playing_[voice_index] = true;
			sample_voice_sample_num_[voice_index] = sample_index;
			sample_voice_looping_[voice_index] = looping;
		}
		return voice_index;
	}
	else
		return -1;
}

Int32 AudioManagerVita::StopPlayingSampleVoice(const Int32 voice_index)
{
	Int32 return_value = SCE_NGS_OK;

	if(sample_voice_playing_[voice_index])
	{
		return_value = sceNgsVoiceKill( sample_voices_[voice_index] );
		if(return_value == SCE_NGS_OK)
		{
			sample_voice_playing_[voice_index] = false;
			sample_voice_sample_num_[voice_index] = -1;
		}
	}

	return return_value;
}

void AudioManagerVita::UnloadMusic()
{
	StopMusic();
	music_.CleanUp();
}

void AudioManagerVita::UnloadSample(Int32 sample_num)
{
	if(sample_num < samples_.size())
	{
		// stop any voices that are using this sample
		for(UInt32 voice_index=0; voice_index<max_simultaneous_voices_;++voice_index)
		{
			if(sample_voice_sample_num_[voice_index] != -1 && sample_voice_sample_num_[voice_index] == sample_num)
				StopPlayingSampleVoice(voice_index);
		}

		delete samples_[sample_num];
		samples_[sample_num] = NULL;
	}
}

void AudioManagerVita::UnloadAllSamples()
{
	// stop all voices
	for(UInt32 voice_index=0; voice_index<max_simultaneous_voices_;++voice_index)
	{
		if(sample_voice_sample_num_[voice_index] != -1)
			StopPlayingSampleVoice(voice_index);
	}

	// clean up loaded sample data
	for ( std::vector<SoundInfo*>::iterator sample_iter =  samples_.begin(); sample_iter != samples_.end();++sample_iter)
		delete *sample_iter;
	samples_.clear();
}


Int32 AudioManagerVita::ConnectRacks( SceNgsHVoice hVoiceSource, SceNgsHVoice hVoiceDest, SceNgsHPatch* patch )
{
	int                  returnCode;
	SceNgsPatchSetupInfo patchInfo;

	// Create patch
	patchInfo.hVoiceSource           = hVoiceSource;
	patchInfo.nSourceOutputIndex     = 0;
	patchInfo.nSourceOutputSubIndex  = SCE_NGS_VOICE_PATCH_AUTO_SUBINDEX;
	patchInfo.hVoiceDestination      = hVoiceDest;
	patchInfo.nTargetInputIndex      = 0;

	returnCode = sceNgsPatchCreateRouting( &patchInfo, patch );
	if ( returnCode != SCE_NGS_OK ) {
		printf( "connectRacks: sceNgsPatchCreateRouting() failed: 0x%08X\n", returnCode );
		return returnCode;
	}

	VolumeInfo default_volume_info;
	returnCode = SetPatchChannelVolumes(*patch, default_volume_info);

	return SCE_NGS_OK;
}

Int32 AudioManagerVita::SetPatchChannelVolumes(SceNgsHPatch hPatchHandle, const struct VolumeInfo& volume_info)
{
	int                  returnCode;
	SceNgsPatchRouteInfo patchRouteInfo;

	/** Set volumes on patch */
	returnCode = sceNgsPatchGetInfo(hPatchHandle, &patchRouteInfo, NULL );
	if (returnCode != SCE_NGS_OK) {
		printf( "setPatchChannelVolumes: sceNgsPatchGetInfo() failed: 0x%08X\n", returnCode );
		return returnCode;
	}

	float volume = volume_info.volume;
	float pan = volume_info.pan;

	if (volume < 0.0f)
		volume = 0.0f;
	else if (volume_info.volume > 1.0f)
		volume = 1.0f;

	if (pan < -1.0f)
		pan = -1.0f;
	else if (pan > 1.0f)
		pan = 1.0f;

	float source_volume_left = volume*(pan <= 0.0f ? 1.0f: 1.0f-pan);
	float source_volume_right = volume*(pan >= 0.0f ? 1.0f: 1.0f+pan);
	if (patchRouteInfo.nOutputChannels == 1) {
		if (patchRouteInfo.nInputChannels == 1) {
			patchRouteInfo.vols.m[0][0] = source_volume_left;	// left to left
		} else {
			patchRouteInfo.vols.m[0][0] = source_volume_left;	// left to left
			patchRouteInfo.vols.m[0][1] = source_volume_right;	// left to right
		}
	} else {
		if (patchRouteInfo.nInputChannels == 1) {
			patchRouteInfo.vols.m[0][0] = source_volume_left;	// left to left
			patchRouteInfo.vols.m[1][0] = source_volume_left;	// right to left
		} else {
			patchRouteInfo.vols.m[0][0] = source_volume_left;	// left to left
			patchRouteInfo.vols.m[0][1] = 0.0f;			// left to right
			patchRouteInfo.vols.m[1][0] = 0.0f;			// right to left
			patchRouteInfo.vols.m[1][1] = source_volume_right;	// right to right
		}
	}
	returnCode = sceNgsVoicePatchSetVolumesMatrix(hPatchHandle, &patchRouteInfo.vols );
	if (returnCode != SCE_NGS_OK) {
		printf( "setPatchChannelVolumes: sceNgsVoicePatchSetVolumesMatrix() failed: 0x%08X\n", returnCode );
		return returnCode;
	}

	return SCE_OK;
}


Int32 AudioManagerVita::SetSamplePitch(const Int32 voice_index, float pitch)
{
	return SetPitch(sample_voices_[voice_index], pitch);
}

Int32 AudioManagerVita::SetMusicPitch(float pitch)
{
	Int32 return_value = SCE_NGS_OK;

	return_value = LockMutex(&music_voice_params_mutex_);
	if(return_value == SCE_NGS_OK)
		return_value = SetPitch(music_voice_, pitch);
	if(return_value == SCE_NGS_OK)
		return_value = UnlockMutex(&music_voice_params_mutex_);

	return return_value;
}

Int32 AudioManagerVita::SetPitch(SceNgsHVoice hVoice, float pitch)
{
	int returnCode = SCE_OK;
	SceNgsBufferInfo		bufferInfo;
	SceNgsPlayerParams	*pPcmParams;

	// Get player parameters
	returnCode = sceNgsVoiceLockParams( hVoice, SCE_NGS_VOICE_T1_PCM_PLAYER, SCE_NGS_PLAYER_PARAMS_STRUCT_ID, &bufferInfo );
	if( returnCode != SCE_NGS_OK )
		return returnCode;

	// Set player parameters
	pPcmParams = (SceNgsPlayerParams *)bufferInfo.data;

	pPcmParams->desc.id     = SCE_NGS_PLAYER_PARAMS_STRUCT_ID;
	pPcmParams->desc.size   = sizeof(SceNgsPlayerParams);

	pPcmParams->fPlaybackScalar = pitch;

	// Update player parameters
	returnCode = sceNgsVoiceUnlockParams( hVoice, SCE_NGS_VOICE_T1_PCM_PLAYER );
	if( returnCode != SCE_NGS_OK )
		return returnCode;

	return returnCode;
}

Int32 AudioManagerVita::GetSampleVoiceVolumeInfo(const Int32 voice_index, struct VolumeInfo& volume_info)
{
	Int32 return_value = SCE_NGS_OK;
	if(voice_index >= 0 && voice_index < max_simultaneous_voices_)
		volume_info = sample_voice_volume_info_[voice_index];
	else
		return_value = -1;
	return return_value;
}

Int32 AudioManagerVita::SetSampleVoiceVolumeInfo(const Int32 voice_index, const struct VolumeInfo& volume_info)
{
	Int32 return_value = SCE_NGS_OK;
	if(voice_index >= 0 && voice_index < max_simultaneous_voices_)
	{
		return_value = SetPatchChannelVolumes(sample_patch_[voice_index], volume_info);
		if(return_value == SCE_NGS_OK)
			sample_voice_volume_info_[voice_index] = volume_info;
	}
	else
		return_value = -1;
	return return_value;
}

Int32 AudioManagerVita::GetMusicVolumeInfo(struct VolumeInfo& volume_info)
{
	volume_info = music_volume_info_;
	return SCE_NGS_OK;
}

Int32 AudioManagerVita::SetMusicVolumeInfo(const struct VolumeInfo& volume_info)
{
	Int32 return_value = SCE_NGS_OK;
	return_value = SetPatchChannelVolumes(music_patch_, volume_info);
		if(return_value == SCE_NGS_OK)
			music_volume_info_ = volume_info;
	return return_value;
}

Int32 AudioManagerVita::SetMasterVolume(float volume)
{
	Int32 return_value = SCE_NGS_OK;
	if(volume >= 0.0f && volume <= 1.0f)
	{
		int volume_value = static_cast<int>(volume*static_cast<float>(SCE_AUDIO_VOLUME_0DB));
		int volume_params[2];
		volume_params[0] = volume_value;
		volume_params[1] = volume_value;
		return_value = sceAudioOutSetVolume( audio_out_port_id_, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, volume_params );
	}

	return return_value;
}

Int32 AudioManagerVita::CreateMutex(SceKernelLwMutexWork* mutex, const char* name)
{
	int ret;
	if(mutex == NULL) return -1;

	ret = sceKernelCreateLwMutex( mutex,
							name,
							SCE_KERNEL_LW_MUTEX_ATTR_TH_FIFO,// | SCE_KERNEL_LW_MUTEX_ATTR_RECURSIVE,
							0,
							NULL);

	return ret;
}

Int32 AudioManagerVita::CleanUpMutex(SceKernelLwMutexWork* mutex)
{
	int ret;
	if(mutex == NULL) return -1;

	ret = sceKernelDeleteLwMutex( mutex);
	return ret;
}

Int32 AudioManagerVita::LockMutex(SceKernelLwMutexWork* mutex)
{
	int ret;
	if(mutex == NULL) return -1;
	
	ret = sceKernelLockLwMutex( mutex, 1, NULL);
	return ret;
}

Int32 AudioManagerVita::UnlockMutex(SceKernelLwMutexWork* mutex)
{
	int ret;
	if(mutex == NULL) return -1;

	ret = sceKernelUnlockLwMutex( mutex, 1);
	return ret;
}

Int32 AudioManagerVita::LockMusicMutex()
{
	return LockMutex(&music_voice_params_mutex_);
}

Int32 AudioManagerVita::UnlockMusicMutex()
{
	return UnlockMutex(&music_voice_params_mutex_);
}

}

#include "AudioCapture.hpp"

#include <iostream>

#define REFTIMES_PER_SEC  10

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { m_LoadedSuccessfully = false; return; }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

AudioCaptureSource::AudioCaptureSource( )
{
	HRESULT hr;
	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
	IMMDeviceEnumerator *pEnumerator = NULL;
	IMMDevice *pDevice = NULL;
	IAudioClient *pAudioClient = NULL;
	IAudioCaptureClient *pCaptureClient = NULL;
	WAVEFORMATEX *pwfx = NULL;

	hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void**)&pEnumerator );
	EXIT_ON_ERROR( hr );

	hr = pEnumerator->GetDefaultAudioEndpoint(
		eRender, eConsole, &pDevice );
	EXIT_ON_ERROR( hr );

	hr = pDevice->Activate(
		IID_IAudioClient, CLSCTX_ALL,
		NULL, (void**)&pAudioClient );
	EXIT_ON_ERROR( hr );

	hr = pAudioClient->GetMixFormat( &pwfx );
	EXIT_ON_ERROR( hr );

	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_LOOPBACK,
		hnsRequestedDuration,
		0,
		pwfx,
		NULL );
	EXIT_ON_ERROR( hr );

	hr = pAudioClient->GetService(
		IID_IAudioCaptureClient,
		(void**)&pCaptureClient );
	EXIT_ON_ERROR( hr );

	hr = pAudioClient->Start( );  // Start recording.
	EXIT_ON_ERROR( hr );

	pEnumerator->Release( );

	m_LoadedSuccessfully = true;

	m_Device = pDevice;
	m_AudioClient = pAudioClient;
	m_AudioCaptureClient = pCaptureClient;
	m_Format = pwfx;
}

AudioCaptureSource::~AudioCaptureSource( )
{
	m_AudioClient->Stop( );

	CoTaskMemFree( m_Format );
	m_Device->Release( );
	m_AudioClient->Release( );
	m_AudioCaptureClient->Release( );
}

bool AudioCaptureSource::IsReady( ) const
{
	return m_LoadedSuccessfully;
}


std::size_t AudioCaptureSource::ReadData( std::uint8_t * target, std::size_t count )
{
	UINT32 packetLength = 0;

	HRESULT hr;
	hr = m_AudioCaptureClient->GetNextPacketSize( &packetLength );
	if( FAILED( hr ) )
	{
		__debugbreak( );
		return -1;
	}

	BYTE * data;
	DWORD flags;
	UINT32 numFramesAvailable;

	// Get the available data in the shared buffer.
	hr = m_AudioCaptureClient->GetBuffer(
		&data,
		&numFramesAvailable,
		&flags, NULL, NULL );
	if( FAILED( hr ) )
	{
		__debugbreak( );
		return -1;
	}

	if( flags & AUDCLNT_BUFFERFLAGS_SILENT )
	{
		m_AudioCaptureClient->ReleaseBuffer( numFramesAvailable );
		return 0;
	}
	else
	{
		int numBytesAvailable = numFramesAvailable * (m_Format->wBitsPerSample / 8) * m_Format->nChannels;
		int writeCount = min( numBytesAvailable, count );
		memcpy( target, data, writeCount );
		m_AudioCaptureClient->ReleaseBuffer( numFramesAvailable );
		return writeCount;
	}
}

const WAVEFORMATEX * AudioCaptureSource::GetFormat( ) const
{
	return m_Format;
}
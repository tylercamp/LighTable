
#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <mmdeviceapi.h>
#include <Audioclient.h>

#include <cstdlib>
#include <cstdint>

//	http://msdn.microsoft.com/en-us/library/windows/desktop/dd370800(v=vs.85).aspx General WASAPI
//	http://msdn.microsoft.com/en-us/library/windows/desktop/dd316551(v=vs.85).aspx Loopback


class AudioCaptureSource
{
public:
	AudioCaptureSource( );
	~AudioCaptureSource( );

	bool IsReady( ) const;

	std::size_t ReadData( std::uint8_t * target, std::size_t count );
	const WAVEFORMATEX * GetFormat( ) const;

private:
	bool m_LoadedSuccessfully;

	IAudioClient * m_AudioClient;
	IAudioCaptureClient * m_AudioCaptureClient;
	IMMDevice * m_Device;

	WAVEFORMATEX * m_Format;
};


#endif
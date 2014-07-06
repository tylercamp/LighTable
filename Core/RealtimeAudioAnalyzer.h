
#ifndef _REALTIME_AUDIO_ANALYZER_H_
#define _REALTIME_AUDIO_ANALYZER_H_

#include <vector>
#include <mutex>
#include <thread>


//	Note: Wave data is always mono
struct WaveFormat
{
	int SampleRate;
	int BitsPerSample;
};



class FFTResult
{
public:
	FFTResult( const std::vector<float> & waveData, const WaveFormat & waveFormat, bool normalizeWave = true );

	float GetBin( size_t index ) const;

private:
	WaveFormat m_SourceWaveFormat;
	std::vector<float> m_FFTBins;
};



class FrameInstance
{
public:
	~FrameInstance( );

	const std::vector<float> & GetFrameSamples( );
	const WaveFormat & GetFormat( );

	const FFTResult & GetFFT( );

protected:
	FrameInstance( WaveFormat frameFormat, const float * frameSamples, size_t bufferCount );

	friend class RealtimeAudioInstance;

private:
	std::vector<float> m_FrameSamples;
	FFTResult * m_FFT; // Lazily generated
	WaveFormat m_Format;
};



class AudioCaptureSource;

class RealtimeAudioInstance
{
public:
	RealtimeAudioInstance( size_t frameSize );
	~RealtimeAudioInstance( );

	//	FrameInstance that is generated must be manually deleted
	FrameInstance * CaptureCurrentAudioFrame( );



protected:
	float * m_CurrentRawAudioFrame;
	size_t m_MaxFrameCount;
	size_t m_CurrentFrameCount;

	std::thread * m_CaptureThread;
	std::mutex m_FrameMutex;

	bool m_ContinueAudioCapture;

	AudioCaptureSource * m_CaptureSource;

	friend void AudioCaptureThread( RealtimeAudioInstance * );
};

#endif
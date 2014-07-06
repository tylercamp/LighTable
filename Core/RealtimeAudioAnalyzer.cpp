
#include "RealtimeAudioAnalyzer.h"
#include "AudioCapture.hpp"

#include "kissfft.hh"

#define sqr(x) ((x)*(x))

FFTResult::FFTResult( const std::vector<float> & waveData, const WaveFormat & waveFormat, bool normalizeWave ) :
	m_SourceWaveFormat( waveFormat )
{
	size_t waveSampleCount = waveData.size( );

	kissfft<float> fft( waveSampleCount, false );

	kissfft<float>::cpx_type * sourceBuffer = new kissfft<float>::cpx_type[waveSampleCount];
	kissfft<float>::cpx_type * destBuffer = new kissfft<float>::cpx_type[waveSampleCount];

	for( size_t i = 0; i < waveSampleCount; i++ )
	{
		sourceBuffer[i].real( waveData[i] );
		sourceBuffer[i].imag( 0.0f );
	}

	if( normalizeWave )
	{
		float maxMagnitude = 0.0f;
		float currentMagnitude;
		for( size_t i = 0; i < waveSampleCount; i++ )
		{
			currentMagnitude = fabsf( waveData[i] );
			if( currentMagnitude > maxMagnitude )
				maxMagnitude = currentMagnitude;
		}

		for( size_t i = 0; i < waveSampleCount; i++ )
		{
			sourceBuffer[i].real( waveData[i] / maxMagnitude );
		}
	}

	fft.transform( sourceBuffer, destBuffer );

	m_FFTBins.reserve( waveSampleCount );
	for( size_t i = 0; i < waveSampleCount; i++ )
	{
		//	Post-scaling (http://www.zytrax.com/tech/audio/equalization.html)
		float real, imaginary;
		real = (destBuffer[i].real( ) * 2) / waveSampleCount;
		imaginary = (destBuffer[i].imag( ) * 2) / waveSampleCount;
		m_FFTBins.push_back( sqrtf( sqr(destBuffer[i].real()) + sqr(destBuffer[i].imag()) ) );
	}

	delete sourceBuffer;
	delete destBuffer;
}

float FFTResult::GetBin( size_t index ) const
{
	return m_FFTBins[index];
}





FrameInstance::FrameInstance( WaveFormat frameFormat, const float * frameSamples, size_t bufferCount ) :
	m_Format( frameFormat )
{
	m_FrameSamples.reserve( bufferCount );
	for( size_t i = 0; i < bufferCount; i++ )
		m_FrameSamples.push_back( frameSamples[i] );

	m_FFT = nullptr;
}

FrameInstance::~FrameInstance( )
{
	if( m_FFT != nullptr )
		delete m_FFT;
}

const std::vector<float> & FrameInstance::GetFrameSamples( )
{
	return m_FrameSamples;
}

const WaveFormat & FrameInstance::GetFormat( )
{
	return m_Format;
}

const FFTResult & FrameInstance::GetFFT( )
{
	if( m_FFT == nullptr )
		m_FFT = new FFTResult( m_FrameSamples, m_Format );

	return *m_FFT;
}





void AudioCaptureThread( RealtimeAudioInstance * ownerAudioInstance )
{
	CoInitializeEx( nullptr, COINIT_SPEED_OVER_MEMORY );
	//	TODO: Add error checking
	AudioCaptureSource * captureSource = new AudioCaptureSource( );

	ownerAudioInstance->m_CaptureSource = captureSource;

	float * destinationBuffer = ownerAudioInstance->m_CurrentRawAudioFrame;
	size_t & currentFrameCount = ownerAudioInstance->m_CurrentFrameCount;
	size_t maxFrameCount = ownerAudioInstance->m_MaxFrameCount;


	float rawBuffer[2048];
	//	Always assume mono conversion is necessary
	float monoConversionBuffer[1024];


	while( ownerAudioInstance->m_ContinueAudioCapture )
	{
		size_t bytesRead = captureSource->ReadData( (uint8_t *)rawBuffer, sizeof( rawBuffer ) / sizeof( float ) );

		if( bytesRead > 0 )
		{
			size_t numSamples = bytesRead / sizeof( float ) / 2; // numBytes / bytesPerSample / numChannels

			for( size_t i = 0; i < numSamples; i++ )
			{
				monoConversionBuffer[i] = (rawBuffer[i * 2] + rawBuffer[i * 2 + 1]) / 2.0f;
			}



			//	Update current raw frame data
			{
				std::lock_guard<std::mutex> lock( ownerAudioInstance->m_FrameMutex );

				if( numSamples < maxFrameCount )
				{
					//	Shift raw buffer to make room for new data
					for( size_t i = maxFrameCount - 1; i >= numSamples && i >= 0; i-- )
					{
						destinationBuffer[i] = destinationBuffer[i - numSamples];
					}
				}
				//	(Otherwise we'll just overwrite all of the old data)

				//	Copy new audio contents
				for( size_t i = 0; i < numSamples && i < maxFrameCount; i++ )
				{
					destinationBuffer[i] = monoConversionBuffer[i];
				}

				currentFrameCount = min( currentFrameCount + numSamples, maxFrameCount );
			}
		}
	}

	delete captureSource;

	CoUninitialize( );
}

RealtimeAudioInstance::RealtimeAudioInstance( size_t frameSize )
{
	m_MaxFrameCount = frameSize;
	m_CurrentFrameCount = 0;

	m_CurrentRawAudioFrame = new float[m_MaxFrameCount];

	m_ContinueAudioCapture = true;
	m_CaptureThread = new std::thread( AudioCaptureThread, this );
}

RealtimeAudioInstance::~RealtimeAudioInstance( )
{
	m_ContinueAudioCapture = false;
	m_CaptureThread->join( );
	delete m_CaptureThread;

	delete m_CurrentRawAudioFrame;
}

FrameInstance * RealtimeAudioInstance::CaptureCurrentAudioFrame( )
{
	WaveFormat waveFormat;
	waveFormat.BitsPerSample = m_CaptureSource->GetFormat( )->wBitsPerSample;
	waveFormat.SampleRate = m_CaptureSource->GetFormat( )->nSamplesPerSec;

	float * buffer = new float[m_MaxFrameCount];

	//	Generate a copy of the current raw audio frame (attempt to keep this loop as fast as possible to minimize locking on capture thread)
	{
		std::lock_guard<std::mutex> lock( m_FrameMutex );

		for( size_t i = 0; i < m_CurrentFrameCount; i++ )
			buffer[i] = m_CurrentRawAudioFrame[i];
	}

	if( m_CurrentFrameCount != m_MaxFrameCount )
	{
		//	Not a full buffer, fill in the rest of the data with zeroes
		for( size_t i = m_CurrentFrameCount; i < m_MaxFrameCount; i++ )
			buffer[i] = 0.0f;
	}

	FrameInstance * currentFrameInstance = new FrameInstance( waveFormat, buffer, m_MaxFrameCount );
	delete buffer;	//	FrameInstance is expected to copy the data

	return currentFrameInstance;
}
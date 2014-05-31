
#include <Windows.h>
#include <SDL.h>
#include <gl/GL.h>

#include <vector>
#include <list>

#undef main

#include "../Core/AudioCapture.hpp"
#include "../Core/kissfft.hh"

#pragma comment (lib, "SDL.lib")
#pragma comment (lib, "Core.lib")
#pragma comment (lib, "OpenGL32.lib")

float RandomNorm( )
{
	return ((float)rand( )) / (float)RAND_MAX;
}

void ProcessStereoAudioToMonoAudio( float * stereoAudio, float * monoAudio, size_t stereoBufferCount )
{
	for( size_t i = 0; i < stereoBufferCount / 2; i++ )
	{
		monoAudio[i] = (stereoAudio[i * 2 + 0] + stereoAudio[i * 2 + 1]) / 2.0f;
	}
}

// float VectorAverage( const std::vector<float> & vector )
// {
// 	if( vector.size( ) == 0 )
// 		return 0.0f;
// 
// 	float average = 0.0f;
// 	for( float value : vector )
// 		average += value;
// 	return average / vector.size( );
// }

// void ShiftVector( std::vector<float> & vector )
// {
// 	for( int i = 0; i < vector.size( ) - 1; i++ )
// 		vector[i] = vector[i + 1];
// 	vector.pop_back( );
// }

void GenerateCompositeWaveform( const std::list<std::vector<float>> & sourceWaveSessions, float ** outputBuffer, std::size_t * outputCount )
{
	std::size_t size = 0;
	for( auto & session : sourceWaveSessions )
		size += session.size( );

	float * result = new float[size];
	std::size_t progression = 0;
	for( auto & session : sourceWaveSessions )
	{
		memcpy( result + progression, session.data( ), sizeof( float ) * session.size( ) );
		progression += session.size( );
	}

	*outputBuffer = result;
	*outputCount = size;
}



//int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int )
int main( )
{
	CoInitializeEx( nullptr, COINIT_SPEED_OVER_MEMORY );
	AudioCaptureSource * captureSource = new AudioCaptureSource( );

	SDL_Init( SDL_INIT_VIDEO );
	SDL_SetVideoMode( 800, 600, 32, SDL_OPENGL );

	glMatrixMode( GL_PROJECTION );
	glOrtho( 0, 1, 0, 10, 0, 1 );

	glClearColor( 0.2f, 0.2f, 0.2f, 0.0f );

// 	size_t lastSize = -1;
// 
// 	int runningAverageLength = 1;
// 
// 	std::vector<float> runningAverages[2000];
// 	for( int i = 0; i < 2000; i++ )
// 		runningAverages[i].reserve( runningAverageLength + 1 );

	size_t maxSessions = 10;
	std::list<std::vector<float>> waveSessions;

	bool run = true;
	while( run )
	{
		SDL_PumpEvents( );
		SDL_Event event;
		while( SDL_PollEvent( &event ) )
		{
			if( event.type == SDL_QUIT )
				run = false;
		}

		float rawAudioBuffer[2048];
		size_t numBytesReceived = captureSource->ReadData( (uint8_t *)rawAudioBuffer, sizeof( rawAudioBuffer ) );
		float monoAudioBuffer[1024];
		size_t numFrames = numBytesReceived / 8; // bytes / sizeof(float) / numChannels

		ProcessStereoAudioToMonoAudio( rawAudioBuffer, monoAudioBuffer, numFrames * 2 );

		kissfft<float>::cpx_type * sourceData = nullptr;
		kissfft<float>::cpx_type * destData = nullptr;

		if( numFrames > 0 )
		{
			glClear( GL_COLOR_BUFFER_BIT );

			//	Append the audio session
			std::vector<float> currentSession; currentSession.reserve( numFrames );
			for( size_t i = 0; i < numFrames; i++ )
				currentSession.push_back( monoAudioBuffer[i] );
			waveSessions.push_back( currentSession );

			if( waveSessions.size( ) > maxSessions )
				waveSessions.pop_front( );
			else
				//	Move on to capture next session, we want a full session buffer before transforming
				continue;

			//	Composite all sessions
			float * resultCompositeBuffer;
			std::size_t compositeBufferCount;
			GenerateCompositeWaveform( waveSessions, &resultCompositeBuffer, &compositeBufferCount );

			sourceData = new kissfft<float>::cpx_type[compositeBufferCount];
			destData = new kissfft<float>::cpx_type[compositeBufferCount];

			kissfft<float> fft( compositeBufferCount, false );

			for( int i = 0; i < compositeBufferCount; i++ )
			{
				sourceData[i].real( resultCompositeBuffer[i] );
				sourceData[i].imag( 0.0f );
			}

			fft.transform( sourceData, destData );

			//	Delete composite buffer (will be regenerated next session)
			delete resultCompositeBuffer;

			//	Add transformed data to running averages
// 			for( int i = 0; i < numFrames; i++ )
// 			{
// 				runningAverages[i].push_back( destData[i].real( ) );
// 				if( runningAverages[i].size( ) > runningAverageLength )
// 					ShiftVector( runningAverages[i] );
// 			}

			float channelWidth = 1.0f / (compositeBufferCount / 2);

			glBegin( GL_QUADS );
			glColor3f( 1.0f, 1.0f, 1.0f );
			//	Start at 1 to skip DC line
			for( int i = 1; i < numFrames; i++ )
			{
				glVertex2f( i * channelWidth, 0.0f );
				glVertex2f( i * channelWidth, destData[i].real( ));
				glVertex2f( (i + 1)*channelWidth, destData[i].real( ) );
				glVertex2f( (i + 1)*channelWidth, 0.0f );
			}
			glEnd( );

			delete sourceData;
			delete destData;
		}

// 		if( lastSize != -1 && lastSize != numFrames && numFrames > 0 )
// 			__debugbreak( );
//
// 		if( numFrames > 0 )
// 			lastSize = numFrames;

		SDL_GL_SwapBuffers( );
	}

	SDL_Quit( );

	delete captureSource;
	CoUninitialize( );

	return 0;
}
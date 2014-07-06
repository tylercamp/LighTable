
#include <Windows.h>
#include <SDL.h>
#include <gl/GL.h>

#include <vector>
#include <list>

#include <thread>
#include <mutex>

#undef main

// #include "../Core/AudioCapture.hpp"
// #include "../Core/kissfft.hh"

#include "../Core/RealtimeAudioAnalyzer.h"

#pragma comment (lib, "SDL.lib")
#pragma comment (lib, "Core.lib")
#pragma comment (lib, "OpenGL32.lib")

void ProcessStereoAudioToMonoAudio( float * stereoAudio, float * monoAudio, size_t stereoBufferCount )
{
	for( size_t i = 0; i < stereoBufferCount / 2; i++ )
	{
		monoAudio[i] = (stereoAudio[i * 2 + 0] + stereoAudio[i * 2 + 1]) / 2.0f;
	}
}

void ShiftBuffer( float * buffer, size_t bufferSize, size_t shiftCount )
{
	if( shiftCount < bufferSize )
	{
		for( size_t i = bufferSize - 1; i >= shiftCount; i-- )
			buffer[i] = buffer[i - shiftCount];
	}

	//	Clear newly-unused indices
	for( size_t i = 0; i < shiftCount && i < bufferSize; i++ )
		buffer[i] = 0.0f;
}

struct RunData
{
	float * TargetBuffer;
	size_t BufferCount;
	std::mutex AccessMutex;
	bool ContinueCapture;
	bool IsFull;
};

bool FindWindowIndices( float * buffer, size_t bufferCount, size_t * windowBegin, size_t * windowEnd, float valueTolerance = 0.01f, float dyTolerance = 0.3f )
{
	//	Loop limit of bufferCount / 2 to prevent lop-sided windows

	for( size_t i_left = 0; i_left < bufferCount / 2; i_left++ )
	{
		for( size_t i_right = bufferCount - 1; i_right >= bufferCount / 2; i_right-- )
		{
			if( fabsf( buffer[i_left] - buffer[i_right] ) <= valueTolerance )
			{
				//	Found two y's practically equivalent

				float dy_left = buffer[i_left + 1] - buffer[i_left];
				float dy_right = buffer[i_right] - buffer[i_right - 1];

				//	Check to see if their deltas are similar
				if( fabsf( dy_left - dy_right ) <= dyTolerance )
				{
					//	Found a good enough window
					if( windowBegin )
						*windowBegin = i_left;
					if( windowEnd )
						*windowEnd = i_right;

					return true;
				}
			}
		}
	}

	return false;
}

// void CaptureThread( RunData * runData )
// {
// 	CoInitializeEx( nullptr, COINIT_SPEED_OVER_MEMORY );
// 	AudioCaptureSource * captureSource = new AudioCaptureSource( );
// 
// 	float rawBuffer[3000];
// 	float monoBuffer[1500];
// 
// 	size_t floatsRead = 0;
// 
// 	//	NOTE: Windowing function may be necessary (overlap function would be preferable - starting from each end, try to find dy (for dy/dt) that are equal
// 	//		with largest window
// 	while( runData->ContinueCapture )
// 	{
// 		size_t readByteCount = captureSource->ReadData( (uint8_t *)rawBuffer, sizeof( rawBuffer ) );
// 		if( readByteCount > 0 )
// 		{
// 			size_t numFloats = readByteCount / sizeof( float );
// 			size_t numFrames = numFloats / 2; // always assume stereo
// 
// 			ProcessStereoAudioToMonoAudio( rawBuffer, monoBuffer, numFloats );
// 
// 			{
// 				std::lock_guard<std::mutex> lock( runData->AccessMutex );
// 
// 				ShiftBuffer( runData->TargetBuffer, runData->BufferCount, numFrames );
// 				for( size_t i = 0; i < numFrames && i < runData->BufferCount; i++ )
// 					runData->TargetBuffer[i] = monoBuffer[i];
// 			}
// 
// 			floatsRead += numFrames;
// 			runData->IsFull = floatsRead >= runData->BufferCount;
// 		}
// 	}
// 
// 	CoUninitialize( );
// }

#define PI 3.1415926535897

int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int )
//int main( )
{
// 	RunData runData;
// 	runData.BufferCount = 4096;
// 	runData.TargetBuffer = new float[runData.BufferCount];
// 	runData.ContinueCapture = true;
// 	runData.IsFull = false;
// 
// 	ZeroMemory( runData.TargetBuffer, sizeof( float ) * runData.BufferCount );

	//std::thread captureThread( CaptureThread, &runData );

	size_t bufferCount = 1024;
	RealtimeAudioInstance * realtimeAudio = new RealtimeAudioInstance( bufferCount );


	SDL_Init( SDL_INIT_VIDEO );
	SDL_SetVideoMode( 800, 600, 32, SDL_OPENGL | SDL_RESIZABLE );

	glMatrixMode( GL_PROJECTION );
	glOrtho( 0, 1, /* -2 */ 0, 2.5, 0, 1 );

	glClearColor( 0.2f, 0.2f, 0.2f, 0.0f );

	//float * renderBuffer = new float[runData.BufferCount];

	size_t windowableCount = 0;

	bool run = true;
	while( run )
	{
		SDL_PumpEvents( );
		SDL_Event event;
		while( SDL_PollEvent( &event ) )
		{
			if( event.type == SDL_QUIT )
				run = false;

			if( event.type == SDL_VIDEORESIZE )
			{
				glViewport( 0, 0, event.resize.w, event.resize.h );
			}
		}

// 			{
// 				std::lock_guard<std::mutex> lock( runData.AccessMutex );
// 				memcpy( renderBuffer, runData.TargetBuffer, sizeof( float ) * runData.BufferCount );
// 			}

		glClear( GL_COLOR_BUFFER_BIT );

// 			kissfft<float>::cpx_type * sourceData = nullptr;
// 			kissfft<float>::cpx_type * destData = nullptr;
// 
// 			sourceData = new kissfft<float>::cpx_type[runData.BufferCount];
// 			destData = new kissfft<float>::cpx_type[runData.BufferCount];
// 
// 			kissfft<float> fft( runData.BufferCount, false );
// 
// 			for( int i = 0; i < runData.BufferCount; i++ )
// 			{
// 				float val = renderBuffer[i];
// 				sourceData[i].real( val );
// 				sourceData[i].imag( 0.0f );
// 			}
// 
// 			fft.transform( sourceData, destData );

		FrameInstance * currentFrame = realtimeAudio->CaptureCurrentAudioFrame( );
		auto fft = currentFrame->GetFFT( );

		float channelWidth = 1.0f / (bufferCount / 2);

		glBegin( GL_QUADS );
		glColor3f( 1.0f, 1.0f, 1.0f );
		//	Start at 1 to skip DC line
		for( int i = 1; i < bufferCount / 2; i++ )
		{
			//float height = log10f( destData[i].real( ) );
			//float height /* = destData[i].real( ) */;
			//height = fabsf( height );
			float height = fft.GetBin( i );
			//	Played a bunch with this equation until the visualization looked "usable"
			height = log10f ( fft.GetBin( i ) );
				
			glVertex2f( i * channelWidth, 0.0f );
			glVertex2f( i * channelWidth, height );
			glVertex2f( (i + 1)*channelWidth, height );
			glVertex2f( (i + 1)*channelWidth, 0.0f );
		}
		glEnd( );

// 			delete sourceData;
// 			delete destData;

		delete currentFrame;

		SDL_GL_SwapBuffers( );
	}

	SDL_Quit( );

	delete realtimeAudio;

// 	runData.ContinueCapture = false;
// 	captureThread.join( );

	return 0;
}
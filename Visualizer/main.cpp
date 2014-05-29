

/* Test application for audio-capture. Basic renderer of the captured waveforms. */


#define WIN32_LEAN_AND_MEAN

#include <SDL.h>
#include <Windows.h>
#include <gl/GL.h>
#include <queue>
#include <mmsystem.h>
#undef main

#include "../Core/AudioCapture.hpp"

#pragma comment( lib, "Core.lib" )
#pragma comment( lib, "OpenGL32.lib" )
#pragma comment( lib, "SDL.lib" )
#pragma comment( lib, "winmm.lib" )

bool UpdateBufferWithAudioData( std::vector<float> & target, float amplitude, AudioCaptureSource * source )
{
	target.clear( );
	float intermediateBuffer[2048];
	std::size_t requestRead = sizeof( intermediateBuffer );
	std::size_t lengthRead = source->ReadData( (std::uint8_t *)intermediateBuffer, requestRead );

	if( lengthRead == 0 )
	{
		target.push_back( 0 );
		target.push_back( 0 );
		return false;
	}

	int numFloats = lengthRead / sizeof( float );

	int channels = source->GetFormat( )->nChannels;

	for( std::size_t i = 0; i < numFloats / channels; i++ )
	{
		//	Average the channels
		float average = 0.0f;
		for( std::size_t j = 0; j < channels; j++ )
			average += intermediateBuffer[i * channels + j];
		average /= channels;

		target.push_back( average * amplitude );
	}

	return true;
}

bool fullscreen = false;
void GenerateScreen( int sw, int sh )
{
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, true );

	SDL_SetVideoMode( sw, sh, 32, SDL_HWSURFACE | SDL_OPENGL | SDL_DOUBLEBUF | SDL_RESIZABLE | (fullscreen ? SDL_NOFRAME : 0) );

	glViewport( 0, 0, sw, sh );
	glMatrixMode( GL_PROJECTION );
	glOrtho( 0, 1, 1, -1, 0.0f, 1.0f );

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_LINE_SMOOTH );
	glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
	glLineWidth( 3.0f );

	float base = 0.2f;
	glClearColor( base, base, base, 1.0f );
}

int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int )
//int main( )
{
	CoInitializeEx( nullptr, COINIT_SPEED_OVER_MEMORY );
	AudioCaptureSource * captureSource = new AudioCaptureSource( );

	SDL_Init( SDL_INIT_VIDEO );

	int sw = 800, sh = 600;
	GenerateScreen( sw, sh );

	std::vector<float> buffer;
	buffer.reserve( 2000 );

	bool run = true;
	while( run )
	{
		SDL_Event event;
		SDL_PumpEvents( );
		while( SDL_PollEvent( &event ) )
		{
			switch( event.type )
			{
				case( SDL_QUIT ) : {
					run = false;
					break;
				}

				case( SDL_KEYDOWN ) : {
					if( event.key.keysym.sym == SDLK_ESCAPE )
						run = false;
					if( event.key.keysym.mod & KMOD_ALT && event.key.keysym.sym == SDLK_RETURN )
					{
						fullscreen = !fullscreen;

						if( fullscreen )
						{
							SDL_putenv( "SDL_VIDEO_WINDOW_POS=0,0" );
							GenerateScreen( GetSystemMetrics( SM_CXSCREEN ), GetSystemMetrics( SM_CYSCREEN ) );
							//	Optimization
							SetActiveWindow( nullptr );
						}
						else
						{
							SDL_putenv( "SDL_VIDEO_WINDOW_POS=200,200" );
							GenerateScreen( sw, sh );
						}
					}
					break;
				}

				case( SDL_VIDEORESIZE ) : {
					glViewport( 0, 0, event.resize.w, event.resize.h );
					if( !fullscreen )
					{
						sw = event.resize.w;
						sh = event.resize.h;
					}
					break;
				}
			}
		}

		glClear( GL_COLOR_BUFFER_BIT );

		UpdateBufferWithAudioData( buffer, 1.5f, captureSource );

		glBegin( GL_LINE_STRIP );
		glColor3f( 1.0f, 1.0f, 1.0f );

		for( std::size_t i = 0; i < buffer.size( ); i++ )
		{
			float x, y;
			x = (float)i / (buffer.size( ) - 1);
			y = buffer[i];
			glVertex2f( x, y );
		}

		glColor4f( 1.0f, 1.0f, 1.0f, 0.02f );
		glVertex2f( 1.0f, 0.0f );
		glVertex2f( 0.0f, 0.0f );

		glEnd( );

		SDL_GL_SwapBuffers( );

	}

	delete captureSource;
	CoUninitialize( );

	SDL_Quit( );

	return 0;
}
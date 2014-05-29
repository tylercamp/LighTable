
#include <iostream>
#include <string>
#include <cassert>

#include <vector>

#include "AudioCapture.hpp"
#include "Serial.hpp"
#include "ArduinoLEDStripController.h"

/*
 * Post:
 * 
 * A while ago I bought a bunch of materials for a DIY ambient monitor lighting setup, which worked out to be a waste of money. 6 days ago I started to repurpose those parts into something cooler:

I've currently got code to capture Windows' audio output, code to communicate with my Arduino, and code to control the colors of the lighting kit for the LED strips. My goal? A kick-ass desk that lights up according to whatever you're playing from your speakers.

I'm currently setting the system up to have a bunch of different coloring systems that change for an even better light show. I ordered velcro strips to attach the LEDs to my desk. This thing's going to be the SHEEEEEYIT

All my code is currently on Github, you can check it out here:

 */

#pragma comment (lib, "Core.lib")

void ProcessStereoAudioToMonoAudio( float * stereoAudio, float * monoAudio, size_t stereoBufferCount )
{
	for( size_t i = 0; i < stereoBufferCount / 2; i++ )
	{
		monoAudio[i] = (stereoAudio[i*2 + 0] + stereoAudio[i*2 + 1]) / 2.0f;
	}
}

float RandomNorm( )
{
	return ((float)rand( )) / (float)RAND_MAX;
}

int main( int argc, char * argv[] )
{
	std::cout << "Starting up..." << std::endl;

	//	COM initialization (needed for AudioCaptureSource)
	CoInitializeEx( nullptr, COINIT_SPEED_OVER_MEMORY );

	AudioCaptureSource * captureSource = new AudioCaptureSource( );
	Serial * arduinoSerialConnection = new Serial( "COM3" ); // 'COM3' is my own setup (check Device Manager)
	ArduinoLEDStripController * ledStripController = new ArduinoLEDStripController( arduinoSerialConnection );

	std::cout << "Ready!" << std::endl;


	//	First test program for LighTable, proof-of-concept

	float r = 1.0f, g = 1.0f, b = 1.0f, brightness = 1.0f;
	float r_target = r, g_target = g, b_target = b;

	float  transitionSpeed = 0.001f,
		transitionSpeedFade = 0.02f,
		minTransitionSpeed = 0.002f,
		maxTransitionSpeed = 0.1f;
	
	while( !GetAsyncKeyState( VK_ESCAPE ) )
	{
		LEDStripColorState newColors;

		//	Read current Windows audio
		float rawBuffer[2048];
		std::size_t numBytesRead = captureSource->ReadData( (uint8_t *)rawBuffer, 2048 );
		float monoBuffer[1024];

		//	Convert from Stereo to Mono (easier to process)
		ProcessStereoAudioToMonoAudio( rawBuffer, monoBuffer, numBytesRead / sizeof( float ) );

		std::size_t monoBufferLength = numBytesRead / sizeof( float ) / 2;

		//	Process audio data
		float maxAmplitude = 0.0f;
		for( int i = 0; i < monoBufferLength; i++ )
		{
			assert( monoBuffer[i] >= -1.0f && monoBuffer[i] <= 1.0f );
			maxAmplitude = max( fabs( maxAmplitude ), monoBuffer[i] );
		}

		//	Always decrease brightness slightly
		brightness -= 0.001f;
		brightness = max( brightness, 0.0f );
		//	Increase brightness based on audio
		brightness = max( brightness, maxAmplitude * 2.0f );
		brightness = min( 1.0f, brightness );

		//	Change our color transition speed based on audio
		transitionSpeed += 0.25f * pow( maxAmplitude, 2.f );

		transitionSpeed -= transitionSpeedFade;

		//	Clamp transition speed
		transitionSpeed = max( transitionSpeed, minTransitionSpeed );
		transitionSpeed = min( transitionSpeed, maxTransitionSpeed );

		//	Regenerate target color if we've reached it
		if( fabs( r - r_target ) < 0.001f && fabs( g - g_target ) < 0.001f && fabs( b - b_target ) < 0.001f )
		{
			do 
			{
				r_target = RandomNorm( );
				g_target = RandomNorm( );
				b_target = RandomNorm( );

				//	Make sure not to pick a dark color
			} while ( r_target + g_target + b_target < 1.0f );

			std::cout << "New target: (" << r_target << "," << g_target << "," << b_target << ")\n";
		}
		else
		{
			//	Else move current color toward the target color
			float rdir, gdir, bdir;
			rdir = r - r_target > 0.0f ? -1.0f : 1.0f;
			gdir = g - g_target > 0.0f ? -1.0f : 1.0f;
			bdir = b - b_target > 0.0f ? -1.0f : 1.0f;

			if( fabs( r - r_target ) > transitionSpeed )
				r += transitionSpeed * rdir;
			else
				r = r_target;

			if( fabs( g - g_target ) > transitionSpeed )
				g += transitionSpeed * gdir;
			else
				g = g_target;

			if( fabs( b - b_target ) > transitionSpeed )
				b += transitionSpeed * bdir;
			else
				b = b_target;
		}

		//	LED brightness output is not linear
		float gammaCorrectedBrightness = pow( brightness, 2.8f );
		uint8_t r_converted = (uint8_t)(brightness * r * 255);
		uint8_t g_converted = (uint8_t)(brightness * g * 255);
		uint8_t b_converted = (uint8_t)(brightness * b * 255);

		newColors.SetAllLEDsToColor( r_converted, g_converted, b_converted );

		//	Update LEDs with new color data
		ledStripController->SetStripColorState( newColors );
	}


	delete ledStripController;
	delete arduinoSerialConnection;
	delete captureSource;

	CoUninitialize( );
	return 0;
}

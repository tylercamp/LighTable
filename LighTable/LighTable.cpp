
#include <iostream>
#include <string>
#include <cassert>

#include <vector>
#include <deque>
#include <chrono>

#include <thread>

#include <cmath>

#include "kissfft.hh"

#include "LighTable.h"

#include "AudioCapture.hpp"
#include "Serial.hpp"
#include "ArduinoLEDStripController.h"

#pragma comment (lib, "Core.lib")


//	http://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
typedef struct
{
	float r;       // percent
	float g;       // percent
	float b;       // percent
} rgb;

typedef struct
{
	float h;       // angle in degrees
	float s;       // percent
	float v;       // percent
} hsv;

rgb hsv2rgb( hsv in )
{
	float      hh, p, q, t, ff;
	long        i;
	rgb         out;

	if( in.s <= 0.0 )
	{       // < is bogus, just shuts up warnings
		out.r = in.v;
		out.g = in.v;
		out.b = in.v;
		return out;
	}
	hh = in.h;
	if( hh >= 360.0 ) hh = 0.0;
	hh /= 60.0;
	i = (long)hh;
	ff = hh - i;
	p = in.v * (1.0f - in.s);
	q = in.v * (1.0f - (in.s * ff));
	t = in.v * (1.0f - (in.s * (1.0f - ff)));

	switch( i )
	{
		case 0:
			out.r = in.v;
			out.g = t;
			out.b = p;
			break;
		case 1:
			out.r = q;
			out.g = in.v;
			out.b = p;
			break;
		case 2:
			out.r = p;
			out.g = in.v;
			out.b = t;
			break;

		case 3:
			out.r = p;
			out.g = q;
			out.b = in.v;
			break;
		case 4:
			out.r = t;
			out.g = p;
			out.b = in.v;
			break;
		case 5:
		default:
			out.r = in.v;
			out.g = p;
			out.b = q;
			break;
	}
	return out;
}

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

bool SerialPortRespondsWithACK( Serial * serialPort )
{
	using namespace std::chrono;

	system_clock::time_point startTime = system_clock::now( );

	do
	{
		Sleep( 100 );

		char buffer[20];
		int dataSize = serialPort->ReadData( buffer, sizeof( buffer ) );
		if( dataSize > 0 )
		{
			buffer[dataSize] = 0; // null-terminate the string
			std::string dataString = buffer;

			//	If the first line of text matches 'Ada', we're good to go
			if( dataString.substr( 0, dataString.find_first_of( '\n' ) ) == "Ada" )
				return true;
		}

		//	Wait up to 3 seconds for a response (should really be instantaneous)
	} while( duration_cast<seconds>(system_clock::now( ) - startTime).count( ) < 3 );

	return false;
}

struct LogicData
{
	bool ContinueRunning;
	ArduinoLEDStripController * LEDStripController;
	
	int AmplitudeHistoryLength;

	float BrightnessSensitivityFactor;
	float ColorAdvanceSensitivityFactor;
	float ColorAdvanceSensitivityRamp;

	float MaxColorAdvanceSpeed;
	float MinColorAdvanceSpeed;
	float ColorAdvanceSpeedFade;

	float BrightnessFadeSpeed;

	float GammaRamp;

	enum ColorMode
	{
		MODE_MONOCOLOR_RANDOM, // Randomly picks colors to change to
		MODE_HUECYCLE_MONOCOLOR, // Picks a single hue while simultaneously cycling saturation offsets via amplitude over time
		MODE_HUECYCLE_DIVERSE, // LEDs cover the entire hue spectrum, constant saturation, hue is cycled via amplitude
		MODE_TRANSFORMCOLOR_AMPLITUDE // RGB of all LEDs is solely a function of a FFT, only brightness is affected by amplitude
	} Mode;
};

void LogicThread( LogicData * data )
{
	CoInitializeEx( nullptr, COINIT_SPEED_OVER_MEMORY );
	AudioCaptureSource * captureSource = new AudioCaptureSource( );
	ArduinoLEDStripController * ledStripController = data->LEDStripController;

	std::deque<float> amplitudeHistory;

	float r = 1.0f, g = 1.0f, b = 1.0f, brightness = 1.0f;
	float r_target = r, g_target = g, b_target = b;

	float transitionSpeed = data->MinColorAdvanceSpeed;

	size_t amplitudeHistoryLength = data->AmplitudeHistoryLength;

	float hueTime = 0.0f;
	float saturationTime = 0.0f;

// 	kissfft<double>::cpx_type inputBuffer[1024];
// 	kissfft<double>::cpx_type outputFFT[1024];
// 	float transformAmplitudes[512];

	while( data->ContinueRunning )
	{
		LEDStripColorState newColors;
		newColors.SetAllLEDsToColor( 255, 0, 0 );

		//	Read current Windows audio
		float rawBuffer[2048];
		std::size_t numBytesRead = captureSource->ReadData( (uint8_t *)rawBuffer, 2048 );
		float monoBuffer[1024];

		//	Convert from Stereo to Mono (easier to process)
		ProcessStereoAudioToMonoAudio( rawBuffer, monoBuffer, numBytesRead / sizeof( float ) );

		std::size_t monoBufferLength = numBytesRead / sizeof( float ) / 2;

// 		if( monoBufferLength <= 0 )
// 			continue;

/*		kissfft<double> fft( monoBufferLength, false );*/

		//	Process audio data
		float maxAmplitude = 0.0f;
		for( size_t i = 0; i < monoBufferLength; i++ )
		{
			maxAmplitude = max( maxAmplitude, fabsf( monoBuffer[i] ) );
		}

		//	Make sure we have a full history before we start doing any processing
		if( amplitudeHistory.size( ) < amplitudeHistoryLength )
		{
			amplitudeHistory.push_back( maxAmplitude );
			continue;
		}

// 		for( int i = 0; i < monoBufferLength; i++ )
// 		{
// 			inputBuffer[i].real( monoBuffer[i] / maxAmplitude );
// 			inputBuffer[i].imag( 0.0f );
// 		}
// 
// 		fft.transform( inputBuffer, outputFFT );
// 
// 		for( int i = 0; i < monoBufferLength / 2; i++ )
// 		{
// 			float real = (outputFFT[i].real( ) * 2.0f) / monoBufferLength;
// 			float imaginary = (outputFFT[i].imag( ) * 2.0f) / monoBufferLength;
// 			transformAmplitudes[i] = sqrtf( real*real + imaginary*imaginary ) * maxAmplitude;
// 		}

		float averageHistoryAmplitude = 0.0f;
		for( float amplitude : amplitudeHistory )
			averageHistoryAmplitude += amplitude;
		averageHistoryAmplitude /= amplitudeHistoryLength;

		if( numBytesRead > 0 )
		{
			amplitudeHistory.push_back( maxAmplitude );
			if( amplitudeHistory.size( ) > amplitudeHistoryLength )
				amplitudeHistory.pop_front( );
		}

		//	Always decrease brightness slightly
		brightness -= data->BrightnessFadeSpeed;
		brightness = max( brightness, 0.0f );
		//	Increase brightness based on audio
		brightness = max( brightness, maxAmplitude * data->BrightnessSensitivityFactor );
		brightness = min( 1.0f, brightness );

		switch( data->Mode )
		{
			case(LogicData::MODE_HUECYCLE_DIVERSE) :
			{
				for( int i = 0; i < N_LEDS; i++ )
				{
					hsv chsv;
					chsv.h = sinf( hueTime + i * 0.02f ) * 180.0f + 180.0f;
					chsv.s = 1.0f;
					chsv.v = powf( brightness, 1.8f );
					rgb result = hsv2rgb( chsv );
					newColors.SetLEDColor( i, result.r * 255, result.g * 255, result.b * 255 );
				}

				transitionSpeed -= data->ColorAdvanceSpeedFade;

				//	Clamp transition speed
				transitionSpeed = max( transitionSpeed, data->MinColorAdvanceSpeed );

				//	Change our color transition speed based on audio
				transitionSpeed += data->ColorAdvanceSensitivityFactor * pow( max( 0.0f, maxAmplitude - averageHistoryAmplitude ), data->ColorAdvanceSensitivityRamp * 1.0f );
				transitionSpeed = min( transitionSpeed, data->MaxColorAdvanceSpeed );

				hueTime += transitionSpeed;

				break;
			}

			case( LogicData::MODE_MONOCOLOR_RANDOM ):
			{
				//	Change our color transition speed based on audio
				transitionSpeed += data->ColorAdvanceSensitivityFactor * pow( max( 0.0f, maxAmplitude - averageHistoryAmplitude ), data->ColorAdvanceSensitivityRamp );

				transitionSpeed -= data->ColorAdvanceSpeedFade;

				//	Clamp transition speed
				transitionSpeed = max( transitionSpeed, data->MinColorAdvanceSpeed );
				transitionSpeed = min( transitionSpeed, data->MaxColorAdvanceSpeed );

				//	Regenerate target color if we've reached it
				if( fabs( r - r_target ) < 0.001f && fabs( g - g_target ) < 0.001f && fabs( b - b_target ) < 0.001f )
				{
					do
					{
						r_target = RandomNorm( );
						g_target = RandomNorm( );
						b_target = RandomNorm( );

						//	Make sure not to pick a dark color
					} while( r_target + g_target + b_target < 1.0f );

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

				//	LED brightness output is not linear, nor is our perception of brightness
				float gammaCorrectedBrightness = pow( brightness, data->GammaRamp );
				uint8_t r_converted = (uint8_t)(brightness * r * 255);
				uint8_t g_converted = (uint8_t)(brightness * g * 255);
				uint8_t b_converted = (uint8_t)(brightness * b * 255);

				newColors.SetAllLEDsToColor( r_converted, g_converted, b_converted );
				break;
			}
		}

		//	Update LEDs with new color data
		ledStripController->SetStripColorState( newColors );
	}

	CoUninitialize( );
}

LighTable::LighTable( )
{
	m_OperationsThread = nullptr;
	m_ArduinoSerialConnection = nullptr;
	m_StripController = nullptr;
	m_ThreadLogicData = nullptr;
	m_ColorMode = LogicData::MODE_MONOCOLOR_RANDOM;
}

LighTable::~LighTable( )
{
	Stop( );
}

void LighTable::Start( const std::string & arduinoComPort )
{
	if( m_IsRunning )
		return;

	Serial * arduinoSerialConnection = new Serial( arduinoComPort.c_str( ) );
	ArduinoLEDStripController * ledStripController = new ArduinoLEDStripController( arduinoSerialConnection );

	LogicData * logicData = new LogicData( );
	logicData->ContinueRunning = true;

	logicData->Mode = (LogicData::ColorMode)m_ColorMode;

	logicData->LEDStripController = ledStripController;
	logicData->ColorAdvanceSpeedFade = 0.035f;
	logicData->MinColorAdvanceSpeed = 0.0005f;
	logicData->MaxColorAdvanceSpeed = 0.05f;
	logicData->ColorAdvanceSensitivityFactor = 2.0f;
	logicData->ColorAdvanceSensitivityRamp = 2.0f;

	logicData->AmplitudeHistoryLength = 25;

	logicData->BrightnessSensitivityFactor = 2.5f;
	logicData->BrightnessFadeSpeed = 0.0015f;

	logicData->GammaRamp = 2.8f;

	m_OperationsThread = new std::thread( LogicThread, logicData );
	m_ThreadLogicData = logicData;

	m_ArduinoSerialConnection = arduinoSerialConnection;
	m_StripController = ledStripController;

	m_IsRunning = true;

// 	std::cout << "- Connected! Waiting for Arduino response confirmation..." << std::endl;
// 	if( !SerialPortRespondsWithACK( arduinoSerialConnection ) )
// 	{
// 		std::cout << "--- Error: Did not receive correct response/request timed out. Expected 'Ada' ACK string. Is there an Arduino on " << arduinoSerialPort << "? Is it running the right software?" << std::endl;
// 		std::cin.get( );
// 		return 0;
// 	}
// 	else
// 	{
// 		std::cout << "- Got correct response!" << std::endl;
// 	}
}

void LighTable::Stop( )
{
	if( !m_IsRunning )
		return;

	m_IsRunning = false;

	LogicData * logicData = (LogicData *)m_ThreadLogicData;

	logicData->ContinueRunning = false;
	m_OperationsThread->join( );
	delete m_OperationsThread;

	delete m_StripController;
	delete m_ArduinoSerialConnection;

	delete logicData;

	m_StripController = nullptr;
	m_ArduinoSerialConnection = nullptr;
	m_ThreadLogicData = nullptr;
	m_OperationsThread = nullptr;
}

bool LighTable::IsRunning( )
{
	return m_IsRunning;
}

void LighTable::SetColoringMode( int colorMode )
{
	m_ColorMode = colorMode;

	if( m_ThreadLogicData != nullptr )
		((LogicData *)m_ThreadLogicData)->Mode = (LogicData::ColorMode)colorMode;
}

void LighTable::SetBrightnessSensitivity( float sensitivityFactor )
{
	if( !m_IsRunning ) return;

	((LogicData *)m_ThreadLogicData)->BrightnessSensitivityFactor = sensitivityFactor;
}

void LighTable::SetColorSensitivity( float sensitivityFactor )
{
	if( !m_IsRunning ) return;

	((LogicData *)m_ThreadLogicData)->ColorAdvanceSensitivityFactor = sensitivityFactor;
}
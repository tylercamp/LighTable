
//	https://github.com/adafruit/Adalight/tree/master/Processing/Colorswirl

#include "ArduinoLEDStripController.h"

#include <cassert>

LEDStripColorState::LEDStripColorState( )
{
	ZeroMemory( m_ColorData, sizeof( m_ColorData ) );
}

void LEDStripColorState::SetAllLEDsToColor( uint8_t r, uint8_t g, uint8_t b )
{
	for( int i = 0; i < 25; i++ )
	{
		this->SetLEDColor( i, r, g, b );
	}
}

void LEDStripColorState::SetLEDColor( int index, uint8_t r, uint8_t g, uint8_t b )
{
	m_ColorData[index][0] = r;
	m_ColorData[index][1] = g;
	m_ColorData[index][2] = b;
}

const uint8_t * LEDStripColorState::GetLEDColor( int index ) const
{
	assert( index >= 0 && index < 25 );
	return m_ColorData[index];
}

LEDStripColorState & LEDStripColorState::operator=(const LEDStripColorState & other)
{
	memcpy( m_ColorData, other.m_ColorData, sizeof( m_ColorData ) );

	return *this;
}





ArduinoLEDStripController::ArduinoLEDStripController( Serial * arduinoSerial )
{
	m_ArduinoSerialProxy = arduinoSerial;
}

void ArduinoLEDStripController::SetStripColorState( const LEDStripColorState & newState )
{
	m_StripColorState = newState;

	this->_ApplyCurrentStateToArduino( );
}

const LEDStripColorState & ArduinoLEDStripController::GetCurrentStripColorState( ) const
{
	return m_StripColorState;
}

void ArduinoLEDStripController::_ApplyCurrentStateToArduino( )
{
	//	Entire header could probably hard-coded as a constant

	uint8_t sendBuffer[6 + 25 * 3]; // header size + num-LEDs * channels-per-LED

	//	Build header
	sendBuffer[0] = 'A';
	sendBuffer[1] = 'd';
	sendBuffer[2] = 'a';
	sendBuffer[3] = 24 >> 8;
	sendBuffer[4] = 24 & 0xff;
	sendBuffer[5] = sendBuffer[3] ^ sendBuffer[4] ^ 0x55;

	//	Copy color data
	for( int i = 0; i < 25; i++ )
	{
		const uint8_t * currentLedData = m_StripColorState.GetLEDColor( i );
		sendBuffer[6 + i*3 + 0] = currentLedData[0];
		sendBuffer[6 + i*3 + 1] = currentLedData[1];
		sendBuffer[6 + i*3 + 2] = currentLedData[2];
	}

	m_ArduinoSerialProxy->WriteData( (char *)sendBuffer, sizeof( sendBuffer ) );
}

/*

import processing.serial.*;

int N_LEDS = 25; // Max of 65536

void setup( )
{
	byte[] buffer = new byte[6 + N_LEDS * 3];
	Serial myPort;
	int    i, hue1, hue2, bright, lo, r, g, b, t, prev, frame = 0;
	long   totalBytesSent = 0;
	float  sine1, sine2;

	noLoop( );

	// Assumes the Arduino is the first/only serial device.  If this is not the
	// case, change the device index here.  println(Serial.list()); can be used
	// to get a list of available serial devices.
	myPort = new Serial( this, Serial.list( )[0], 115200 );

	// A special header / magic word is expected by the corresponding LED
	// streaming code running on the Arduino.  This only needs to be initialized
	// once because the number of LEDs remains constant:
	buffer[0] = 'A';                                // Magic word
	buffer[1] = 'd';
	buffer[2] = 'a';
	buffer[3] = byte( (N_LEDS - 1) >> 8 );            // LED count high byte
	buffer[4] = byte( (N_LEDS - 1) & 0xff );          // LED count low byte
	buffer[5] = byte( buffer[3] ^ buffer[4] ^ 0x55 ); // Checksum

	sine1 = 0.0;
	hue1 = 0;
	prev = second( ); // For bandwidth statistics

	for( ;; )
	{
		sine2 = sine1;
		hue2 = hue1;

		// Start at position 6, after the LED header/magic word
		for( i = 6; i < buffer.length; )
		{
			// Fixed-point hue-to-RGB conversion.  'hue2' is an integer in the
			// range of 0 to 1535, where 0 = red, 256 = yellow, 512 = green, etc.
			// The high byte (0-5) corresponds to the sextant within the color
			// wheel, while the low byte (0-255) is the fractional part between
			// the primary/secondary colors.
			lo = hue2 & 255;
			switch( (hue2 >> 8) % 6 )
			{
				case 0:
					r = 255;
					g = lo;
					b = 0;
					break;
				case 1:
					r = 255 - lo;
					g = 255;
					b = 0;
					break;
				case 2:
					r = 0;
					g = 255;
					b = lo;
					break;
				case 3:
					r = 0;
					g = 255 - lo;
					b = 255;
					break;
				case 4:
					r = lo;
					g = 0;
					b = 255;
					break;
				default:
					r = 255;
					g = 0;
					b = 255 - lo;
					break;
			}

			// Resulting hue is multiplied by brightness in the range of 0 to 255
			// (0 = off, 255 = brightest).  Gamma corrrection (the 'pow' function
			// here) adjusts the brightness to be more perceptually linear.
			bright = int( pow( 0.5 + sin( sine2 ) * 0.5, 2.8 ) * 255.0 );
			buffer[i++] = byte( (r * bright) / 255 );
			buffer[i++] = byte( (g * bright) / 255 );
			buffer[i++] = byte( (b * bright) / 255 );

			// Each pixel is slightly offset in both hue and brightness
			hue2 += 40;
			sine2 += 0.3;
		}

		// Slowly rotate hue and brightness in opposite directions
		hue1 = (hue1 + 4) % 1536;
		sine1 -= .03;

		// Issue color data to LEDs and keep track of the byte and frame counts
		myPort.write( buffer );
		totalBytesSent += buffer.length;
		frame++;

		// Update statistics once per second
		if( (t = second( )) != prev )
		{
			print( "Average frames/sec: " );
			print( int( (float)frame / (float)millis( ) * 1000.0 ) );
			print( ", bytes/sec: " );
			println( int( (float)totalBytesSent / (float)millis( ) * 1000.0 ) );
			prev = t;
		}
	}
}

void draw( )
{
}

*/

//	https://github.com/adafruit/Adalight/tree/master/Processing/Colorswirl

#include "ArduinoLEDStripController.h"

#include <cassert>

LEDStripColorState::LEDStripColorState( )
{
	ZeroMemory( m_ColorData, sizeof( m_ColorData ) );
}

void LEDStripColorState::SetAllLEDsToColor( uint8_t r, uint8_t g, uint8_t b )
{
	for( int i = 0; i < N_LEDS; i++ )
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
	assert( index >= 0 && index < N_LEDS );
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

	uint8_t sendBuffer[6 + N_LEDS * 3]; // header size + num-LEDs * channels-per-LED

	//	Build header
	sendBuffer[0] = 'A';
	sendBuffer[1] = 'd';
	sendBuffer[2] = 'a';
	sendBuffer[3] = (N_LEDS - 1) >> 8;
	sendBuffer[4] = (N_LEDS - 1) & 0xff;
	sendBuffer[5] = sendBuffer[3] ^ sendBuffer[4] ^ 0x55;

	//	Copy color data
	for( int i = 0; i < N_LEDS; i++ )
	{
		const uint8_t * currentLedData = m_StripColorState.GetLEDColor( i );
		sendBuffer[6 + i*3 + 0] = currentLedData[0];
		sendBuffer[6 + i*3 + 1] = currentLedData[1];
		sendBuffer[6 + i*3 + 2] = currentLedData[2];
	}

	m_ArduinoSerialProxy->WriteData( (char *)sendBuffer, sizeof( sendBuffer ) );
}
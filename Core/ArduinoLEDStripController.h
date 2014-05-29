
#ifndef _ARDUINO_LED_CONTROLLER_H_
#define _ARDUINO_LED_CONTROLLER_H_

#include "Serial.hpp"

#include <cstdint>

class LEDStripColorState
{
public:
	LEDStripColorState( );
	
	void SetAllLEDsToColor( uint8_t r, uint8_t g, uint8_t b );
	void SetLEDColor( int index, uint8_t r, uint8_t g, uint8_t b );

	const uint8_t * GetLEDColor( int index ) const;

	LEDStripColorState & operator=( const LEDStripColorState & other );

private:
	uint8_t m_ColorData[25][3];
};

//	Intended for use with the Adafruit RGB LED strand
//	http://www.adafruit.com/products/322
//	
class ArduinoLEDStripController
{
public:
	ArduinoLEDStripController( Serial * arduinoSerial );

	void SetStripColorState( const LEDStripColorState & newState );
	const LEDStripColorState & GetCurrentStripColorState( ) const;

private:
	int m_NumLedStrips;

	void _ApplyCurrentStateToArduino( );

	LEDStripColorState m_StripColorState;
	Serial * m_ArduinoSerialProxy;
};

#endif
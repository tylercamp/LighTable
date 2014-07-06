
#ifndef _LIGHTABLE_H_
#define _LIGHTABLE_H_

#include <thread>

#include "Serial.hpp"
#include "ArduinoLEDStripController.h"

class LighTable
{
public:
	LighTable( );
	~LighTable( );

	void Start( const std::string & arduinoComPort );
	void Stop( );

	bool IsRunning( );

	void SetColoringMode( int colorMode );

	void SetBrightnessSensitivity( float sensitivityFactor );
	void SetColorSensitivity( float sensitivityFactor );

private:
	std::thread * m_OperationsThread;

	ArduinoLEDStripController * m_StripController;
	Serial * m_ArduinoSerialConnection;

	void * m_ThreadLogicData;
	
	int m_ColorMode;
	bool m_IsRunning;
};

#endif
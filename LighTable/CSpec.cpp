
#include "CSpec.h"
#include "LighTable.h"

LighTable * g_LighTableInstance = nullptr;

void InitLighTable( )
{
	if( g_LighTableInstance == nullptr )
		g_LighTableInstance = new LighTable( );
}

void StartLighTable( const char * comPort )
{
	InitLighTable( );

	g_LighTableInstance->Start( comPort );
}

void StopLighTable( )
{
	InitLighTable( );

	g_LighTableInstance->Stop( );
}

bool LighTableIsRunning( )
{
	InitLighTable( );

	return g_LighTableInstance->IsRunning( );
}

void SetColorSensitivity( float sensitivityFactor )
{
	InitLighTable( );

	g_LighTableInstance->SetColorSensitivity( sensitivityFactor );
}

void SetBrightnessSensitivity( float sensitivityFactor )
{
	InitLighTable( );

	g_LighTableInstance->SetBrightnessSensitivity( sensitivityFactor );
}

void SetColorMode( int colorMode )
{
	InitLighTable( );
	g_LighTableInstance->SetColoringMode( colorMode );
}
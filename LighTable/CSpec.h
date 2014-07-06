
//	Defines C-style API for interfacing with LighTable

#ifndef _C_SPEC_H_
#define _C_SPEC_H_

#ifdef BUILD_DLL
#define APICALL extern "C" __declspec(dllexport)
#else
#define APICALL
#endif

APICALL void StartLighTable( const char * comPort );
APICALL void StopLighTable( );
APICALL bool LighTableIsRunning( );

APICALL void SetColorSensitivity( float sensitivityFactor );
APICALL void SetBrightnessSensitivity( float sensitivityFactor );

APICALL void SetReferenceAmplitudeSampleSize( int numFrameSamples );

APICALL void SetColorMode( int colorMode );

#endif

#ifndef BUILD_DLL

#include "CSpec.h"

#include <iostream>

int main( )
{
	StartLighTable( "COM3" );
	std::cin.get( );
	StopLighTable( );

	return 0;
}

#endif
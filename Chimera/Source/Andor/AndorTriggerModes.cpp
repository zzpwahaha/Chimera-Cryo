// created by Mark O. Brown
#include "stdafx.h"
#include "AndorTriggerModes.h"
const std::array<AndorTriggerMode::mode, 5> AndorTriggerMode::allModes = {
	AndorTriggerMode::mode::External,
	AndorTriggerMode::mode::Internal,
	AndorTriggerMode::mode::Software,
	AndorTriggerMode::mode::ExternalStart,
	AndorTriggerMode::mode::ExternalExposure };

std::string AndorTriggerMode::toStr( AndorTriggerMode::mode m )
{
	if ( m == AndorTriggerMode::mode::External )
	{
		return "External";
	}
	else if ( m == AndorTriggerMode::mode::Internal )
	{
		return "Internal";
	}
	else if ( m == AndorTriggerMode::mode::Software)
	{
		return "Software";
	}
	else if (m == AndorTriggerMode::mode::ExternalStart)
	{
		return "External Start";
	}
	else if (m == AndorTriggerMode::mode::ExternalExposure)
	{
		return "External Exposure";
	}
	else
	{
		thrower ("AndorTriggerMode not recognized?!");
	}
}


AndorTriggerMode::mode AndorTriggerMode::fromStr ( std::string txt )
{
	for ( auto m : allModes )
	{
		if ( txt == toStr ( m ) )
		{
			return m;
		}
	}
	thrower ( "AndorTriggerMode string not recognized?!" );
}


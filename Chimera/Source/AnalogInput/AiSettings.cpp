#include "stdafx.h"
#include "AiSettings.h"

/*do not change order in AiChannelRange::allRanges*/
const std::array<AiChannelRange::which, 4> AiChannelRange::allRanges = {
	which::off, which::quarter, which::half, which::full };


/*
Note that this function is not case-sensitive - it will always convert the input string to lowercase before testing.
*/
AiChannelRange::which AiChannelRange::fromStr(std::string rowStr)
{
	for (auto w : allRanges)
	{
		if (str(rowStr, 13, false, true) == str(toStr(w), 13, false, true))
		{
			return w;
		}
	}
	thrower("Failed to convert string to Agilent Channel Mode!");
	return which::off;
}


std::string AiChannelRange::toStr(which m)
{
	switch (m)
	{
	case which::off:
		return "Off";
	case which::quarter:
		return "2.5V";
	case which::half:
		return "5V";
	case which::full:
		return "10V";
	}
	thrower("Faied to convert Ai range to string!");
	return "";
}

double AiChannelRange::scales(which m)
{
	switch (m)
	{
	case which::off:
		return 0.0;
	case which::quarter:
		return 2.5;
	case which::half:
		return 5.0;
	case which::full:
		return 10.0;
	}
	thrower("Faied to convert Ai range to scale factor!");
	return 0.0;
}

double AiChannelRange::scales()
{
	return scales(range);
}

unsigned short AiChannelRange::codes(which m)
{
	switch (m)
	{
	case which::off:
		return 0b11/*0b00*/; // 0b00 and 0b11 are for +/- 10V but never use 0b00 cause it may terminate the sended string
	case which::quarter:
		return 0b01;
	case which::half:
		return 0b10;
	case which::full:
		return 0b11;
	}
	thrower("Faied to convert Ai range to scale factor!");
	return 0b11;
}

unsigned short AiChannelRange::codes()
{
	return codes(range);
}
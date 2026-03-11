// created by Mark O. Brown
#include "stdafx.h"
#include "miscCommonFunctions.h"
#include "my_str.h"
#include <string>
#include <filesystem>

unsigned long getNextFileIndex( std::string fileBase, std::string ext )
{
	// find the first data file that hasn't been already written, starting with fileBase1.h5
	unsigned long fileNum = 1;
	while (std::filesystem::exists((fileBase + str (fileNum) + ext).c_str ())){
		fileNum++;
	}
	return fileNum;
}

std::string getCurrentTimeString()
{
	time_t time_obj = time(0);   // get time now
	struct tm currentTime;
	localtime_s(&currentTime, &time_obj);
	std::string timeStr = "(" + str(currentTime.tm_year + 1900) + ":" + str(currentTime.tm_mon + 1) + ":"
		+ str(currentTime.tm_mday) + ") " + str(currentTime.tm_hour) + ":"
		+ str(currentTime.tm_min) + ":" + str(currentTime.tm_sec);
	return timeStr;
}

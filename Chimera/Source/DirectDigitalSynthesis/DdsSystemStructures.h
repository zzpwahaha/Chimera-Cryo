#pragma once 

#include <array>
#include "ParameterSystem/Expression.h"
#include "GeneralObjects/CommonTypes.h"
#include <ExperimentThread/repeatManager.h>

enum class DDSGrid : size_t
{
	numPERunit = 4,
	numOFunit = 3,
	total = numPERunit * numOFunit
};


struct DdsInfo
{
	std::string name = "";

	double currFreq = 0;
	double minFreq = 0;
	double maxFreq = 500;
	double defaultFreq = 80;

	double currAmp = 0;
	double minAmp = 0;
	double maxAmp = 100;
	double defaultAmp = 100;

	std::string note = "None";
};



struct DdsCommandForm
{
	// can either be "ddsamp", "ddsfreq", "ddslinspaceamp", "ddslinspacefreq", "ddsrampamp", or "ddsrampfreq"
	std::string commandName;

	unsigned short line;
	timeType time;

	Expression initVal;
	Expression finalVal;
	Expression rampTime;
	Expression numSteps;

	// stores whether this command is subject to repeats and which repeat it correpsonds to in the tree if so
	repeatInfoId repeatId = { 0, {0,0} };
};

struct DdsCommand
{
	unsigned short line;
	double time;
	double amp;
	double freq;
	double endAmp;
	double endFreq;
	double rampTime;

	// same as DdsCommandForm, this will be used for repeat generation, i.e. copy and extend the std::vector<DdsCommand>
	repeatInfoId repeatId = { 0, {0,0} };
};


struct DdsSnapshot
{
	double time;
	std::array<std::array<double, 2>, 12> ddsValues;
	std::array<std::array<double, 2>, 12> ddsEndValues;
	std::array<double, 12> ddsRampTimes;
};


struct DdsChannelSnapshot
{
	char ampOrFreq;
	unsigned short channel;
	double time;
	double val;
	double endVal;
	double rampTime;
};






/*mark's dds table*/
struct ddsConnectionType{
	enum class type	{
		None, Serial, Async
	};
};

struct ddsIndvRampListInfo{
	unsigned short index=0;

	unsigned channel=0;
	Expression freq1="100"; // in MHZ
	Expression freq2="100";
	Expression amp1="100";  // between 0 and 100
	Expression amp2="100";
	Expression rampTime="500"; // in milliseconds
};

struct ddsRampBasic{
	bool explicitlySet = false;
	double freq1; // in MHZ
	double freq2;
	double amp1;  // between 0 and 100
	double amp2;
};


template<typename type>
class ddsBox{
	public:
		// a wrapper to make a matrix of "type" with the dimensions of the dds box - i.e. one value of "type" for each 
		// channel of each board.
		type& operator ()( unsigned boardNumber, unsigned channelNumber );
		type operator ()( unsigned boardNumber, unsigned channelNumber ) const;

		std::array<std::array<type, 4>, 2>& getBoards ( );
		std::array<type, 4>& getBoard ( unsigned which );
		unsigned numBoards ( );
		unsigned numChannels ( );

	private:
		std::array<std::array<type, 4>, 2> data;
};

template<typename type>
unsigned ddsBox<type>::numBoards ( )
{
	return 2;
}

template<typename type>
unsigned ddsBox<type>::numChannels ( )
{
	return 4;
}



template<typename type>
type& ddsBox<type>::operator ()( unsigned boardNumber, unsigned channelNumber )
{
	if ( boardNumber > 1 )
	{
		thrower ( "DDS board number out of range - should be 0 or 1" );
	}
	if ( channelNumber > 3 )
	{
		thrower ( "DDS channel number out of range - should be between 0 and 3 (inclusive)" );
	}
	return data[ boardNumber ][ channelNumber ];
}

template<typename type>
type ddsBox<type>::operator ()( unsigned boardNumber, unsigned channelNumber ) const 
{
	if ( boardNumber > 1 )
	{
		thrower ( "DDS board number out of range - should be 0 or 1" );
	}
	if ( channelNumber > 3 )
	{
		thrower ( "DDS channel number out of range - should be between 0 and 3 (inclusive)" );
	}
	return data[ boardNumber ][ channelNumber ];
}

template<typename type>
std::array<std::array<type, 4>, 2>& ddsBox<type>::getBoards ( )
{
	return data;
}

template<typename type>
std::array<type, 4>& ddsBox<type>::getBoard ( unsigned which )
{
	if ( which > 1 )
	{
		thrower ( "DDS board number out of range - should be 0 or 1" );
	}
	return data[ boardNumber ];
}


struct ddsRampFinFullInfo
{
	ddsBox<ddsRampBasic> rampParams;
	double rampTime;
};

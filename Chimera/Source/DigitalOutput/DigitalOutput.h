#pragma once
#include "Control.h"
#include <array>
#include "DoStructures.h"
#include "PrimaryWindows/IChimeraQtWindow.h"
#include <CustomQtControls/AutoNotifyCtrls.h>

class DigitalOutput
{
	public:
		void initialize ( IChimeraQtWindow* parent );
		void initLoc ( unsigned num, unsigned row);
		
		void enable ( bool enabledStatus );
		void updateStatus (  );

		bool defaultStatus;
		bool getStatus ( );
		std::pair<unsigned, unsigned> getPosition ( );

		void set ( bool status );
		void setName ( std::string nameStr );
		
		bool holdStatus;
		void setHoldStatus ( bool stat );
		CQCheckBox* check = nullptr;

	private:
		unsigned row;
		unsigned num;
		bool status;
};


class allDigitalOutputs
{
	public:
		allDigitalOutputs ( );
		DigitalOutput & operator()( unsigned num, unsigned row );
		/*assume unit is a row and get stacked vertically*/
		static const unsigned numRows = size_t(DOGrid::numOFunit);
		static const unsigned numColumns = size_t(DOGrid::numPERunit);
		// here, typename tells the compiler that the return will be a type.
		typename std::array<DigitalOutput, numRows*numColumns>::iterator begin ( ) { return core.begin ( ); }
		typename std::array<DigitalOutput, numRows*numColumns>::iterator end ( ) { return core.end ( ); }

	private:
		std::array<DigitalOutput, numRows*numColumns> core;
};

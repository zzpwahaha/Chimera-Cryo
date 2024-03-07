#pragma once
#include <vector>

//Simple helper class to keep track of how much memory has been used, and what channels to use next.
class MemoryController {
public:

	MemoryController();
	// returns vector of channel IDs to use on gmoog given desired number of channels used in parallel, 
	// and how much memory has already been used by the move sequence
	std::vector<int> getNextChannels(int channelsNeeded);
	void moveChannel(int blockID, int nmoves = 1);

private:
	struct memBlock
	{
		int blockID; //each block contains 8 channels. 0-7, 8-15, 16-23, 24-31, 32-39, 40-47;
		int channelID;
		size_t usedMemory;
		//constructor just takes block ID, and assumes memory starts out empty.
		memBlock(int id);
		//define a sorting operation that sorts by available memory
		bool operator < (const memBlock& mem) const;
		
	};

	std::vector<memBlock> memBlocks;

};
#include "stdafx.h"
#include "MemoryController.h"

MemoryController::MemoryController()
{
	memBlocks.reserve(8); 
	for (size_t i = 0; i < 8; i++) {
		memBlock mem(i);
		memBlocks.push_back(mem);
	}
}

std::vector<int> MemoryController::getNextChannels(int channelsNeeded)
{
	std::vector<int> channelsOut(channelsNeeded);

	for (size_t i = 0; i < channelsNeeded; i++) {
		memBlock& mBlock = getMemBlock();
		channelsOut[i] = mBlock.blockID + 8 * mBlock.channelID;
		mBlock.channelID += 1; //iterate the channel ID, so that if this block is selected again we use the next channel.
		mBlock.usedMemory += 3; //each full move uses up 3 snapshots.

		if (mBlock.usedMemory > 253) {
			//if the next move will use up more than the available 256 memory slices, delete this memory block so that it is not used again.
			memBlocks.erase(memBlocks.begin());
			if (memBlocks.size() < 1) {
				thrower("Maximum number of moves exceeded, gigamoog memory is full.");
			}
		}
	}

	for (size_t i = 0; i < memBlocks.size(); i++) {
		memBlocks[i].channelID = 0; //reset the channel IDs for the next round of moves.
	}

	return channelsOut;
}

void MemoryController::moveChannel(int blockID, int nmoves)
{
	memBlocks[blockID].usedMemory += nmoves;
}

MemoryController::memBlock& MemoryController::getMemBlock()
{
	std::stable_sort(memBlocks.begin(), memBlocks.end()); //Sort the blocks by used memory.
	return memBlocks[0];
}

MemoryController::memBlock::memBlock(int id)
	: blockID(id), channelID(0), usedMemory(0) {}

bool MemoryController::memBlock::operator<(const memBlock& mem) const
{
	return (usedMemory < mem.usedMemory);
}

#pragma once
#include <Rearrangement/rearrangeStructures.h>
#include <Rearrangement/rearrangeParameters.h>
#include <GeneralObjects/Queues.h>

class RearrangeGenerator
{
public:
	// THIS CLASS IS NOT COPYABLE.
	RearrangeGenerator& operator=(const RearrangeGenerator&) = delete;
	RearrangeGenerator(const RearrangeGenerator&) = delete;
	RearrangeGenerator(rearrangeParameters moveParam);
	void loadAtomImage(AtomImage atomImage);
	moveSequence getRearrangeMoves();
	moveSequence getRearrangeMoves(std::string rearrangeType);

private:
	std::vector<unsigned> generateCoordinates(std::vector<bool> positions);
	void filterAtomQueue();
	int sourceColumnSum(int iColumn, const std::vector<bool>& atomImg);
	int sourceRowSum(int iRow, const std::vector<bool>& atomImg);
	int equalizeY(moveSequence& moveseq);
	void enoughY(moveSequence& moveseq, bool constantMoves = false);
	void scrunchX(moveSequence& moveseq, bool centered = false);
	void scrunchY(moveSequence& moveseq, bool centered = false);
	void scrunchYFixedLength(moveSequence& moveseq, int nPerColumn, bool centered = false);
	void scrunchYTarget(moveSequence& moveseq, bool constantMoves = false);
	void compressX(moveSequence& moveseq); // need to figure out WTF
	void compressX2(moveSequence& moveseq);
	void filterReservoir(moveSequence& moveseq);

public:

private:
	AtomImage atomImage;
	const rearrangeParameters moveParam;
	const std::vector<unsigned> positionCoordinatesX, positionCoordinatesY;

	std::vector<bool> positionsX, positionsY; // will be assigned with moveParam.initialPositionsX/Y and could be changed within getRearrangeMoves
};


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
	moveSequence getRearrangeMoves(unsigned rearrangeRound);
	moveSequence getRearrangeMoves(std::string rearrangeType);

private:
	std::vector<unsigned> generateCoordinates(std::vector<bool> positions);
	void filterAtomQueue();
	int sourceColumnSum(int iColumn, const std::vector<bool>& atomImg);
	int sourceRowSum(int iRow, const std::vector<bool>& atomImg);
	int equalizeX(moveSequence& moveseq, bool constantMoves = true);
	int equalizeY(moveSequence& moveseq, bool constantMoves = true);
	void enoughX(moveSequence& moveseq, bool constantMoves = true);
	void enoughY(moveSequence& moveseq, bool constantMoves = true);
	void scrunchX(moveSequence& moveseq, bool centered = false);
	void scrunchY(moveSequence& moveseq, bool centered = false);
	void scrunchYFixedLength(moveSequence& moveseq, int nPerColumn, bool centered = false);
	void scrunchXTarget(moveSequence& moveseq, bool constantMoves = false);
	void scrunchYTarget(moveSequence& moveseq, bool constantMoves = false);
	void compressX(moveSequence& moveseq); // need to figure out WTF
	void compressX2(moveSequence& moveseq);
	void filterReservoir(moveSequence& moveseq);
	void RearrangeGenerator::removeFilteredAtomX(moveSequence& moveseq);
	void RearrangeGenerator::removeFilteredAtomY(moveSequence& moveseq);

	void tweezer1DInitializationTest(moveSequence& moveseq); // This is only initializing and deinitializing tones at loaded atom location, no move is involved. only works for 1D (or first row)
public:
	const rearrangeParameters moveParam;

private:
	AtomImage atomImage;
	const std::vector<unsigned> positionCoordinatesX, positionCoordinatesY;

	std::vector<bool> positionsX, positionsY; // will be assigned with moveParam.initialPositionsX/Y and could be changed within getRearrangeMoves
};


#include "stdafx.h"
#include "RearrangeGenerator.h"

moveSequence RearrangeGenerator::getRearrangeMoves(std::string rearrangeType)
{
	moveSequence moveseq;

	const auto& positions = moveParam.initialPositions;
	auto& positionsX = moveParam.initialPositionsX;
	auto& positionsY = moveParam.initialPositionsY;
	const auto& targetPositionsTemp = moveParam.targetPositions; //Make a copy of the target positions that can be modified.

	//define coordinate representation for convenience.
	//std::vector<unsigned> positionCoordinatesX, positionCoordinatesY;
	positionCoordinatesX.clear();
	positionCoordinatesY.clear();
	positionCoordinatesX.reserve(positionsX.size());
	positionCoordinatesY.reserve(positionsY.size());
	int ix = 0;
	for (auto const& channelBoolX : positionsX) {
		if (channelBoolX) {
			positionCoordinatesX.push_back(ix);
		}
		ix++;
	}
	int iy = 0;
	for (auto const& channelBoolY : positionsY) {
		if (channelBoolY) {
			positionCoordinatesY.push_back(iy);
		}
		iy++;
	}

	filterAtomQueue();

	if (rearrangeType == "scrunchx") {
		scrunchX(moveseq);
	}
	else if (rearrangeType == "scrunchy") {
		scrunchY(moveseq);
	}
	else if (rearrangeType == "equalscrunchy") {
		int nPerColumn = equalizeY(moveseq);
		scrunchYFixedLength(moveseq, nPerColumn);
	}
	else if (rearrangeType == "equalcenterscrunchy") {
		int nPerColumn = equalizeY(moveseq);
		scrunchYFixedLength(moveseq, nPerColumn, true);
	}
	else if (rearrangeType == "equalcenterscrunchyx") {
		int nPerColumn = equalizeY(moveseq);
		scrunchYFixedLength(moveseq, nPerColumn, true);

		int nGap = positionsY.size() / 2 - moveParam.scrunchSpacing * (nPerColumn / 2);
		for (int iy = 0; iy < positionsY.size(); iy++) {
			positionsY[iy] = false;
			if (iy >= nGap && iy % moveParam.scrunchSpacing == 0 && iy < nGap + moveParam.scrunchSpacing * nPerColumn) {
				positionsY[iy] = true;
			}
		}

		scrunchX(moveseq, true);
	}
	else if (rearrangeType == "arbscrunchy") {
		enoughY(moveseq);
		scrunchYTarget(moveseq);
	}
	else if (rearrangeType == "arbscrunchycompressx")
	{
		enoughY(moveseq);
		scrunchYTarget(moveseq);
		compressX(moveseq);
	}
	else if (rearrangeType == "arbscrunchycompressx2")
	{
		enoughY(moveseq);
		scrunchYTarget(moveseq);
		compressX2(moveseq);
	}
	else if (rearrangeType == "filterarbscrunchy")
	{
		enoughY(moveseq, true);
		scrunchYTarget(moveseq, true);
		filterReservoir(moveseq);
	}
	else if (rearrangeType == "filterarbscrunchycompressx")
	{
		enoughY(moveseq, true);
		scrunchYTarget(moveseq, true);
		filterReservoir(moveseq);
		compressX(moveseq);
	}
	else if (rearrangeType == "filterarbscrunchycompressx2")
	{
		enoughY(moveseq, true);
		scrunchYTarget(moveseq, true);
		compressX(moveseq);
		filterReservoir(moveseq);
	}
	else if (rearrangeType == "centerscrunchx") {
		scrunchX(moveseq, true);
	}
	else if (rearrangeType == "centerscrunchy") {
		scrunchY(moveseq, true);
	}
	else if (rearrangeType == "scrunchxy") {
		scrunchX(moveseq);

		//Update initial atom locations appropriately.
		int nAtomsInRow = moveParam.nTweezerX;
		int nGap = 0;
		for (int ix = 0; ix < positionsX.size(); ix++) {
			positionsX[ix] = false;
			if (ix >= nGap && ix % moveParam.scrunchSpacing == 0 && ix < nGap + moveParam.scrunchSpacing * nAtomsInRow) {
				positionsX[ix] = true;
			}
		}

		scrunchY(moveseq);
	}
	else if (rearrangeType == "scrunchyx") {
		scrunchY(moveseq);

		int nAtomsInRow = moveParam.nTweezerY;
		int nGap = 0;
		for (int iy = 0; iy < positionsY.size(); iy++) {
			positionsY[iy] = false;
			if (iy >= nGap && iy % moveParam.scrunchSpacing == 0 && iy < nGap + moveParam.scrunchSpacing * nAtomsInRow) {
				positionsY[iy] = true;
			}
		}

		scrunchX(moveseq);
	}
	else if (rearrangeType == "centerscrunchyx") {
		scrunchY(moveseq, true);

		int nAtomsInRow = moveParam.nTweezerY;
		int nGap = positionsY.size() / 2 - moveParam.scrunchSpacing * (nAtomsInRow / 2);
		for (int iy = 0; iy < positionsY.size(); iy++) {
			positionsY[iy] = false;
			if (iy >= nGap && iy % moveParam.scrunchSpacing == 0 && iy < nGap + moveParam.scrunchSpacing * nAtomsInRow) {
				positionsY[iy] = true;
			}
		}

		scrunchX(moveseq, true);
	}
	/*stuff below is not fully tested, be ware*/
	else if (rearrangeType == "equaltetris") {
		int nPerColumn = equalizeY(moveseq);
		scrunchYFixedLength(moveseq, nPerColumn);

		const unsigned wx = positionsX.size();
		const unsigned wy = positionsY.size();
		auto targetPositionsTemp = moveParam.targetPositions;
		auto imageTemp = atomImage.image;

		int iySource = moveParam.scrunchSpacing * (nPerColumn - 1); //Start taking atoms from top-most scrunched row, iterate towards bottom.
		int nRowSource = sourceRowSum(iySource, imageTemp); //number of atoms remaining in source row
		int iyTarget = wy - 1;

		//iterate through target rows from the opposite side from scrunch. iyTarget must be int so that it can go negative for this condition to fail.
		while (iyTarget >= 0 && iySource >= 0) {
			int ixTarget = 0;
			int ixSource = 0;
			moveSingle single;
			//iterate through all target positions in row
			while (ixTarget < wx) {
				//if atom required at target
				if (targetPositionsTemp[ixTarget + wx * iyTarget]) {
					//check if a source atom is available in current source row
					if (nRowSource < 1) {
						//if no atoms, move to the next source row and reset available atoms in row
						// TODO: THERE IS A POTENTIAL BUG - ZZP
						// If iySource get changed within while (ixTarget < wx),  the single.startAOY.push_back(iySource) could be pushing wrong iySource
						iySource -= moveParam.scrunchSpacing; 
						if (iySource >= 0) {
							nRowSource = sourceRowSum(iySource, imageTemp);
						}
						break;
					}
					//find next atom in source row. This should be guaranteed due to counting nRowSource.
					while (!imageTemp[ixSource + wx * iySource]) {
						ixSource++;
					}
					single.startAOX.push_back(ixSource);
					imageTemp[ixSource + wx * iySource] = 0; //erase used source position.
					nRowSource--; //remove an atom from the source row

					single.endAOX.push_back(ixTarget); //place tweezer at desired final location.
					targetPositionsTemp[ixTarget + wx * iyTarget] = 0; //erase filled target position.
				}
				ixTarget++; //iterate through target positions if no atom needed, or atom has been placed in target site.
				//if no source, this loops breaks, and continues from the previous target position.
			}
			if (iySource < 0) {
				//TODO: work out why this is getting triggered. Should always have enough atoms.
				break;
			}
			if (single.nx() > 0) {
				single.startAOY.push_back(iySource); //tweezers at desired source row
				single.endAOY.push_back(iyTarget); //tweezers at desired target row

				moveseq.moves.push_back(single); //add the move to the sequence, which either fully populates a target row, or fully depletes a source row.
			}
			if (ixTarget >= wx) {
				//only iterate to next target row if entire row has been populated/checked.
				iyTarget--;
			}
		}
	}
	else if (rearrangeType == "equaltetris2")
	{
		int nPerColumn = equalizeY(moveseq);
		scrunchYFixedLength(moveseq, nPerColumn);

		const unsigned wx = positionsX.size();
		const unsigned wy = positionsY.size();
		auto targetPositionsTemp = moveParam.targetPositions;
		auto imageTemp = atomImage.image;

		int iySource = moveParam.scrunchSpacing * (nPerColumn - 1); //Start taking atoms from top-most scrunched row, iterate towards bottom.
		int nRowSource = sourceRowSum(iySource, imageTemp); //number of atoms remaining in source row
		int iyTarget = wy - 1;

		//iterate through target rows from the opposite side from scrunch. iyTarget must be int so that it can go negative for this condition to fail.
		while (iyTarget >= 0 && iySource >= 0) {
			int ixTarget = 0;
			int ixSource = 0;
			moveSingle fromReservoir, scrunch, toTarget;

			//iterate through all target positions in row
			while (ixTarget < wx) {
				//if atom required at target
				if (targetPositionsTemp[ixTarget + wx * iyTarget]) {
					//check if a source atom is available in current source row
					if (nRowSource < 1) {
						//if no atoms, move to the next source row and reset available atoms in row
						iySource -= moveParam.scrunchSpacing;
						if (iySource >= 0) {
							nRowSource = sourceRowSum(iySource, imageTemp);
						}
						break;
					}
					//find next atom in source row. This should be guaranteed due to counting nRowSource.
					while (!imageTemp[ixSource + wx * iySource]) {
						ixSource++;
					}
					fromReservoir.startAOX.push_back(ixSource);
					fromReservoir.endAOX.push_back(ixSource);
					scrunch.startAOX.push_back(ixSource);

					imageTemp[ixSource + wx * iySource] = 0; //erase used source position.
					nRowSource--; //remove an atom from the source row

					scrunch.endAOX.push_back(ixTarget); //place tweezer at desired final location.
					toTarget.startAOX.push_back(ixTarget);
					toTarget.endAOX.push_back(ixTarget);

					targetPositionsTemp[ixTarget + wx * iyTarget] = 0; //erase filled target position.
				}
				ixTarget++; //iterate through target positions if no atom needed, or atom has been placed in target site.
				//if no source, this loops breaks, and continues from the previous target position.
			}
			if (iySource < 0) {
				//TODO: work out why this is getting triggered. Should always have enough atoms.
				break;
			}
			if (fromReservoir.nx() > 0) {
				fromReservoir.startAOY.push_back(iySource); //tweezers at desired source row
				fromReservoir.endAOY.push_back(iySource + (iyTarget - iySource) / 2); //tweezers at desired target row

				scrunch.startAOY.push_back(iySource + (iyTarget - iySource) / 2);
				scrunch.endAOY.push_back(iySource + (iyTarget - iySource) / 2);

				toTarget.startAOY.push_back(iySource + (iyTarget - iySource) / 2);
				toTarget.endAOY.push_back(iyTarget);

				moveseq.moves.push_back(fromReservoir);
				moveseq.moves.push_back(scrunch);
				moveseq.moves.push_back(toTarget); //add the move to the sequence, which either fully populates a target row, or fully depletes a source row.
			}
			if (ixTarget >= wx) {
				//only iterate to next target row if entire row has been populated/checked.
				iyTarget--;
			}
		}
	}
	else if (rearrangeType == "tetris") {
		const unsigned wx = positionsX.size();
		const unsigned wy = positionsY.size();
		auto targetPositionsTemp = moveParam.targetPositions;
		auto imageTemp = atomImage.image;

		unsigned maxAtomRow = 0;
		unsigned numAtomRow = 0;
		//Find which row has most atoms.
		for (int iy = 0; iy < positionsY.size(); iy++) {
			numAtomRow = 0;
			for (int ix = 0; ix < positionsX.size(); ix++) {
				if (imageTemp[ix + wx * iy]) {
					numAtomRow++;
				}
			}
			if (numAtomRow > maxAtomRow) {
				maxAtomRow = numAtomRow;
			}
		}

		//scrunchX(moveseq); //scrunch atoms to left side (index 0 side)
		////// This is slightly modified scrunchx to leave sharp edge to draw from.
		int iy = 0;
		for (auto const& channelBoolY : positionsY) {
			if (channelBoolY) {
				moveSingle single;
				int ix = 0;
				for (auto const& channelBoolX : positionsX) {
					if (channelBoolX && atomImage.image[ix + positionsX.size() * iy]) {
						single.startAOX.push_back(ix); //Place tweezers on all atoms in row
						atomImage.image[ix + positionsX.size() * iy] = 0; //remove atom from pickup location
					}
					ix++;
				}
				single.startAOY.push_back(iy); //Single tone in y
				single.endAOY.push_back(iy); //y does not move
				moveseq.moves.push_back(single);

				int nAtomsInRow = moveseq.moves.back().nx();
				int nGap = (maxAtomRow - nAtomsInRow) * moveParam.scrunchSpacing;
				for (int ix2 = 0; ix2 < nAtomsInRow; ix2++) {
					moveseq.moves.back().endAOX.push_back(nGap + moveParam.scrunchSpacing * ix2); //Bunch up tweezers in center of row
					atomImage.image[nGap + moveParam.scrunchSpacing * ix2 + positionsX.size() * iy] = 1; //place atom in dropoff location
				};
			}
			iy++;
		}
		//////

		int ixSource = moveParam.scrunchSpacing * (maxAtomRow - 1); //Start taking atoms from right-most scrunched column, iterate towards left.
		int nColumnSource = sourceColumnSum(ixSource, imageTemp); //number of atoms remaining in source column
		int ixTarget = wx - 1;

		//iterate through target columns from the opposite side from scrunch. ixTarget must be int so that it can go negative for this condition to fail.
		while (ixTarget >= 0 && ixSource >= 0) {
			int iyTarget = 0;
			int iySource = 0;
			moveSingle single;

			//iterate through all target positions in column
			while (iyTarget < wy) {
				//if atom required at target
				if (targetPositionsTemp[ixTarget + wx * iyTarget]) {
					//check if a source atom is available in current source column
					if (nColumnSource < 1) {
						//if no atoms, move to the next source column and reset available atoms in column
						ixSource -= moveParam.scrunchSpacing;
						if (ixSource >= 0){
							nColumnSource = sourceColumnSum(ixSource, imageTemp);
						}
						break;
					}
					//find next atom in source column. This should be guaranteed due to counting nColumnSource.
					while (!imageTemp[ixSource + wx * iySource]) {
						iySource++;
					}
					single.startAOY.push_back(iySource);
					imageTemp[ixSource + wx * iySource] = 0; //erase used source position.
					nColumnSource--; //remove an atom from the source column

					single.endAOY.push_back(iyTarget); //place tweezer at desired final location.
					targetPositionsTemp[ixTarget + wx * iyTarget] = 0; //erase filled target position.
				}
				iyTarget++; //iterate through target positions if no atom needed, or atom has been placed in target site.
				//if no source, this loops breaks, and continues from the previous target position.
			}
			if (ixSource < 0) {
				break;
			}
			if (single.ny() > 0) {
				single.startAOX.push_back(ixSource); //tweezers at desired source column
				single.endAOX.push_back(ixTarget); //tweezers at desired target column

				moveseq.moves.push_back(single); //add the move to the sequence, which either fully populates a target column, or fully depletes a source column.
			}
			//only iterate to next target column if entire column has been populated/checked.
			if (iyTarget >= wy) {
				ixTarget--;
			}
		}
	}
	else if (rearrangeType == "tetris2")
	{
		const unsigned wx = positionsX.size();
		const unsigned wy = positionsY.size();
		auto targetPositionsTemp = moveParam.targetPositions;
		auto imageTemp = atomImage.image;

		unsigned maxAtomRow = 0;
		unsigned numAtomRow = 0;
		//Find which row has most atoms.
		for (int iy = 0; iy < positionsY.size(); iy++) {
			numAtomRow = 0;
			for (int ix = 0; ix < positionsX.size(); ix++) {
				if (imageTemp[ix + wx * iy]) {
					numAtomRow++;
				}
			}
			if (numAtomRow > maxAtomRow) {
				maxAtomRow = numAtomRow;
			}
		}

		//scrunchX(moveseq); //scrunch atoms to left side (index 0 side)
		////// This is slightly modified scrunchx to leave sharp edge to draw from.
		int iy = 0;
		for (auto const& channelBoolY : positionsY) {
			if (channelBoolY) {
				moveSingle single;
				int ix = 0;
				for (auto const& channelBoolX : positionsX) {
					if (channelBoolX && atomImage.image[ix + positionsX.size() * iy]) {
						single.startAOX.push_back(ix); //Place tweezers on all atoms in row
						atomImage.image[ix + positionsX.size() * iy] = 0; //remove atom from pickup location
					}
					ix++;
				}
				single.startAOY.push_back(iy); //Single tone in y
				single.endAOY.push_back(iy); //y does not move
				moveseq.moves.push_back(single);

				int nAtomsInRow = moveseq.moves.back().nx();
				int nGap = (maxAtomRow - nAtomsInRow) * moveParam.scrunchSpacing;
				for (int ix2 = 0; ix2 < nAtomsInRow; ix2++) {
					moveseq.moves.back().endAOX.push_back(nGap + moveParam.scrunchSpacing * ix2); //Bunch up tweezers in center of row
					atomImage.image[nGap + moveParam.scrunchSpacing * ix2 + positionsX.size() * iy] = 1; //place atom in dropoff location
				};
			}
			iy++;
		}
		//////

		int ixSource = moveParam.scrunchSpacing * (maxAtomRow - 1); //Start taking atoms from right-most scrunched column, iterate towards left.
		int nColumnSource = sourceColumnSum(ixSource, imageTemp); //number of atoms remaining in source column
		int ixTarget = wx - 1;

		while (ixTarget >= 0 && ixSource >= 0) //iterate through target columns from the opposite side from scrunch. ixTarget must be int so that it can go negative for this condition to fail.
		{
			int iyTarget = 0;
			int iySource = 0;
			moveSingle fromReservoir, scrunch, toTarget;
			//iterate through all target positions in column
			while (iyTarget < wy) {
				//if atom required at target
				if (targetPositionsTemp[ixTarget + wx * iyTarget]) {
					//check if a source atom is available in current source column
					if (nColumnSource < 1) {
						ixSource -= moveParam.scrunchSpacing; //if no atoms, move to the next source column and reset available atoms in column
						if (ixSource >= 0) {
							nColumnSource = sourceColumnSum(ixSource, imageTemp);
						}
						break;
					}
					//find next atom in source column. This should be guaranteed due to counting nColumnSource.
					while (!imageTemp[ixSource + wx * iySource]) {
						iySource++;
					}
					fromReservoir.startAOY.push_back(iySource);
					fromReservoir.endAOY.push_back(iySource);
					scrunch.startAOY.push_back(iySource);

					imageTemp[ixSource + wx * iySource] = 0; //erase used source position.
					nColumnSource--; //remove an atom from the source column

					scrunch.endAOY.push_back(iyTarget); //place tweezer at desired final location.
					toTarget.startAOY.push_back(iyTarget);
					toTarget.endAOY.push_back(iyTarget);

					targetPositionsTemp[ixTarget + wx * iyTarget] = 0; //erase filled target position.
				}
				iyTarget++; //iterate through target positions if no atom needed, or atom has been placed in target site.
				//if no source, this loops breaks, and continues from the previous target position.
			}
			if (ixSource < 0) {
				//TODO: work out why this is getting triggered. Should always have enough atoms.
				break;
			}
			if (fromReservoir.ny() > 0) {
				fromReservoir.startAOX.push_back(ixSource); //tweezers at desired source column
				fromReservoir.endAOX.push_back(ixSource + (ixTarget - ixSource) / 2); //tweezers half way to target column.
				scrunch.startAOX.push_back(ixSource + (ixTarget - ixSource) / 2); //scrunch occurs at half way point.
				scrunch.endAOX.push_back(ixSource + (ixTarget - ixSource) / 2);
				toTarget.startAOX.push_back(ixSource + (ixTarget - ixSource) / 2);
				toTarget.endAOX.push_back(ixTarget);


				moveseq.moves.push_back(fromReservoir); //add the move to the sequence, which either fully populates a target column, or fully depletes a source column.
				moveseq.moves.push_back(scrunch);
				moveseq.moves.push_back(toTarget);
			}
			if (iyTarget >= wy) {
				//only iterate to next target column if entire column has been populated/checked.
				ixTarget--;
			}
		}
	}
	else {
		thrower("Invalid rearrangement mode.");
	}

	return moveseq;
}

void RearrangeGenerator::filterAtomQueue()
{	
	//exclude atoms that were not on a target site.
	const auto positions = moveParam.initialPositions;
	for (int i = 0; i < positions.size(); i++) {
		atomImage.image[i] = atomImage.image[i] * (positions)[i];
	}
}

int RearrangeGenerator::sourceColumnSum(int iColumn, const std::vector<bool>& atomImg)
{
	int nColumnSource = 0; //number of atoms remaining in source column
	const auto& positionsX = moveParam.initialPositionsX;
	const auto& positionsY = moveParam.initialPositionsY;
	for (int iy = 0; iy < (positionsY.size()); iy++) {
		if (atomImg[iColumn + (positionsX.size()) * iy]) {
			nColumnSource++;
		}
	}

	return nColumnSource;
}

int RearrangeGenerator::sourceRowSum(int iRow, const std::vector<bool>& atomImg)
{
	const auto& positionsX = moveParam.initialPositionsX;
	const auto& positionsY = moveParam.initialPositionsY;
	int nRowSource = 0; //number of atoms remaining in source row
	for (int ix = 0; ix < positionsX.size(); ix++) {
		if (atomImg[ix + positionsX.size() * iRow]) {
			nRowSource++;
		}
	}

	return nRowSource;
}

int RearrangeGenerator::equalizeY(moveSequence& moveseq)
{
	// Equalize the number of atoms in each column. Returns target atoms/column
	const auto& positionsX = moveParam.initialPositionsX;
	const auto& positionsY = moveParam.initialPositionsY;
	// Calculate mean atom number per column.
	int nxMean = 0;
	std::vector<int> nxList;
	for (auto const& coordX : positionCoordinatesX) {
		int nx = 0;
		for (auto const& coordY : positionCoordinatesY) {
			if (atomImage.image[coordX + positionsX.size() * coordY])
				nx++;
		}
		nxMean += nx;
		nxList.push_back(nx);
	}
	nxMean /= (moveParam.nTweezerX);
	for (auto& element : nxList) // nxList now contains excess atoms.
		element -= nxMean;

	//iterate through columns, and move to adjacent column as needed.
	int ixTweezer = 0; //tweezer index, can step by multiple lattice sites.
	for (auto const& coordX : positionCoordinatesX)
	{
		if (ixTweezer >= positionCoordinatesX.size() - 1) {
			break; //end on second last load column.
		}
		if (nxList[ixTweezer] > 0) {
			// move excess atoms out of column
			moveSingle single;
			for (auto const& coordY : positionCoordinatesY) {
				if (atomImage.image[coordX + positionsX.size() * coordY]
					&& !atomImage.image[positionCoordinatesX[ixTweezer + 1] + (positionsX.size()) * coordY]) {
					// move only if site in adjacent loaded column is empty.
					single.startAOY.push_back(coordY); //select atoms in column to move
					single.endAOY.push_back(coordY);
					atomImage.image[coordX + positionsX.size() * coordY] = 0;
					atomImage.image[positionCoordinatesX[ixTweezer + 1] + positionsX.size() * coordY] = 1; //remove atom from pickup location and place in target
					nxList[ixTweezer]--;
					nxList[ixTweezer + 1]++; //keep track of column atom numbers
				}
				if (nxList[ixTweezer] <= 0) {
					break; //stop if all excess atoms have been moved.
				}
			}
			single.startAOX.push_back(coordX); //Single tone in x
			single.endAOX.push_back(positionCoordinatesX[ixTweezer + 1]); //x moves to next load column
			if (single.ny() > 0) { moveseq.moves.push_back(single); }
		}
		if (nxList[ixTweezer] < 0) {
			// pull missing atoms into column
			moveSingle single;
			for (auto const& coordY : positionCoordinatesY) {
				if (!atomImage.image[coordX + positionsX.size() * coordY]
					&& atomImage.image[positionCoordinatesX[ixTweezer + 1] + positionsX.size() * coordY]) {
					// move only if site in adjacent loaded column is full.
					single.startAOY.push_back(coordY); //select atoms in column to move
					single.endAOY.push_back(coordY);
					atomImage.image[coordX + (positionsX.size()) * coordY] = 1;
					atomImage.image[positionCoordinatesX[ixTweezer + 1] + (positionsX.size()) * coordY] = 0; //remove atom from pickup location and place in target
					nxList[ixTweezer]++;
					nxList[ixTweezer + 1]--; //keep track of column atom numbers
				}
				if (nxList[ixTweezer] >= 0)
				{
					break; //stop if all excess atoms have been moved.
				}
			}
			single.startAOX.push_back(positionCoordinatesX[ixTweezer + 1]); //x moves from next load column
			single.endAOX.push_back(coordX); //Single tone in x
			if (single.ny() > 0) { moveseq.moves.push_back(single); }
		}
		ixTweezer++;
	}
	return nxMean;
}

void RearrangeGenerator::enoughY(moveSequence& moveseq, bool constantMoves)
{
	//Ensure the number of atoms in each column is sufficient for target pattern.
	const auto& positionsX = moveParam.initialPositionsX;
	const auto& positionsY = moveParam.initialPositionsY;
	const auto& targetPositions = moveParam.targetPositions;
	std::vector<int> nxList;
	int ix = 0;
	for (auto const& channelBoolX : positionsX) {
		if (channelBoolX) {
			int iy = 0;
			int nx = 0;
			for (auto const& channelBoolY : positionsY) {
				if (channelBoolY && atomImage.image[ix + positionsX.size() * iy]) {
					nx++; //count atoms present
				}
				if (targetPositions[ix + positionsX.size() * iy]) {
					nx--; //remove atoms that are needed in target state
				}
				iy++;
			}
			nxList.push_back(nx); // nxList contains excess atoms.
		}
		ix++;
	}

	//iterate through columns, and move to adjacent column as needed.
	int ixTweezer = 0; //tweezer index, can step by multiple lattice sites.
	for (auto const& coordX : positionCoordinatesX)
	{
		if (ixTweezer >= positionCoordinatesX.size() - 1) {
			break; //end on second last load column.
		}
		if (nxList[ixTweezer] > 0) {
			// move excess atoms out of column if next column needs atoms.
			moveSingle single;
			for (auto const& coordY : positionCoordinatesY) {
				if (atomImage.image[coordX + positionsX.size() * coordY]
					&& !(atomImage.image[positionCoordinatesX[ixTweezer + 1] + positionsX.size() * coordY])
					&& nxList[ixTweezer + 1] < 0) {
					// move only if site in adjacent loaded column is empty, and adjacent column needs atoms.
					single.startAOY.push_back(coordY); //select atoms in column to move
					single.endAOY.push_back(coordY);
					atomImage.image[coordX + (positionsX.size()) * coordY] = 0;
					atomImage.image[positionCoordinatesX[ixTweezer + 1] + positionsX.size() * coordY] = 1; //remove atom from pickup location and place in target
					nxList[ixTweezer]--;
					nxList[ixTweezer + 1]++; //keep track of column atom numbers
				}
				if (nxList[ixTweezer] <= 0) {
					break; //stop if all excess atoms have been moved.
				}
			}
			single.startAOX.push_back(coordX); //Single tone in x
			single.endAOX.push_back(positionCoordinatesX[ixTweezer + 1]); //x moves to next load column
			if (single.ny() > 0 || constantMoves) { moveseq.moves.push_back(single); } //If constant number of moves are needed, always add the move, even if it's empty.
		}
		else if (nxList[ixTweezer] < 0) {
			// pull missing atoms into column.
			moveSingle single;
			for (auto const& coordY : positionCoordinatesY) {
				if (!atomImage.image[coordX + (positionsX.size()) * coordY]
					&& (atomImage.image[positionCoordinatesX[ixTweezer + 1] + positionsX.size() * coordY])) {
					// move only if site in adjacent loaded column is full.
					single.startAOY.push_back(coordY); //select atoms in column to move
					single.endAOY.push_back(coordY);
					atomImage.image[coordX + (positionsX.size()) * coordY] = 1;
					atomImage.image[positionCoordinatesX[ixTweezer + 1] + (positionsX.size()) * coordY] = 0; //remove atom from pickup location and place in target
					nxList[ixTweezer]++;
					nxList[ixTweezer + 1]--; //keep track of column atom numbers
				}
				if (nxList[ixTweezer] >= 0) {
					break; //stop if all excess atoms have been moved.
				}
			}
			single.startAOX.push_back(positionCoordinatesX[ixTweezer + 1]); //x moves from next load column
			single.endAOX.push_back(coordX); //Single tone in x
			if (single.ny() > 0 || constantMoves) { moveseq.moves.push_back(single); }
		}
		else if (constantMoves) {
			moveSingle single;
			single.startAOX.push_back(coordX); //dummy move to maintain constant move number.
			single.endAOX.push_back(coordX);
			moveseq.moves.push_back(single);
		}
		ixTweezer++;
	}
}

void RearrangeGenerator::scrunchX(moveSequence& moveseq, bool centered)
{
	const auto& positions = moveParam.initialPositions;
	const auto& positionsX = moveParam.initialPositionsX;
	const auto& positionsY = moveParam.initialPositionsY;

	int iy = 0;
	for (auto const& channelBoolY : positionsY)
	{
		if (channelBoolY)  {
			//If first step, choose the rows with load tweezers. If not first step, choose the rows that atoms were scrunched to.
			moveSingle single;
			int ix = 0;
			for (auto const& channelBoolX : positionsX) {
				//Place tweezers on all atoms (picked up by initialPositionX and atom existance in image) in row
				if (channelBoolX && atomImage.image[ix + positionsX.size() * iy]) {
					single.startAOX.push_back(ix); 
					atomImage.image[ix + (positionsX.size()) * iy] = 0; //remove atom from pickup location
				}
				ix++;
			}
			single.startAOY.push_back(iy); //Single tone in y
			single.endAOY.push_back(iy); //y does not move
			moveseq.moves.push_back(single);

			int nAtomsInRow = moveseq.moves.back().nx();
			int nGap = 0;
			if (centered) {
				nGap = positionsX.size() / 2 - moveParam.scrunchSpacing * (nAtomsInRow / 2);
				nGap = (nGap < 0) ? 0 : nGap;
			}
			for (int ix2 = 0; ix2 < nAtomsInRow; ix2++) {
				if (nGap + moveParam.scrunchSpacing * ix2 >= positionsX.size()) {
					moveseq.moves.back().endAOX.push_back(-2); // remove atom from higher frequency side
				}
				else {
					moveseq.moves.back().endAOX.push_back(nGap + moveParam.scrunchSpacing * ix2); //Bunch up tweezers in center of row
					atomImage.image[nGap + moveParam.scrunchSpacing * ix2 + positionsX.size() * iy] = 1; //place atom in dropoff location
				}
			};
		}
		iy++;
	}
}

void RearrangeGenerator::scrunchY(moveSequence& moveseq, bool centered)
{
	const auto& positions = moveParam.initialPositions;
	const auto& positionsX = moveParam.initialPositionsX;
	const auto& positionsY = moveParam.initialPositionsY;
	int ix = 0;
	for (auto const& channelBoolX : positionsX)
	{
		if (channelBoolX) {
			//If first step, choose the rows with load tweezers. If not first step, choose the rows that atoms were scrunched to.
			moveSingle single;
			int iy = 0;
			for (auto const& channelBoolY : positionsY) {
				if (channelBoolY && atomImage.image[ix + positionsX.size() * iy]) {
					single.startAOY.push_back(iy); //Place tweezers on all atoms in row
					atomImage.image[ix + positionsX.size() * iy] = 0; //remove atom from pickup location
				}
				iy++;
			}
			single.startAOX.push_back(ix); //Single tone in x
			single.endAOX.push_back(ix); //x does not move
			moveseq.moves.push_back(single);

			int nAtomsInRow = moveseq.moves.back().ny();
			int nGap = 0;
			if (centered) {
				nGap = positionsY.size() / 2 - moveParam.scrunchSpacing * (nAtomsInRow / 2);
				nGap = (nGap < 0) ? 0 : nGap;
			}
			for (int iy2 = 0; iy2 < nAtomsInRow; iy2++) {
				if (nGap + (moveParam.scrunchSpacing) * iy2 >= positionsY.size()) {
					moveseq.moves.back().endAOY.push_back(-2); // remove atom from higher frequency side
				}
				else {
					moveseq.moves.back().endAOY.push_back(nGap + moveParam.scrunchSpacing * iy2); //Bunch up tweezers in center of row
					atomImage.image[ix + positionsX.size() * (nGap + iy2 * moveParam.scrunchSpacing)] = 1; //place atom in dropoff location
				}
			};
		}
		ix++;
	}

}

void RearrangeGenerator::scrunchYFixedLength(moveSequence& moveseq, int nPerColumn, bool centered)
{
	const auto& positionsX = moveParam.initialPositionsX;
	const auto& positionsY = moveParam.initialPositionsY;
	int ix = 0;
	for (auto const& channelBoolX : positionsX) {
		if (channelBoolX) {
			//If first step, choose the rows with load tweezers. If not first step, choose the rows that atoms were scrunched to.
			moveSingle single;
			int iy = 0;
			for (auto const& channelBoolY : positionsY) {
				if (channelBoolY && atomImage.image[ix + positionsX.size() * iy]) {
					single.startAOY.push_back(iy); //Place tweezers on all atoms in row
					atomImage.image[ix + positionsX.size() * iy] = 0; //remove atom from pickup location
				}
				iy++;
			}
			single.startAOX.push_back(ix); //Single tone in x
			single.endAOX.push_back(ix); //x does not move
			moveseq.moves.push_back(single);

			int nAtomsInRow = moveseq.moves.back().ny();
			int nGap = 0;
			if (centered) {
				nGap = positionsY.size() / 2 - moveParam.scrunchSpacing * (nPerColumn / 2);
				nGap = (nGap < 0) ? 0 : nGap;
			}
			for (int iy2 = 0; iy2 < nAtomsInRow; iy2++) {
				if (iy2 < nAtomsInRow - nPerColumn) {
					moveseq.moves.back().endAOY.push_back(-1); // remove atom from lower frequency side
				}
				else if (nGap + moveParam.scrunchSpacing * (iy2 - (nAtomsInRow - nPerColumn)) >= positionsY.size()) {
					moveseq.moves.back().endAOY.push_back(-2); // remove atom from higher frequency side
				}
				else {
					moveseq.moves.back().endAOY.push_back(nGap + moveParam.scrunchSpacing * (iy2 - (nAtomsInRow - nPerColumn))); //Bunch up tweezers, offset to compensate for removed atoms.
					atomImage.image[ix + (positionsX.size()) * (nGap + iy2 * moveParam.scrunchSpacing)] = 1; //place atom in dropoff location
				}
			};
		}
		ix++;
	}
}

void RearrangeGenerator::scrunchYTarget(moveSequence& moveseq, bool constantMoves)
{
	const auto& positionsX = moveParam.initialPositionsX;
	const auto& positionsY = moveParam.initialPositionsY;
	auto targetPositionsTemp = moveParam.targetPositions;
	const unsigned wx = positionsX.size();
	const unsigned wy = positionsY.size();

	int ix = 0;
	for (auto const& channelBoolX : positionsX) {
		if (channelBoolX) {
			int nxTarget = 0;
			for (int iy = 0; iy < wy; iy++) {
				//count number of target sites in column.
				if (targetPositionsTemp[ix + wx * iy]) {
					nxTarget++;
				}
			}

			moveSingle single;
			int iy = 0;
			for (auto const& channelBoolY : positionsY) {
				if (channelBoolY && atomImage.image[ix + wx * iy]) {
					single.startAOY.push_back(iy); //Place tweezers on all atoms in column
					atomImage.image[ix + wx * iy] = 0; //remove atom from pickup location
				}
				iy++;
			}
			single.startAOX.push_back(ix); //Single tone in x
			single.endAOX.push_back(ix); //x does not move
			if (single.ny() > 0 || constantMoves) { moveseq.moves.push_back(single); }

			int nAtomsInRow = moveseq.moves.back().ny();
			int iyTarget = 0;
			for (int iy2 = 0; iy2 < nAtomsInRow; iy2++) {
				if (iy2 < (nAtomsInRow - nxTarget) / 2) {
					//moveseq.moves.back().endAOY.push_back(-1); // remove atom from lower frequency side
					moveseq.moves.back().endAOY.push_back(iy2); // same as above?
				}
				else if (nxTarget > 0) {
					while (targetPositionsTemp[ix + wx * iyTarget] == 0) { iyTarget++; } //iterate to next target site
					moveseq.moves.back().endAOY.push_back(iyTarget); //Move tweezer to target site
					atomImage.image[ix + wx * iyTarget] = 1; //place atom in dropoff location
					targetPositionsTemp[ix + wx * iyTarget] = 0; //remove target site
					nxTarget--;
				}
				else {
					//moveseq.moves.back().endAOY.push_back(-2); // remove atom from higher frequency side
					moveseq.moves.back().endAOY.push_back(wy - (nAtomsInRow - iy2) - 2); // same as above but two sites away from edge?
				}
			}
		}
		ix++;
	}
}

void RearrangeGenerator::compressX(moveSequence& moveseq)
{
	//Change the spacing between columns of target pattern by moving entire columns at a time.
	const auto& positionsX = moveParam.initialPositionsX;
	const auto& positionsY = moveParam.initialPositionsY;

	unsigned iCenter = positionCoordinatesX.size() / 2; //central load column index
	unsigned coordXCenter = positionCoordinatesX[iCenter];

	//iterate through columns, and move to adjacent column as needed.
	int coordX = 0;
	int coordXtarget = 0;
	for (int i = 1; i < positionCoordinatesX.size(); i++) {
		//iterate through all but central column
		if (i + iCenter < positionCoordinatesX.size()) {
			coordX = positionCoordinatesX[i + iCenter];
		}
		else {
			coordX = positionCoordinatesX[2 * iCenter - (i + 1)];
		}
		coordXtarget = coordXCenter + (coordX - coordXCenter) * moveParam.scrunchSpacing / 3; //Assuming load is 3x lattice spacing for now.
		moveSingle single;
		for (int iy = 0; iy < positionsY.size(); iy++) {
			if (atomImage.image[coordX + positionsX.size() * iy]
				&& moveParam.targetPositions[coordX + positionsX.size() * iy]) {
				// tweezers on all atoms that occupy target sites in column
				single.startAOY.push_back(iy); //select atoms in column to move
				single.endAOY.push_back(iy);
				atomImage.image[coordXtarget + positionsX.size() * iy] = 1;
				atomImage.image[coordX + positionsX.size() * iy] = 0; //remove atom from pickup location and place in target
			}
		}
		single.startAOX.push_back(coordX);
		single.endAOX.push_back(coordXtarget); //move entire column
		if (single.ny() > 0) { moveseq.moves.push_back(single); }
	}
}

void RearrangeGenerator::compressX2(moveSequence& moveseq)
{
	//Change the spacing between columns of target pattern by moving entire columns at a time.
	const auto& positionsX = moveParam.initialPositionsX;
	const auto& positionsY = moveParam.initialPositionsY;

	int iCenter = positionCoordinatesX.size() / 2; //central load column index
	int coordXCenter = positionCoordinatesX[iCenter];

	//iterate through columns, and move to adjacent column as needed.
	int coordX = 0;
	int coordXtarget = 0;
	for (int i = 1; i < positionCoordinatesX.size(); i++) {
		//iterate through all but central column
		if (i + iCenter < positionCoordinatesX.size()) {
			coordX = positionCoordinatesX[i + iCenter];
		}
		else {
			coordX = positionCoordinatesX[2 * iCenter - (i + 1)];
		}
		coordXtarget = coordXCenter + (coordX - coordXCenter) * moveParam.scrunchSpacing / 4;
		moveSingle single;
		for (int iy = 0; iy < positionsY.size(); iy++) {
			if (atomImage.image[coordX + positionsX.size() * iy]
				&& moveParam.targetPositions[coordX + positionsX.size() * iy]) {
				// tweezers on all atoms that occupy target sites in column
				single.startAOY.push_back(iy); //select atoms in column to move
				single.endAOY.push_back(iy);
				atomImage.image[coordXtarget + positionsX.size() * iy] = 1;
				atomImage.image[coordX + positionsX.size() * iy] = 0; //remove atom from pickup location and place in target
			}
		}
		single.startAOX.push_back(coordX);
		single.endAOX.push_back(coordXtarget); //move entire column
		if (single.ny() > 0) { moveseq.moves.push_back(single); }
	}
}

void RearrangeGenerator::filterReservoir(moveSequence& moveseq)
{
	///Place tweezers on all positions to be kept
	moveSingle single;
	int ix = 0;
	for (auto const& channelBoolX : moveParam.filterPositionsX) {
		if (channelBoolX) {
			single.startAOX.push_back(ix);
			single.endAOX.push_back(ix);
		}
		ix++;
	}

	int iy = 0;
	for (auto const& channelBoolY : moveParam.filterPositionsY) {
		if (channelBoolY) {
			single.startAOY.push_back(iy);
			single.endAOY.push_back(iy);
		}
		iy++;
	}

	moveseq.moves.push_back(single);
}

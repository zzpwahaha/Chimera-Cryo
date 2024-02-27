#include "stdafx.h"
#include <Rearrangement/AtomCruncherWorker.h>
#include <Rearrangement/atomCruncherInput.h>

CruncherThreadWorker::CruncherThreadWorker (std::unique_ptr<atomCruncherInput> input_) 
	: input(std::move(input_)) 
{}

CruncherThreadWorker::~CruncherThreadWorker () {
}

void CruncherThreadWorker::init () {
	auto gridSize = input->grids.size ();
	if (gridSize == 0) {
		return;
	}
	monitoredPixelIndecies.resize(gridSize);
	// preparing for the crunching
	for (auto gridInc : range (gridSize)) {
		auto& grid = input->grids[gridInc];
		if (grid.useFile) {
			try {
				atomGrid::loadGridFile(grid);
			}
			catch (ChimeraError& e) {
				emit error("Can not open grid file: " + qstr(grid.fileName) + ". Experiment would probably fail due to failure of realtime analysis.");
				return;
			}
			for (auto& locBlocks : grid.atomLocs) {
				std::vector<long> pixelIndices;
				for (auto& loc : locBlocks) {
					if (loc.row >= input->imageDims.heightBinned() || loc.column >= input->imageDims.widthBinned()) {
						emit error("atom grid appears to include pixels outside the image frame! Not allowed, seen by atom "
							"cruncher thread");
						return;
					}

					int index = ((loc.row)*input->imageDims.widthBinned() + loc.column);
					if (index >= input->imageDims.widthBinned() * input->imageDims.heightBinned()) {
						emit error("Math error! Somehow, the pixel indexes appear within bounds, but the calculated index"
							" is larger than the image is!  (A low level bug, this shouldn't happen)");
						return;
					}
					pixelIndices.push_back(index);
				}
				monitoredPixelIndecies[gridInc].push_back(pixelIndices);
			}
		}
		else {
			for (auto columnInc : range(grid.width)) {
				for (auto rowInc : range(grid.height)) {
					unsigned pixelRow = (grid.gridOrigin.row + rowInc * grid.pixelSpacingY);
					unsigned pixelColumn = (grid.gridOrigin.column + columnInc * grid.pixelSpacingX);
					unsigned pixelRowTmp, pixelColumnTmp;
					std::vector<long> pixelIndices;
					for (auto colIncl : range(2 * grid.includedPixelX + 1)) {
						for (auto rowIncl : range(2 * grid.includedPixelY + 1)) {
							pixelColumnTmp = pixelColumn + colIncl - grid.includedPixelX;
							pixelRowTmp = pixelRow + rowIncl - grid.includedPixelY;
							if (pixelRowTmp >= input->imageDims.heightBinned() || pixelColumnTmp >= input->imageDims.widthBinned() || 
								pixelRowTmp < 0 || pixelColumnTmp < 0) {
								emit error("atom grid appears to include pixels outside the image frame! Not allowed, seen by atom "
									"cruncher thread");
								return;
							}

							int index = ((pixelRowTmp) * input->imageDims.widthBinned() + pixelColumnTmp);
							if (index >= input->imageDims.widthBinned() * input->imageDims.heightBinned()) {
								emit error("Math error! Somehow, the pixel indexes appear within bounds, but the calculated index"
									" is larger than the image is!  (A low level bug, this shouldn't happen)");
								return;
							}
							pixelIndices.push_back(index);
						}
					}
					monitoredPixelIndecies[gridInc].push_back(pixelIndices);
				}
			}
		}
	}
	for (auto picThresholds : input->thresholds) {
		if (picThresholds.size () != 1 && picThresholds.size () != input->grids[0].numAtoms ()) {
			emit error (cstr ("the list of thresholds isn't size 1 (constant) or the size of the number of atoms in the "
				"first grid! Size is " + str (picThresholds.size ()) + "and grid size is " +
				str (input->grids[0].numAtoms ())));
			return;
		}
	}
	imageCount = 0;

	handleImage();
}


void CruncherThreadWorker::handleImage (){
	// loop watching the image queue.
	while (true) {
		//if (imageCount % 2 == 0) {
		//	input->catchPicTime->push_back (chronoClock::now ());
		//}
		auto image = input->imageQueue->pop();
		if (!(*input->cruncherThreadActive)) {
			break; // signals for exiting this function so that the thread can be released
		}
		// tempImagePixels[grid][pixel]; only contains the counts for the pixels being monitored.
		PixListQueue tempImagePixels(input->grids.size());
		// tempAtomArray[grid][pixel]; only contains the boolean true/false of whether an atom passed a threshold or not. 
		atomQueue tempAtomArray(input->grids.size());
		for (auto gridInc : range(input->grids.size())) {
			tempAtomArray[gridInc].image = std::vector<bool>(monitoredPixelIndecies[gridInc].size());
			tempImagePixels[gridInc].image = std::vector<double>(monitoredPixelIndecies[gridInc].size(), 0.0);
		}
		for (auto gridInc : range(input->grids.size())) {
			tempImagePixels[gridInc].picStat = image.picStat;
			tempAtomArray[gridInc].picStat = image.picStat;
			for (auto atomInc : range(monitoredPixelIndecies[gridInc].size())) {
				///*** Deal with 1st element entirely first, as this is important for the rearranger thread and the 
				/// load-skip both of which are very time-sensitive.
				for (auto pixelIndex : monitoredPixelIndecies[gridInc][atomInc]) {
					if (pixelIndex > image.image.size()) {
						emit error("Monitored pixel indecies included pixel out of image?!?! pixel: " + qstr(pixelIndex)
							+ ", size: " + qstr(image.image.size()));
						// should I return here?
					}
					else {
						tempImagePixels[gridInc].image[atomInc] += image.image.data[pixelIndex];
					}
				}
				tempImagePixels[gridInc].image[atomInc] /= monitoredPixelIndecies[gridInc][atomInc].size();
			}

			unsigned count = 0;
			for (auto& pix : tempImagePixels[gridInc].image) {
				auto& picThresholds = input->thresholds[imageCount % input->picsPerRep];
				if (pix >= picThresholds[count % picThresholds.size()]) {
					tempAtomArray[gridInc].image[count] = true;
				}
				count++;
			}
			// explicitly deal with the rearranger thread and load skip as soon as possible, these are time-critical.
			if (gridInc == 0) {
				// if last picture of repetition, check for loadskip condition.
				if (imageCount % input->picsPerRep == input->picsPerRep - 1) {
					unsigned numAtoms = std::accumulate(tempAtomArray[0].image.begin(), tempAtomArray[0].image.end(), 0);
					*input->skipNext = (numAtoms >= input->atomThresholdForSkip);
				}
			}
		}
		emit atomArray(tempAtomArray);
		emit pixArray(tempImagePixels);
		imageCount++;
	}
}

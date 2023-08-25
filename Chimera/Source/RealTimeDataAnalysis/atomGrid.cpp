#include <stdafx.h>
#include "atomGrid.h"
#include <ConfigurationSystems/ConfigSystem.h>

const std::string atomGrid::GRID_FILE_EXTENSION = ".grid";

void atomGrid::loadGridFile(atomGrid& grid)
{
	if (!grid.useFile) {
		return;
	}
	ConfigStream gridFile(PLOT_FILES_SAVE_LOCATION + "\\" + grid.fileName + GRID_FILE_EXTENSION, true);
	std::vector<std::vector<coordinate>> atomsLocations;
	unsigned long x, y;
	while (gridFile.peek() != EOF) {
		ConfigStream locs(gridFile.getline());
		atomsLocations.emplace_back();
		while (locs.peek() != EOF) {
			try {
				locs >> x >> y;
				atomsLocations.back().push_back(coordinate(y, x));
			}
			catch (const std::ios_base::failure& fail) {
				throwNested("ERROR: failed to read string stream in while reading data from " + grid.fileName + ". Probably the data size is not of even number." 
					+ "\r\n" + fail.what());
			}
			catch (ChimeraError &e) {
				throwNested("ERROR: failed to convert grid parameters to longs while reading data from " + grid.fileName + 
					". Probably the data size is not of even number if the text is empty.\r\n" + e.what());
			}
		}
	}
	grid.atomLocs = atomsLocations;
}

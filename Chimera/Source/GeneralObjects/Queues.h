// created by Mark O. Brown
#pragma once

#include <vector>
#include <GeneralObjects/Matrix.h>
#include <qobject.h>

// info for the picture in the experiment run in which the pic is taken
struct picureStatus {
	unsigned long long picNum;
	unsigned repNum;
	unsigned varNum;
};

// basically just a little nicer than a std::pair
struct AtomImage{
	picureStatus picStat;
	std::vector<bool> image;
};

struct NormalImage{
	picureStatus picStat;
	Matrix<long> image;
};

struct PixList{
	picureStatus picStat;
	std::vector<double> image;
};

// imQueue[gridNum][queuePositions][pixelNum(flattened)]
typedef std::vector<AtomImage> atomQueue;
typedef std::vector<NormalImage> imageQueue;
typedef std::vector<PixList> PixListQueue;
//typedef std::vector<imageQueue> multiGridImageQueue;
typedef std::vector<PixListQueue> multiGridImageQueue;
typedef std::vector<atomQueue> multiGridAtomQueue;

Q_DECLARE_METATYPE (NormalImage)
Q_DECLARE_METATYPE (atomQueue)
Q_DECLARE_METATYPE (PixListQueue)
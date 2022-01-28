#include "stdafx.h"
#include "MOTAnalysisControl.h"
#include <CustomQtControls/AutoNotifyCtrls.h>
#include <CMOSCamera/MakoCamera.h>
#include <PrimaryWindows/QtMakoWindow.h>
#include <PrimaryWindows/QtAuxiliaryWindow.h>
#include <algorithm>
#include <utility>
#include <qthread.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>



MOTAnalysisControl::MOTAnalysisControl(IChimeraQtWindow* parent) 
	: IChimeraSystem(parent)
{
	for (auto tp : MOTAnalysisType::allTypes) {
		if (tp != MOTAnalysisType::type::density2d) {
			plotData1D.insert({ tp,std::vector<double>() });
			plotStdev1D.insert({ tp,std::vector<double>() });
		}
		//calcViewer.emplace( tp, QCustomPlotCtrl() ); emplace does not work here since QCustomPlotCtrl does not have a move contructor, 
		//see https://stackoverflow.com/questions/41317880/unordered-map-emplace-giving-compiler-time-errors
		//calcViewer[tp]; // this will work since [] operator will auto create val if key is not existed before
	}
	//calcViewer[MOTAnalysisType::type::density2d].setStyle(plotStyle::PicturePlot);

}

void MOTAnalysisControl::initialize(IChimeraQtWindow* parent)
{
	//IChimeraQtWindow* parent = this->parentWin;

	calcActive = new QCheckBox("Active?", this);
	QLabel* tmp = new QLabel("Calc.Type:", this);
	MOTCalcCombo = new CQComboBox(parent);
	for (auto& t : MOTAnalysisType::allTypes) {
		MOTCalcCombo->addItem(qstr(MOTAnalysisType::toStr(t)));
	}
	QLabel* tmp2 = new QLabel("CMOS.Src:", this);
	makoSrcCombo = new CQComboBox(parent);
	for (size_t idx = 0; idx < MAKO_NUMBER; idx++) {
		makoSrcCombo->addItem("MAKO" + qstr(idx + 1));
		if (MAKO_SAFEMODE[idx]) {
			QStandardItemModel* model = qobject_cast<QStandardItemModel*>(makoSrcCombo->model());
			QStandardItem* item = model->item(idx);
			item->setEnabled(false);
		}
	}
	twoDScanActive = new CQCheckBox("2D Scan?", parent);
	QLabel* tmp3 = new QLabel("X Key:");
	xKeyCombo = new CQComboBox(parent);
	QLabel* tmp4 = new QLabel("Y Key:");
	yKeyCombo = new CQComboBox(parent);
	yKeyCombo->setEnabled(false);
	QLabel* xkeyvalL = new QLabel("X Vals:");
	QLabel* ykeyvalL = new QLabel("Y Vals:");
	xKeyValCombo = new CQComboBox(parent);
	yKeyValCombo = new CQComboBox(parent);
	for (auto tp : MOTAnalysisType::allTypes) {
		calcViewer[tp];
		if (tp == MOTAnalysisType::type::density2d) {
			calcViewer[MOTAnalysisType::type::density2d].setStyle(plotStyle::DensityPlot);
		}
		calcViewer[tp].init(parent, qstr(MOTAnalysisType::toStr(tp)));
		calcViewer[tp].plot->setMinimumSize(600, 400);
		calcViewer[tp].plot->replot();
	}

	connect(MOTCalcCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, xkeyvalL, ykeyvalL](int idx) {
		auto currentType = MOTAnalysisType::allTypes[MOTCalcCombo->currentIndex()];
		if (currentType == MOTAnalysisType::type::density2d) {
			xkeyvalL->show(); xKeyValCombo->show(); 
			if (twoDScanActive->isEnabled()) {
				ykeyvalL->show(); yKeyValCombo->show();
			}
			else {
				ykeyvalL->hide(); yKeyValCombo->hide();
			}
		}
		else {
			xkeyvalL->hide(); ykeyvalL->hide();
			xKeyValCombo->hide(); yKeyValCombo->hide();
		}
		visiblePlot->hide();
		visiblePlot = calcViewer[currentType].plot;
		calcViewer[currentType].resetChart();
		visiblePlot->show();
		visiblePlot->replot(); });

	connect(makoSrcCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int idx) {
		if (calcActive->isChecked()) {
			makoCam = this->parentWin->makoWin->getMakoCam(idx);} });
	connect(calcActive, &QCheckBox::stateChanged, this, [this](int checked) {
		int makoidx = makoSrcCombo->currentIndex();
		makoCam = this->parentWin->makoWin->getMakoCam(makoidx); });
	connect(twoDScanActive, &QCheckBox::stateChanged, this, [this,xkeyvalL,ykeyvalL](int checked) {
		yKeyCombo->setEnabled(checked);
		if (MOTAnalysisType::allTypes[MOTCalcCombo->currentIndex()] == MOTAnalysisType::type::density2d) {
			if (checked) {
				ykeyvalL->show(); yKeyValCombo->show();
			}
			else {
				ykeyvalL->hide(); yKeyValCombo->hide();
			}
		} });
	connect(&this->parentWin->auxWin->getConfigs(), &ParameterSystem::paramsChanged, this, [this]() {updateXYKeys(); });

	connect(xKeyCombo, qOverload<int>(&QComboBox::activated), this, [this](int idx) {
		if ((!xKeysList.empty()) && xKeysList.size() > idx) {
			xKeyValCombo->clear();
			for (auto val : xKeysList[idx].keyValues) {
				xKeyValCombo->addItem(qstr(val,3,true));
			}
			currentXKeys = xKeysList[idx].keyValues;
			updatePlotData1D();
			for (auto& [key, val] : calcViewer) {
				val.plot->xAxis->setLabel(qstr(xKeysList[idx].name));
				val.plot->replot();
			}
		} });
	connect(yKeyCombo, qOverload<int>(&QComboBox::activated), this, [this](int idx) {
		if ((!yKeysList.empty()) && yKeysList.size() > idx  && twoDScanActive->isChecked()) {
			yKeyValCombo->clear();
			for (auto val : yKeysList[idx].keyValues) {
				yKeyValCombo->addItem(qstr(val, 3, true));
			}
			currentYKeys = yKeysList[idx].keyValues;
			for (auto& [key, val] : calcViewer) {
				val.plot->yAxis->setLabel(qstr(yKeysList[idx].name));
				val.plot->replot();
			}
		} });

	connect(xKeyValCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int idx) {
		if (plotData2D.empty()) {
			return;
		}
		std::vector<double>& val = plotData2D[idx];
		if (val.size() != (height2D * width2D) || val.empty() || width2D == 0 || height2D == 0) {
			return;
		}
		std::vector<plotDataVec> ddvec(height2D);
		for (size_t idx = 0; idx < height2D; idx++)
		{
			ddvec[idx].reserve(width2D);
			for (size_t idd = 0; idd < width2D; idd++)
			{
				ddvec[idx].push_back(dataPoint{ 0,val[idx * width2D + idd],0 }); // x,y,err
			}
		}
		calcViewer[MOTAnalysisType::type::density2d].setData(ddvec);
		});


	QVBoxLayout* layout = new QVBoxLayout(this);
	this->setMaximumWidth(650);
	this->setMaximumHeight(550);
	// title
	QLabel* analyTitle = new QLabel("MOT ANALYZER", parent);
	layout->addWidget(analyTitle, 0);
	// control gadget
	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);
	layout1->addWidget(calcActive, 0);
	layout1->addWidget(tmp2, 0);
	layout1->addWidget(makoSrcCombo, 0);
	layout1->addWidget(tmp, 0);
	layout1->addWidget(MOTCalcCombo, 0);
	layout1->addStretch(1);
	QHBoxLayout* layout2 = new QHBoxLayout();
	layout2->addWidget(twoDScanActive, 0);
	layout2->addWidget(tmp3, 0);
	layout2->addWidget(xKeyCombo, 0);
	layout2->addWidget(tmp4, 0);
	layout2->addWidget(yKeyCombo, 0);
	layout2->addWidget(xkeyvalL, 0);
	layout2->addWidget(xKeyValCombo, 0);
	layout2->addWidget(ykeyvalL, 0);
	layout2->addWidget(yKeyValCombo, 0);
	xkeyvalL->hide(); ykeyvalL->hide(); xKeyValCombo->hide(); yKeyValCombo->hide();
	layout2->addStretch(1);


	layout->addLayout(layout1, 0);
	layout->addLayout(layout2, 0);
	for (auto tp : MOTAnalysisType::allTypes) {
		calcViewer[tp].plot->hide();
		layout->addWidget(calcViewer[tp].plot);
	}
	visiblePlot = calcViewer[MOTAnalysisType::type::min].plot;
	visiblePlot->show();
	
}

void MOTAnalysisControl::updateXYKeys()
{
	std::vector<parameterType> params = this->parentWin->auxWin->getConfigs().getAllVariables();
	if (params.empty()) {
		return;
	}
	ScanRangeInfo rginfo = this->parentWin->auxWin->getConfigs().getRangeInfo();
	ParameterSystem::generateKey(params, false, rginfo);
	xKeyCombo->clear();
	yKeyCombo->clear();
	xKeysList.clear();
	yKeysList.clear();
	for (auto& para : params) {
		std::sort(para.keyValues.begin(), para.keyValues.end());
		auto last = std::unique(para.keyValues.begin(), para.keyValues.end());
		para.keyValues.erase(last, para.keyValues.end());
		switch (para.scanDimension) {
		case 0: 
			xKeyCombo->addItem(qstr(para.name));
			xKeysList.push_back(para);
			break;
		case 1:
			yKeyCombo->addItem(qstr(para.name));
			yKeysList.push_back(para);
			break;
		default:
			emit warning("Scan dimension is greater than 1, MOT analysis will only perform on scan dimension 0 and 1. ");
		}
	}
}

void MOTAnalysisControl::prepareMOTAnalysis(MakoCamera*& cam)
{
	if (makoCam == nullptr) {
		//emit notification("MOT analysis sees a null camera ptr, please check if you selected the working camera. Possibly"
		//	" a low level bug");
		cam = nullptr;
		return;
	}
	emit notification("MOT analysis is turned on for " + qstr(CameraInfo::toStr(cam->getCameraInfo().camName)) + "\r\n", 1);
	if (calcActive->isChecked() && makoCam != nullptr) {
		cam = makoCam;
	}
	else {
		cam = nullptr;
	}
	if (xKeysList.empty()) {
		thrower("Error in MOTAnalysis: the keylist is empty, make sure to change parameter from const to var and then enable the analysis");
	}
	for (auto typ : MOTAnalysisType::allTypes) {
		if (typ != MOTAnalysisType::type::density2d) {
			plotData1D[typ].resize(xKeysList.back().keyValues.size(), 0.0);
			plotStdev1D[typ].resize(xKeysList.back().keyValues.size(), 0.0);
		}
	}
	width2D = makoCam->getMakoCore().getRunningSettings().dims.width();
	height2D = makoCam->getMakoCore().getRunningSettings().dims.height();
	plotData2D.clear();
	plotData2D.resize(xKeysList.back().keyValues.size());
	incomeOrder.clear();
	incomeOrder.reserve(makoCam->getMakoCore().getRunningSettings().totalPictures());
	updateXYKeys();
	currentXKeys = xKeysList[xKeyCombo->currentIndex()].keyValues;
	if (twoDScanActive->isChecked()) {
		currentYKeys = yKeysList[yKeyCombo->currentIndex()].keyValues;
	}
}

void MOTAnalysisControl::handleNewPlotData1D(std::vector<double> val, std::vector<double> stdev, int currentNum) 
{
	incomeOrder.push_back(currentNum);
	size_t dim1 = xKeysList.back().keyValues.size();
	for (size_t idx = 0; idx < val.size(); idx++) {
		auto typ = MOTAnalysisType::allTypes[idx];
		plotData1D[typ][currentNum % dim1] = val[idx];
		plotStdev1D[typ][currentNum % dim1] = stdev[idx];
	}
	updatePlotData1D();

}

void MOTAnalysisControl::handleNewPlotData2D(std::vector<double> val, int width, int height, int currentNum)
{
	if (width2D != width || height2D != height) {
		errBox("MOTAnalysis see a discrepency in the image width and size. The size expected from"
			" MAKO setting is (" + str(width2D) + "," + str(height2D) + "). But get(" + str(width) + ", " + str(height) + ") instead");
		return;
	}
	//right now 2D data does not support 2D scan
	std::vector<plotDataVec> ddvec(height);
	for (size_t idx = 0; idx < height; idx++)
	{
		ddvec[idx].reserve(width);
		for (size_t idd = 0; idd < width; idd++)
		{
			ddvec[idx].push_back(dataPoint{ 0,val[idx * width + idd],0 }); // x,y,err
		}
	}
	int var = currentNum % xKeysList.back().keyValues.size();
	if (xKeyValCombo->currentIndex() == var) {
		calcViewer[MOTAnalysisType::type::density2d].setData(ddvec);
	}
	plotData2D[currentNum % xKeysList.back().keyValues.size()] = val;
}

void MOTAnalysisControl::updatePlotData1D()
{
	for (auto& [typ, view] : calcViewer) {
		if (plotData1D[typ].empty()) {
			break;
		}
		std::vector<plotDataVec> ddvec(1);
		for (auto idx : range(plotData1D[typ].size())) {
			ddvec[0].push_back(dataPoint{ currentXKeys[idx],plotData1D[typ][idx],plotStdev1D[typ][idx] });
		}
		view.setData(ddvec);
	}

}

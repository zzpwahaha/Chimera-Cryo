#pragma once
#include <atomic>

class IChimeraQtWindow;
class QTimer;
class QCheckBox;
class QLineEdit;
class QHBoxLayout;

class InExpCalControl
{
public:
	InExpCalControl& operator=(const InExpCalControl&) = delete;
	InExpCalControl(const InExpCalControl&) = delete;
	InExpCalControl(IChimeraQtWindow* parent);
	void initialize(IChimeraQtWindow* parent);
private:
	std::atomic<bool> interrupt = false;
	double interval; // in the unit of minutes
	QTimer* timer = nullptr;


	QCheckBox* inExpCalibrationButton = nullptr;
	QLineEdit* inExpCalibrationDurationEdit = nullptr;
	QHBoxLayout* layout = nullptr;


	friend class MainOptionsControl;
};
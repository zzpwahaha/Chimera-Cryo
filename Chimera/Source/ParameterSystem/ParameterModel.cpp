#include "stdafx.h"
#include "ParameterModel.h"
#include <boost/lexical_cast.hpp>
#include <qmimedata.h>
#include <qdebug.h>

ParameterModel::ParameterModel (bool isGlobal_, QObject* parent)
    : QAbstractTableModel (parent), isGlobal(isGlobal_){
    rangeInfo.defaultInit ();
    rangeInfo.setNumScanDimensions (1);
    rangeInfo.setNumRanges (0, 1);
}

int ParameterModel::rowCount (const QModelIndex& /*parent*/) const{
    return parameters.size();
}

int ParameterModel::columnCount (const QModelIndex& /*parent*/) const{
    int maxRanges = 0;
    for (auto dimRange : rangeInfo.data){
        if (dimRange.size () > maxRanges){
            maxRanges = dimRange.size ();
        }
    }
    int baseCols = (isGlobal ? 2 : 5 + maxRanges*3);
    return baseCols;
}

QVariant ParameterModel::data (const QModelIndex& index, int role) const{
    int row = index.row ();
    int col = index.column ();
    auto param = parameters[row];
    try {
        switch (role) {
		case Qt::FontRole:{
			if (param.overwritten) {
				QFont font;
				font.setBold (true);
				return QVariant(font);
			}
			return QVariant (); // default
		}
		case Qt::ForegroundRole: { // text color
			if (param.overwritten) {
				return QVariant (QBrush (QColor (255, 0, 0)));
			}
			return QVariant (); // default
		}
		case Qt::BackgroundRole: { // background color
            if (!param.constant && !param.active) {
                return QVariant(QBrush(QColor(250, 250, 210)));
            }
			if (param.constant && param.active) {
                return QVariant(QBrush(QColor(31, 148, 7)));
			}
            if (!param.constant && param.active) {
                return QVariant(QBrush(QColor(255, 255, 0)));
            }
			return QVariant (); // default
		}
        case Qt::EditRole: 
            if (!isGlobal && col >= 5) {
                auto rangeNum = int (col - 5) / 3;
                switch ((col - 5) % 3) {
					case 0:
						return qstr (param.ranges[rangeNum].initialValue, 9, true);
					case 1:
						return qstr (param.ranges[rangeNum].finalValue, 9, true);
                }
            } // purposely don't break as this is only different for these two. 
        case Qt::DisplayRole:
            if (isGlobal) {
                switch (col) {
                case 0:
                    return qstr(param.name);
                case 1:
                    return qstr (param.constantValue, 9, true);
                default:
                    return QVariant ();
                }
            }
            else {
                switch (col) {
                case 0:
                    return qstr (param.name);
                case 1:
                    return qstr (param.constant ? "Const." : "Var.");
                case 2:
                    return qstr (param.scanDimension);
                case 3:
                    return param.constant ? qstr (param.constantValue, 9, true) : "---";
                case 4:
                    return qstr (param.parameterScope);
                default:
                    if (param.constant) {
                        return QString ("---");
                    }
                    auto rangeNum = int (col - 5) / 3; // make sure rangeNum is >0, since numRanges return size_t, therefore  int(-1)>size_t(0)
                    if (rangeNum > rangeInfo.numRanges(param.scanDimension) - 1) {
                        return QString("---");
                    }
                    std::string lEnd = rangeInfo (param.scanDimension, rangeNum).leftInclusive ? "[" : "(";
                    std::string rEnd = rangeInfo (param.scanDimension, rangeNum).rightInclusive ? "]" : ")";
                    switch ((col - 5) % 3) {
						case 0:
							return qstr(lEnd + str (param.ranges[rangeNum].initialValue,9,true));
						case 1:
							return qstr (str(param.ranges[rangeNum].finalValue, 9, true) + rEnd);
						case 2:
							return qstr (rangeInfo (param.scanDimension, rangeNum).variations);
                    }
                    return QVariant ();
                }
            }
        }
    }
    catch (ChimeraError& err){
        errBox (err.trace ());
    }
    return QVariant ();
}

QVariant ParameterModel::headerData (int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if (isGlobal) {
            if (section > 1) return QVariant ();
            auto headers = std::vector<std::string> ({ "Symbol", "Value" });
            return QString (headers[section].c_str());
        } else {
            auto headers = std::vector<std::string> ({ " Sym. ", " Type ", " Dim ", " Val. ", " Scope " });
            for (auto rangeInc : range (5)){
                headers.push_back (str (rangeInc + 1) + ". {");
                headers.push_back ("}");
                headers.push_back ("#");
            }
            if (section >= headers.size()) return QVariant ();
            return QString (headers[section].c_str ());
        }
    }
    else if (role == Qt::DisplayRole && orientation == Qt::Vertical) {
        return QString (cstr (section));
    }
    return QVariant ();
}

Qt::ItemFlags ParameterModel::flags (const QModelIndex& index) const {
    if (index.column() == 1 && !isGlobal || // the "const. / var. option is togglable, not editable.
        (index.column() >= preRangeColumns && parameters[index.row()].constant) || // variation values for constants
        index.column() == 3 && !isGlobal && !parameters[index.row()].constant // const value for variables
        //(index.column() - preRangeColumns) / 3 > rangeInfo.numRanges(parameters[index.row()].scanDimension) - 1 // parameter with its dimension does not have this range
        ) {
        return QAbstractTableModel::flags(index) | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }
    if (index.column() >= preRangeColumns) {
        int rangeNum = (index.column() - preRangeColumns) / 3;
        int numRange = rangeInfo.numRanges(parameters[index.row()].scanDimension);
        if (rangeNum > numRange - 1) {
            return QAbstractTableModel::flags(index) | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        }
    }
    return Qt::ItemIsEditable | QAbstractTableModel::flags(index) | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
};

bool ParameterModel::setData (const QModelIndex& index, const QVariant& value, int role){
    if (role == Qt::EditRole) {
        try {
            auto& param = parameters[index.row ()];
            if (isGlobal) {
                switch (index.column ()) {
                    case 0: // always convert name and scope to lowercase.
                        param.name = str (value.toString (), 13, false, true);
                        break;
                    case 1:
                        param.constantValue = boost::lexical_cast<double>(cstr (value.toString ()));
                        break;
                }
            }
            else {
                switch (index.column ()) {
                    case 0: // always convert name and scope to lowercase.
                        param.name = str (value.toString (), 13, false, true);
                        break;
                    case 1: // constant vs. variable toggle
                        break;
                    case 2:
                        param.scanDimension = boost::lexical_cast<int> (cstr(value.toString ())); 
                        if (param.scanDimension+1 > rangeInfo.numScanDimensions ()){
                            rangeInfo.setNumScanDimensions (param.scanDimension+1);
                        }
                        while (param.ranges.size() < rangeInfo.numRanges(param.scanDimension)) {
                            param.ranges.push_back({ 0.0, 0.0 });
                        }
                        break;
                    case 3: 
                        param.constantValue = boost::lexical_cast<double> (cstr(value.toString ()));
                        break;
                    case 4:// always convert name and scope to lowercase.
                        param.parameterScope = str (value.toString (), 13, false, true);
                        break;
                    default:
                        auto rangeNum = int (index.column() - 5) / 3;
                        std::string lEnd = rangeInfo (param.scanDimension, rangeNum).leftInclusive ? "[" : "(";
                        std::string rEnd = rangeInfo (param.scanDimension, rangeNum).rightInclusive ? "]" : ")";

                        switch ((index.column () - 5) % 3) {
							case 0:
								param.ranges[rangeNum].initialValue = boost::lexical_cast<double>(cstr(value.toString ()));
								break;
							case 1:
								param.ranges[rangeNum].finalValue = boost::lexical_cast<double>(cstr (value.toString ()));
								break;
							case 2:
								rangeInfo (param.scanDimension, rangeNum).variations 
									= boost::lexical_cast<unsigned int>(cstr (value.toString ()));

                        }
                }
            }
        }
        catch (boost::bad_lexical_cast&) {
            // hmm
        }
        emit paramsChanged ();
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

void ParameterModel::setParams (std::vector<parameterType> newParams) {
    beginResetModel ();
    parameters = newParams;
    endResetModel();
    emit paramsChanged ();
}

std::vector<parameterType>& ParameterModel::getParams () {
    return parameters;
}

ScanRangeInfo ParameterModel::getRangeInfo (){
    return rangeInfo;
}

void ParameterModel::setRangeInfo (ScanRangeInfo info){
    beginResetModel ();
    rangeInfo = info;
    endResetModel ();
}

void ParameterModel::checkScanDimensionConsistency () {
    for (auto& param : parameters) {
        if (param.scanDimension >= rangeInfo.numScanDimensions ()) {
            param.scanDimension = 0;
        }
    }
}

void ParameterModel::checkVariationRangeConsistency () {
	bool flag = true;
    for (auto& var : parameters) {
        if (var.ranges.size () != rangeInfo.numRanges (var.scanDimension)) {
            if (flag && (!var.constant)) {
                // only alert user for variable parameter, do constant under the hood
                errBox ("The number of variation ranges of a parameter, " + var.name + ", (and perhaps others) did "
                    "not match the official number. The code will force the parameter to match the official number.");
				//flag = false; // only dislpay the error message once.
            }
            var.ranges.resize (rangeInfo.numRanges (var.scanDimension));
        }
    }
}


void ParameterModel::setVariationRangeNumber (int newRangeNum, int currVarRangeNum, unsigned short dimNumber){
    // -2 for the two +- columns
    int currentVariableRangeNumber = currVarRangeNum;
    checkScanDimensionConsistency ();
    checkVariationRangeConsistency ();
    if (rangeInfo.numRanges (dimNumber) != currentVariableRangeNumber) {
        errBox ("somehow, the number of ranges the ParameterSystem object thinks there are and the actual number "
            "displayed are off! The numbers are " + str (rangeInfo.numRanges (dimNumber)) + " and "
            + str (currentVariableRangeNumber) + " respectively. The program will attempt to fix this, but "
            "data may be lost.");
        while (rangeInfo.numRanges (dimNumber) != currentVariableRangeNumber) {
            if (rangeInfo.numRanges (dimNumber) > currentVariableRangeNumber) {
                rangeInfo.dimensionInfo (dimNumber).pop_back ();
                for (auto& param : parameters) {
                    param.ranges.pop_back ();
                }
            }
            else {
                rangeInfo.dimensionInfo (dimNumber).push_back (defaultRangeInfo);
                for (auto& param : parameters) {
                    param.ranges.pop_back ();
                }
            }
        }
    }
    if (currentVariableRangeNumber < newRangeNum) {
        while (currentVariableRangeNumber < newRangeNum) {
            /// add a range.
            rangeInfo.dimensionInfo (dimNumber).push_back (defaultRangeInfo);
            for (unsigned varInc = 0; varInc < parameters.size (); varInc++) {
                if (parameters[varInc].scanDimension == dimNumber) {
                    // only add range for the same dimension parameter, no matter whether it is const or var
                    indvParamRangeInfo tempInfo{ 0,0 };
                    parameters[varInc].ranges.push_back(tempInfo);
                }
            }
            currentVariableRangeNumber++;
        }
    }
    else if (currentVariableRangeNumber > newRangeNum) {
        while (currentVariableRangeNumber > newRangeNum) {
            // delete a range.
            if (rangeInfo.dimensionInfo (dimNumber).size () == 1) {
                return;	// can't delete last range
            }
            for (auto& param : parameters) {
                if (param.scanDimension == dimNumber) {
                    // only remove range for the same dimension parameter, no matter whether it is const or var
                    param.ranges.pop_back();
                }
            }
            rangeInfo.dimensionInfo (dimNumber).pop_back ();
            currentVariableRangeNumber--;
        }
    }
    setParams (parameters);
    setRangeInfo (rangeInfo);
}

Qt::DropActions ParameterModel::supportedDropActions() const
{
    return Qt::DropActions() | Qt::MoveAction;
}

QMimeData* ParameterModel::mimeData(const QModelIndexList& indexes) const
{
    QMimeData* mimeData = new QMimeData();
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    std::vector<int> idxList;
    int lastRow = -1;
    for (const QModelIndex& index : indexes) {
        if (index.isValid()) {
            int row = index.row();
            if (lastRow != row) {
                // only store the data if the row is different
                lastRow = row;
                idxList.push_back(row);
            }
        }
    }
    std::sort(idxList.begin(), idxList.end()); // sort the index in ascending order so that it does not dependen on the order of click
    for (int id : idxList) {
        stream << id;
    }


    mimeData->setData("parameters/parameterIdx", encodedData);
    return mimeData;
}

QStringList ParameterModel::mimeTypes() const
{
    QStringList types;
    types << "parameters/parameterIdx";
    return types;
}

bool ParameterModel::insertRows(int row, int count, const QModelIndex& parent)
{
    qDebug() << "bool ParameterModel::insertRows(int row, int count, const QModelIndex& parent) get called" <<
        "This should never happen";
    /// should never need to invole this

    //if ((row != -1) && (count > 0)) {
    //    beginInsertRows(parent, row, row + count - 1);
    //    for (int i = 0; i != count; i++)
    //        dataList.emplace(dataList.begin() + row, columnCount());
    //    endInsertRows();
    //    emit dataChanged(index(row, 0), index(row + count - 1, columnCount()));
    //    emit layoutChanged();
    //    return true;
    //}
    //else {
    //    return false;
    //}
    //return true;
    return false;
}

bool ParameterModel::removeRows(int row, int count, const QModelIndex& parent)
{
    if ((row != -1) && (count > 0)) {
        beginRemoveRows(parent, row, row + count - 1);
        parameters.erase(parameters.begin() + row, parameters.begin() + row + count);
        endRemoveRows();
        return true;
    }
    else {
        return false;
    }
}

bool ParameterModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild)
{
    qDebug() << "bool ParameterModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild) get called" <<
        "This should never happen";
    /// should never need to invole this
    //std::exit(-1);
    //if ((sourceRow != -1) && (destinationChild != -1) && (count > 0)) {
    //    if ((destinationChild >= sourceRow) && (destinationChild < sourceRow + count)) // destination is in the middle of origin range, so reject
    //        return false;
    //    beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1, destinationParent, destinationChild);
    //    dataList.insert(dataList.begin() + destinationChild, dataList.begin() + sourceRow, dataList.begin() + sourceRow + count);
    //    dataList.erase(dataList.begin() + sourceRow, dataList.begin() + sourceRow + count);
    //    endMoveRows();
    //    emit dataChanged(index(sourceRow, 0), index(sourceRow + count - 1, columnCount()));
    //    emit dataChanged(index(destinationChild, 0), index(destinationChild + count - 1, columnCount()));
    //    emit layoutChanged();
    //    return true;
    //}
    //else {
    //    return false;
    //}
    return false;
}

bool ParameterModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (!data->hasFormat("parameters/parameterIdx"))
        return false;
    if (action == Qt::IgnoreAction)
        return true;
    if (column > 0) // column will be -1 for row drag and move
        return false;
    int endRow;
    if (parent.isValid()) // row was dropped directly on an item (parent)
    {
        // If we apply dropMimeData without any modifications,
        // the data overwrites the given row.
        // However, we would like to insert the data *after* the given row.
        endRow = parent.row() + 1;
    }
    else {
        if (row < 0)
            endRow = parameters.size();
        else
            endRow = qMin(row, int(parameters.size()));
    }


    QByteArray encodedData = data->data("parameters/parameterIdx");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);

    std::vector<std::pair<size_t, parameterType>> dataListInsert;
    // the index in stream should already be sorted in mimeData() function
    // need to store the moved params into dataListInsert first and then insert it into the parameters 
    // so that the insertion would not cause the index being wrong for the later insertion
    while (!stream.atEnd()) {
        int idx;
        stream >> idx;
        dataListInsert.push_back(std::make_pair(idx, parameters[idx]));
    }

    for (auto& d : dataListInsert) {
        beginInsertRows(QModelIndex(), endRow, endRow);
        parameters.insert(parameters.begin() + endRow, d.second);
        endInsertRows();
        endRow++;
    }



    return true;
}

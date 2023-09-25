#pragma once

#include <QAbstractTableModel>
#include "ParameterSystemStructures.h"

class QMimeData;

class ParameterModel : public QAbstractTableModel
{
    Q_OBJECT
    public:
        ParameterModel (bool isGlobal, QObject* parent = nullptr);
        int rowCount (const QModelIndex& parent = QModelIndex ()) const override;
        int columnCount (const QModelIndex& parent = QModelIndex ()) const override;
        QVariant data (const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData (int section, Qt::Orientation orientation, int role) const override;
        Qt::ItemFlags flags (const QModelIndex& index) const override;
        bool setData (const QModelIndex& index, const QVariant& value, int role) override;
        void setParams (std::vector<parameterType> newParams); 
        std::vector<parameterType>& getParams ();
        ScanRangeInfo getRangeInfo ();
        void setRangeInfo (ScanRangeInfo info);
        const unsigned short preRangeColumns = 5;
        void checkScanDimensionConsistency ();
        void checkVariationRangeConsistency ();
        void setVariationRangeNumber (int newRangeNum, int currVarRangeNum, unsigned short dimNumber );
        const bool isGlobal;


        // for drag and drop feature in the view, will change the order the parameters after this
        Qt::DropActions supportedDropActions() const override;
        QMimeData* mimeData(const QModelIndexList& indexes) const override;
        QStringList mimeTypes() const override;
        bool insertRows(int row, int count, const QModelIndex& parent) override; // not need this if not using default dropMimeData function
        bool removeRows(int row, int count, const QModelIndex& parent) override; // called by qt after the user-implemented dropMimaData, if use default dropMimeData, will call insertRows and then removeRows
        bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild) override; // never called by qt during testing
        bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;


    private:
        const IndvRangeInfo defaultRangeInfo = { 2,false,true };
        std::vector<parameterType> parameters;
        ScanRangeInfo rangeInfo;
    signals:
		void paramsChanged ();
        void editCompleted (const QString&);
};
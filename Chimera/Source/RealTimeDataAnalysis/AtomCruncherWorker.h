#pragma once

#include <qobject.h>
#include <string>
#include <GeneralObjects/Queues.h>
#include <RealTimeDataAnalysis/atomCruncherInput.h>

class CruncherThreadWorker : public QObject {
    Q_OBJECT

    public:
        // meant to claim ownership of this unique_ptr, take it by value instead of r-value reference because the latter may or may not claim ownership (depending on internal code paths)
        // see https://stackoverflow.com/questions/8114276/how-do-i-pass-a-unique-ptr-argument-to-a-constructor-or-a-function
        CruncherThreadWorker (std::unique_ptr<atomCruncherInput> input_);
        ~CruncherThreadWorker ();

    public Q_SLOTS:
        void init ();
        void handleImage (NormalImage);
		 
    Q_SIGNALS:
        void error (QString errstr);
        void atomArray (atomQueue aqueue);
        void pixArray (PixListQueue pixlist);
    private:
        std::unique_ptr<atomCruncherInput> input;
        unsigned imageCount;
        std::vector<std::vector<std::vector<long>>> monitoredPixelIndecies; // grid -> atom -> monitored pixels
};


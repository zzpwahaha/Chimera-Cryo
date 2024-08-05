#pragma once
#include <qobject.h>
#include <thread>
#include <GeneralFlumes/BoostAsyncTCPServer.h>

class MessageConsumer : public QObject
{
    Q_OBJECT
public:
    // THIS CLASS IS NOT COPYABLE.
    MessageConsumer& operator=(const MessageConsumer&) = delete;
    MessageConsumer(const MessageConsumer&) = delete;
    MessageConsumer(ThreadsafeQueue<TCPMessageTyep>& queue);
    ~MessageConsumer();
    void start();
private:
    void consume();
    std::string getCurrentDateTime();
    std::vector<std::string> getArguments(const std::string& command);
    std::vector<std::string> splitString(const std::string& input, char delimiter);
    std::vector<std::string> splitString(const std::string& input, const std::string& delimiter);

signals:
    void logMessage(QString msg);
    //void error(QString msg, unsigned errorLevel = 0);
    void openConfiguration(QString addressName);


private:
    const char delimiter_ = '$';
    ThreadsafeQueue<TCPMessageTyep>& queue_;
    std::thread consumer_thread_;
};


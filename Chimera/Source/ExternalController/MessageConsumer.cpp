#include "stdafx.h"
#include "MessageConsumer.h"
#include <chrono>
#include <ctime>
#include <sstream>

MessageConsumer::MessageConsumer(ThreadsafeQueue<TCPMessageTyep>& queue, CommandModulator& modulator) : 
    queue_(queue),
    modulator_(modulator)
{}

MessageConsumer::~MessageConsumer()
{
    if (consumer_thread_.joinable()) {
        consumer_thread_.join();
    }
}

void MessageConsumer::start()
{
    consumer_thread_ = std::thread(&MessageConsumer::consume, this);
}

void MessageConsumer::consume()
{
    auto stratWith = [](std::string msg, std::string trait) {
        return msg.rfind(trait, 0) == 0; }; // pos=0 limits the search to the prefix
    while (true) {
        auto data = queue_.pop();
        auto& message = data.msg;
        auto& connection = data.connection;
        CommandModulator::ErrorStatus status;
        
        // Process the message
        std::string timeStamp = getCurrentDateTime();
        emit logMessage(qstr(timeStamp + ": " + data.msg));

        if (stratWith(message, "Error")) { 
            //emit error(qstr(message), 0);
            continue;
        }
        else if (stratWith(message, "Open-Configuration")) {
            auto args = getArguments(message);
            if (args.size() == 0) {
                emit logMessage(qstr(timeStamp + ": \t" + "No arguemnt found in command: " + message));
                connection->do_write("Error\nNo arguemnt found in command: " + message);
                continue;
            }
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.openConfiguration(qstr(args[0]), status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished opening configuration: " + args[0], status));
        }
        else if (stratWith(message, "Open-Master-Script")) {
            auto args = getArguments(message);
            if (args.size() == 0) {
                emit logMessage(qstr(timeStamp + ": \t" + "No arguemnt found in command: " + message));
                connection->do_write("Error\nNo arguemnt found in command: " + message);
                continue;
            }
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.openMasterScript(qstr(args[0]), status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished opening master script: " + args[0], status));

        }
        else if (stratWith(message, "Save-All")) {
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.saveAll(status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished saving all", status));
        }
        else if (stratWith(message, "Start-Experiment")) {

            connection->do_write(compileReply("Finished starting experiment", status));
        }
        else {
            emit logMessage(qstr(timeStamp + ": \t" + "Unrecongnized command: " + message));
            connection->do_write("Error\nUnrecongnized command: " + message);
        }
    }
}

std::string MessageConsumer::compileReply(std::string normalMsg, CommandModulator::ErrorStatus status)
{
    if (status.error) {
        return str("Error\n") + status.errorMsg;
    }
    else {
        return normalMsg;
    }
}

std::string MessageConsumer::getCurrentDateTime()
{
    // Get current time as a time_point
    auto now = std::chrono::system_clock::now();
    // Convert to time_t, which represents time in seconds since epoch
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    // Convert to tm structure for local time
    std::tm now_tm = *std::localtime(&now_time);
    // Create a stringstream to format the date and time
    std::ostringstream oss;
    oss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::vector<std::string> MessageConsumer::getArguments(const std::string& command)
{
    auto result = splitString(command, delimiter_);
    // remove the first one since that is the command itself
    if (!result.empty()) {
        result.erase(result.begin());
    }
    return result;
}

std::vector<std::string> MessageConsumer::splitString(const std::string& input, char delimiter)
{
    std::vector<std::string> substrings;
    std::stringstream ss(input);
    std::string item;

    while (std::getline(ss, item, delimiter)) {
        substrings.push_back(item);
    }

    return substrings;
}

std::vector<std::string> MessageConsumer::splitString(const std::string& input, const std::string& delimiter)
{
    std::vector<std::string> substrings;
    std::string::size_type start = 0;
    std::string::size_type end;

    while ((end = input.find(delimiter, start)) != std::string::npos) {
        substrings.push_back(input.substr(start, end - start));
        start = end + delimiter.length();
    }
    // Add the last substring
    substrings.push_back(input.substr(start));

    return substrings;
}

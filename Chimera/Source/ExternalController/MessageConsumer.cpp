#include "stdafx.h"
#include "MessageConsumer.h"
#include "boost\lexical_cast.hpp"
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
            auto args = getArguments(message);
            if (args.size() == 0) {
                emit logMessage(qstr(timeStamp + ": \t" + "No arguemnt found in command: " + message));
                connection->do_write("Error\nNo arguemnt found in command: " + message);
                continue;
            }
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.startExperiment(qstr(args[0]), status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished starting experiment", status));
        }
        else if (stratWith(message, "Abort-Experiment")) {
            auto args = getArguments(message);
            if (args.size() < 2) {
                emit logMessage(qstr(timeStamp + ": \t" + "No arguemnt found in command: " + message));
                connection->do_write("Error\nNo arguemnt found in command: " + message);
                continue;
            }
            QMetaObject::invokeMethod(&modulator_, [&]() {
                bool keepData = !stratWith(str(args[0]), "delete");
                modulator_.abortExperiment(keepData, qstr(args[1]), status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished aborting experiment", status));
        }
        else if (stratWith(message, "Is-Experiment-Running?")) {
            bool isRunning;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.isExperimentRunning(isRunning, status);
                }, Qt::BlockingQueuedConnection);
            std::string isRunningStr = isRunning ? "TRUE" : "FALSE";
            connection->do_write(compileReply(isRunningStr + "\tFinished asking if experiment is running", status));
        }
        else if (stratWith(message, "Start-Calibration")) {
            auto args = getArguments(message);
            if (args.size() == 0) {
                emit logMessage(qstr(timeStamp + ": \t" + "No arguemnt found in command: " + message));
                connection->do_write("Error\nNo arguemnt found in command: " + message);
                continue;
            }
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.startCalibration(qstr(args[0]), status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished starting calibration", status));
        }
        else if (stratWith(message, "Is-Calibration-Running?")) {
            bool isRunning;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.isCalibrationRunning(isRunning, status);
                }, Qt::BlockingQueuedConnection);
            std::string isRunningStr = isRunning ? "TRUE" : "FALSE";
            connection->do_write(compileReply(isRunningStr + "\tFinished asking if calibration is running", status));
        }
        else if (stratWith(message, "Set-Static-DDS")) {
            auto args = getArguments(message);
            if (args.size() != 2) {
                emit logMessage(qstr(timeStamp + ": \t" + "Not enough arguemnt found in command: " + message));
                connection->do_write("Error\nNot enough arguemnt found in command: " + message);
                continue;
            }
            unsigned channel;
            try {
                channel = boost::lexical_cast<unsigned>(args[1]);
            }
            catch (boost::bad_lexical_cast&) { 
                emit logMessage(qstr(timeStamp + ": \t" + "Error in converting command argument to number: " + message));
                connection->do_write("Error\nError in converting command argument to number: " + message);
                continue;
            }
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.setStaticDDS(args[0], channel, status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished setting static DDS", status));
        }
        else if (stratWith(message, "Set-DAC")) {
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.setDAC(status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished setting DAC", status));
        }
        else if (stratWith(message, "Set-DDS")) {
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.setDDS(status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished setting DDS", status));
        }
        else if (stratWith(message, "Set-OL")) {
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.setOL(status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished setting OL", status));
        }
        else if (stratWith(message, "Get-MAKO-Image")) {
            auto args = getArguments(message);
            if (args.size() != 1) {
                emit logMessage(qstr(timeStamp + ": \t" + "Not enough arguemnt found in command: " + message));
                connection->do_write("Error\nNot enough arguemnt found in command: " + message);
                continue;
            }
            QVector<double> image;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.getMakoImage(qstr(args[0]), image, status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished getting MAKO image", image.toStdVector(), status));
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

std::vector<char> MessageConsumer::compileReply(std::string normalMsg, std::vector<double> data, CommandModulator::ErrorStatus status)
{
    std::vector<char> buffer; // normalMsg + delimiter($) + size of vector + vector

    std::size_t size = data.size();
    buffer.reserve(normalMsg.size() + 1 + sizeof(size) + sizeof(double) * size);
    // Copy the message and add the delimiter
    buffer.insert(buffer.end(), normalMsg.begin(), normalMsg.end());
    buffer.push_back(delimiter_);
    // Copy the size of the vector
    buffer.insert(buffer.end(), reinterpret_cast<char*>(&size), reinterpret_cast<char*>(&size) + sizeof(size));
    // Copy the vector data
    buffer.insert(buffer.end(), reinterpret_cast<const char*>(data.data()), reinterpret_cast<const char*>(data.data()) + sizeof(double) * size);

    return buffer;
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

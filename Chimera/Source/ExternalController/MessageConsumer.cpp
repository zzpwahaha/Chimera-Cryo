#include "stdafx.h"
#include "MessageConsumer.h"
#include <chrono>
#include <ctime>
#include <sstream>

MessageConsumer::MessageConsumer(ThreadsafeQueue<TCPMessageTyep>& queue) : queue_(queue) 
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
            }
            emit openConfiguration(qstr(args[0]));
            connection->do_write("Finished opening configuration: " + args[0]);
        }
        else if (stratWith(message, "Open")) {

        }
        else {
            emit logMessage(qstr(timeStamp + ": \t" + "Unrecongnized command: " + message));
            connection->do_write("Unrecongnized command: " + message);
        }
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

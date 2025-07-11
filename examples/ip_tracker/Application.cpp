/**
 * Copyright 2022 WolkAbout Technology s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "core/utilities/Logger.h"
#include "core/utilities/Timer.h"
#include "wolk/WolkBuilder.h"
#include "wolk/WolkSingle.h"

#include <arpa/inet.h>
#include <fstream>
#include <ifaddrs.h>
#include <iostream>
#include <netdb.h>
#include <random>
#include <string>
#include <vector>

// Synchronization tools
std::mutex mutex;
std::condition_variable conditionVariable;

// returns string from LogLevel
std::string toString(wolkabout::LogLevel level)
{
    if (level == wolkabout::LogLevel::INFO)
    {
        return "INFO";
    }
    else if (level == wolkabout::LogLevel::WARN)
    {
        return "WARN";
    }
    else if (level == wolkabout::LogLevel::ERROR)
    {
        return "ERROR";
    }
    else if (level == wolkabout::LogLevel::DEBUG)
    {
        return "DEBUG";
    }
    else if (level == wolkabout::LogLevel::TRACE)
    {
        return "TRACE";
    }
    else if (level == wolkabout::LogLevel::OFF)
    {
        return "OFF";
    }
    else
    {
        return "";
    }
}
// returns LogLevel and if string is valid LogLevel
std::pair<wolkabout::LogLevel, bool> fromString(std::string in)
{
    if (in == "INFO")
    {
        return std::make_pair(wolkabout::LogLevel::INFO, true);
    }
    else if (in == "WARN")
    {
        return std::make_pair(wolkabout::LogLevel::WARN, true);
    }
    else if (in == "ERROR")
    {
        return std::make_pair(wolkabout::LogLevel::ERROR, true);
    }
    else if (in == "DEBUG")
    {
        return std::make_pair(wolkabout::LogLevel::DEBUG, true);
    }
    else if (in == "TRACE")
    {
        return std::make_pair(wolkabout::LogLevel::TRACE, true);
    }
    else if (in == "OFF")
    {
        return std::make_pair(wolkabout::LogLevel::OFF, true);
    }
    else
    {
        return std::make_pair(wolkabout::LogLevel::INFO, false);
    }
}
// Class for handling recieved data from feed
class DeviceDataChangeHandler : public wolkabout::connect::FeedUpdateHandler
{
public:
    DeviceDataChangeHandler(std::string log, bool valid) : m_logLevel(log), m_isLogValid(valid) {}

    void handleUpdate(const std::string& deviceKey,
                      const std::map<std::uint64_t, std::vector<wolkabout::Reading>>& readings) override
    {
        // Go through all the timestamps
        for (const auto& pair : readings)
        {
            LOG(DEBUG) << "Received feed information for time: " << pair.first;

            // Take the readings, and apply them
            for (const auto& reading : pair.second)
            {
                LOG(DEBUG) << "Received feed information for reference '" << reading.getReference() << "'.";

                // Lock the mutex
                std::lock_guard<std::mutex> lock{mutex};

                // Check the reference on the readings
                if (reading.getReference() == "log")
                {
                    auto in = fromString(reading.getStringValue());
                    if (in.second)
                    {
                        wolkabout::Logger::getInstance().setLevel(in.first);
                        m_logLevel = toString(in.first);
                        LOG(INFO) << "Log has been updated";
                    }
                    else
                    {
                        LOG(INFO) << "Unknown log value";
                        m_isLogValid = false;
                        conditionVariable.notify_one();
                    }
                }
            }
        }
    }
    bool isLogValid() { return m_isLogValid; }
    std::string previousLogValue() { return m_logLevel; }
    void updateLogToValid() { m_isLogValid = true; }

private:
    std::string m_logLevel;
    bool m_isLogValid;
};
// trims white spaces
std::string trim(const std::string& str)
{
    size_t start = 0;
    while (start < str.length() && std::isspace(str[start]))
    {
        ++start;
    }

    size_t end = str.length();
    while (end > start && std::isspace(str[end - 1]))
    {
        --end;
    }

    return str.substr(start, end - start);
}
// Trying to read json config file
std::vector<std::string> readConfigJson(const std::string& path)
{
    std::vector<std::string> configInfo;

    std::ifstream file(path);
    if (file.fail())
    {
        return {};
    }
    std::string lineInFile;
    while (getline(file, lineInFile))
    {
        if (lineInFile.front() == '[')
        {
            continue;
        }
        lineInFile = trim(lineInFile);
        lineInFile = lineInFile.erase(0, 1);
        if (lineInFile[lineInFile.length() - 1] == ',')
        {
            lineInFile.erase(lineInFile.length() - 2, 2);
            configInfo.push_back(lineInFile);
        }
        else
        {
            lineInFile.erase(lineInFile.length() - 1, 1);
            configInfo.push_back(lineInFile);
            break;
        }
    }

    file.close();

    return configInfo;
}

// Function returns IP address in order loop, ethernet, wifi for IP4 and then for IP6
std::map<std::string, std::string> returnIpAddress()
{
    struct ifaddrs* ifaddr = NULL;
    struct ifaddrs* ifa = NULL;
    int family, s;
    void* tmpAddrPtr = NULL;
    std::map<std::string, std::string> ipMap;

    if (getifaddrs(&ifaddr) == -1)
    {
        return ipMap;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }

        if (ifa->ifa_addr->sa_family == AF_INET)
        {    // check if it is IP4
            tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            std::string firstChar = ifa->ifa_name;
            std::string key(1, firstChar[0]);
            key = key + "4";
            ipMap[key] = addressBuffer;
        }
        else if (ifa->ifa_addr->sa_family == AF_INET6)
        {    // check if it is IP6
            tmpAddrPtr = &((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            std::string firstChar = ifa->ifa_name;
            std::string key(1, firstChar[0]);
            key = key + "6";
            ipMap[key] = addressBuffer;
        }
    }
    freeifaddrs(ifaddr);
    return ipMap;
}

// Reads CPU temperature
double readCPUTemperature()
{
    const std::string path = "/sys/class/thermal/thermal_zone0/temp";
    std::ifstream tempFile(path);
    double temperature = 0.0;

    if (tempFile.is_open())
    {
        int tempInMilli;
        tempFile >> tempInMilli;
        temperature = tempInMilli / 1000.0;
        tempFile.close();
    }
    else
    {
        std::cout << "Unable to open temperature file " << std::endl;
    }
    return temperature;
}

int main(int argc, char** argv)
{
    // Checking for location of config file
    if (argv[1] == NULL)
    {
        std::cout << "Write the location of the json config file as an argument" << std::endl;
        return 0;
    }
    std::vector<std::string> config = readConfigJson(argv[1]);

    if (config.empty())
    {
        std::cout << "Couldn't load config file" << std::endl;
        return 0;
    }
    std::cout << "Config file loaded successfully" << std::endl;

    // Getting IP Address
    std::map<std::string, std::string> newIpMap = returnIpAddress();
    if (newIpMap.empty())
    {
        std::cout << "Couldn't find IP Address" << std::endl;
    }

    // This is the logger setup
    wolkabout::Logger::init(wolkabout::LogLevel::INFO,
                            wolkabout::Logger::Type::CONSOLE | wolkabout::Logger::Type::FILE);

    // Here we create the device that we are presenting as on the platform.
    auto device = wolkabout::Device(config[0], config[1], wolkabout::OutboundDataMode::PUSH);

    auto deviceInfoHandler = std::make_shared<DeviceDataChangeHandler>(toString(wolkabout::LogLevel::INFO), true);

    // And here we create the wolk session
    auto wolk =
      wolkabout::connect::WolkBuilder(device).host(config[2]).feedUpdateHandler(deviceInfoHandler).buildWolkSingle();
    wolk->connect();

    // Setting initial log value
    wolk->addReading("log", deviceInfoHandler->previousLogValue());

    // Initial readings for IP
    std::map<std::string, std::string> currentIpMap = newIpMap;
    for (auto const& element : currentIpMap)
    {
        wolk->addReading(element.first, element.second);
    }

    // Initial CPU temperature
    std::vector<double> cpuTemp;
    double initialCpuTemp = readCPUTemperature();
    wolk->addReading("cpuT", initialCpuTemp);

    // Making timers
    wolkabout::Timer timerIP, timerCpuTemp;

    // check every 5 minutes if the IP address changed
    timerIP.run(std::chrono::minutes(5),
                [&newIpMap, &currentIpMap, &wolk]
                {
                    newIpMap = returnIpAddress();
                    if (!(newIpMap == currentIpMap))
                    {
                        for (auto const& element : newIpMap)
                        {
                            if (currentIpMap[element.first] != element.second)
                            {
                                wolk->addReading(element.first, element.second);
                            }
                        }
                        currentIpMap = newIpMap;
                    }
                    wolk->publish();
                });

    // check every minute for new temperature and send the highest every 5 minutes
    timerCpuTemp.run(std::chrono::minutes(1),
                     [&cpuTemp, &wolk]
                     {
                         cpuTemp.push_back(readCPUTemperature());
                         if (cpuTemp.size() == 5)
                         {
                             auto maxTempAddr = std::max_element(cpuTemp.begin(), cpuTemp.end());
                             wolk->addReading("cpuT", *maxTempAddr);
                             wolk->publish();
                             cpuTemp.clear();
                         }
                     });
    while (true)
    {
        std::unique_lock<std::mutex> lock(mutex);
        conditionVariable.wait(lock, [&deviceInfoHandler] { return !deviceInfoHandler->isLogValid(); });
        wolk->addReading("log", deviceInfoHandler->previousLogValue());
        wolk->publish();
        deviceInfoHandler->updateLogToValid();
    }

    return 0;
}

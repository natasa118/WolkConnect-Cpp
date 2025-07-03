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

#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <ifaddrs.h>
#include <netdb.h>
#include <arpa/inet.h>

std::vector<std::string> readConfig(const std::string& path)
{
    std::ifstream file(path);

    if (file.fail())
    {
        return {};
    }

    std::vector<std::string> configInfo;
    std::string lineInFile;
    while (getline(file, lineInFile))
    {
        configInfo.push_back(lineInFile);
    }

    file.close();
    return configInfo;
}

/**
 * This is a function that will generate a random Temperature value for us.
 *
 * @return A new Temperature value, in the range of -20 to 80.
 */
std::uint64_t generateRandomValue()
{
    // Here we will create the random engine and distribution
    static auto engine =
      std::mt19937(static_cast<std::uint32_t>(std::chrono::system_clock::now().time_since_epoch().count()));
    static auto distribution = std::uniform_real_distribution<>(-20, 80);

    // And generate a random value
    return static_cast<std::uint64_t>(distribution(engine));
}

// Funsction returns ip address in order loop, ethernet, wifi for IP4 and then for IP6
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

void publishIPAddress() {}

int main(int argc, char** argv)
{
    // Checking for location of config file
    if (argv[1] == NULL)
    {
        std::cout << "Write the location of the config file as an argument" << std::endl;
        return 0;
    }
    std::vector<std::string> config = readConfig(argv[1]);
    if (config.empty())
    {
        std::cout << "Couldn't load config file" << std::endl;
        return 0;
    }
    std::cout << "Config file loaded successfully" << std::endl;

    // Getting Ip Address
    std::map<std::string, std::string> newIpMap = returnIpAddress();
    if (newIpMap.empty())
    {
        std::cout << "Couldn't find IP Address" << std::endl;
    }

    // This is the logger setup. Here you can set up the level of logging you would like enabled.
    wolkabout::Logger::init(wolkabout::LogLevel::INFO, wolkabout::Logger::Type::CONSOLE);

    // Here we create the device that we are presenting as on the platform.
    auto device = wolkabout::Device(config[0], config[1], wolkabout::OutboundDataMode::PUSH);

    // And here we create the wolk session
    auto wolk = wolkabout::connect::WolkSingle::newBuilder(device).host(config[2]).buildWolkSingle();
    wolk->connect();

    // send initial readings
    std::map<std::string, std::string> currentIpMap = newIpMap;
    for (auto const& element : currentIpMap)
    {
        wolk->addReading(element.first, element.second);
    }

    wolkabout::Timer timerIP;

    // check every 5mins if the ip address changed
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

    while (true)
    {
    }

    return 0;
}

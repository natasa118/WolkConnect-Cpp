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
#include "wolk/WolkBuilder.h"
#include "wolk/WolkSingle.h"

#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

std::vector<std::string> readConfig(std::string path)
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

int main(int argc, char** argv)
{
    if (argv[1] == NULL)
    {
        std::cout << "Write the location of the config file as an argument" << std::endl;
        return 0;
    }

    std::vector<std::string> config = readConfig(argv[1]);

    if (config.empty())
    {
        std::cout << "Couldn't load file" << std::endl;
        return 0;
    }

    // This is the logger setup. Here you can set up the level of logging you would like enabled.
    wolkabout::Logger::init(wolkabout::LogLevel::INFO, wolkabout::Logger::Type::CONSOLE);

    // Here we create the device that we are presenting as on the platform.
    auto device = wolkabout::Device(config[0], config[1], wolkabout::OutboundDataMode::PUSH);

    // And here we create the wolk session
    auto wolk = wolkabout::connect::WolkSingle::newBuilder(device).host(config[2]).buildWolkSingle();
    wolk->connect();

    // And now we will periodically (and endlessly) send a random temperature value.
    while (true)
    {
        wolk->addReading("T", generateRandomValue());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        wolk->publish();
    }

    return 0;
}

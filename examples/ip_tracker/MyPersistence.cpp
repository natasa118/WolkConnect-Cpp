#include "core/model/Attribute.h"
#include "core/persistence/Persistence.h"
#include "core/persistence/inmemory/InMemoryPersistence.h"
#include "core/utilities/Logger.h"
#include "core/utilities/Timer.h"
#include "core/utilities/nlohmann/json.hpp"
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
class MyPersistence : public wolkabout::Persistence
{
public:
    MyPersistence() { emptyFile(); }

    bool putReading(const std::string& key, const wolkabout::Reading& reading)
    {
        std::ofstream file;
        file.open(m_PERSISTENCE_FILE_PATH, std::ios_base::app);

        if (!file.is_open())
        {
            std::cout << "Couldn't open persistence file to write in" << std::endl;
            return false;
        }
        if (file.is_open())
        {
            file << "Reference: " << key << "      Reading: " << reading.getStringValue() << std::endl;
            file.flush();
            file.close();
            return true;
        }
        return false;
    }
    std::vector<std::shared_ptr<wolkabout::Reading>> getReadings(const std::string& key, std::uint_fast64_t count)
    {
        std::map<std::string, std::vector<std::shared_ptr<wolkabout::Reading>>> myReading = getRead();
        if (myReading.find(key) == myReading.end())
        {
            return {};
        }
        std::vector<std::shared_ptr<wolkabout::Reading>>& txtReadings = myReading.at(key);
        auto size = static_cast<uint_fast64_t>(txtReadings.size());

        return {txtReadings.begin() + static_cast<std::int64_t>((count < size) ? size - count : 0), txtReadings.end()};
    }

    void removeReadings(const std::string& key, uint_fast64_t count)
    {
        std::map<std::string, std::vector<std::shared_ptr<wolkabout::Reading>>> myReading = getRead();
        myReading.erase(key);

        emptyFile();

        std::ofstream file;
        file.open(m_PERSISTENCE_FILE_PATH, std::ios_base::app);

        if (!file.is_open())
        {
            std::cout << "Couldn't open persistence file to write in" << std::endl;
            return;
        }
        if (file.is_open())
        {
            for (auto pair : myReading)
            {
                std::string mapKey = pair.first;
                auto mapVector = pair.second;
                for (auto reading : mapVector)
                {
                    file << "Reference: " << mapKey << "      Reading: " << reading->getStringValue() << std::endl;
                }
            }
            file.flush();
            file.close();
        }
    }
    std::vector<std::string> getReadingsKeys() override
    {
        std::vector<std::string> keys;
        std::map<std::string, std::vector<std::shared_ptr<wolkabout::Reading>>> reading = getRead();
        for (const auto& pair : reading)
        {
            if (!pair.second.empty())
            {
                keys.push_back(pair.first);
            }
        }

        return keys;
    }
    bool putAttribute(const std::string& key, std::shared_ptr<wolkabout::Attribute> attribute) override
    {
        m_attributes.emplace(key, std::move(attribute));
        return true;
    }
    std::map<std::string, std::shared_ptr<wolkabout::Attribute>> getAttributes() override { return m_attributes; }
    std::shared_ptr<wolkabout::Attribute> getAttributeUnderKey(const std::string& key) override
    {
        const auto it = m_attributes.find(key);
        if (it == m_attributes.cend())
            return {};
        return it->second;
    }
    void removeAttributes() override { m_attributes.clear(); }
    void removeAttributes(const std::string& key) override
    {
        const auto it = m_attributes.find(key);
        if (it != m_attributes.cend())
            m_attributes.erase(it);
    }
    std::vector<std::string> getAttributeKeys() override
    {
        auto keys = std::vector<std::string>{};
        std::transform(
          m_attributes.cbegin(), m_attributes.cend(), keys.end(),
          [&](const std::pair<std::string, std::shared_ptr<wolkabout::Attribute>>& pair) { return pair.first; });
        return keys;
    }
    bool putParameter(const std::string& key, wolkabout::Parameter parameter) override
    {
        m_parameters.emplace(key, std::move(parameter));
        return true;
    }
    std::map<std::string, wolkabout::Parameter> getParameters() override { return m_parameters; }
    wolkabout::Parameter getParameterForKey(const std::string& key) override
    {
        const auto it = m_parameters.find(key);
        if (it == m_parameters.cend())
            return {};
        return it->second;
    }
    void removeParameters() override { m_parameters.clear(); };
    void removeParameters(const std::string& key) override
    {
        const auto it = m_parameters.find(key);
        if (it != m_parameters.cend())
            m_parameters.erase(it);
    }
    std::vector<std::string> getParameterKeys() override
    {
        auto keys = std::vector<std::string>{};
        std::transform(m_parameters.cbegin(), m_parameters.cend(), keys.end(),
                       [&](const std::pair<std::string, wolkabout::Parameter>& pair) { return pair.first; });
        return keys;
    }
    bool isEmpty() override
    {
        std::map<std::string, std::vector<std::shared_ptr<wolkabout::Reading>>> reading = getRead();
        if (reading.empty())
        {
            return true;
        }
        return false;
    }

    ~MyPersistence() override {}

private:
    const std::string m_PERSISTENCE_FILE_PATH = "./log_files/persistence_file";
    void emptyFile()
    {
        std::ofstream file(m_PERSISTENCE_FILE_PATH, std::ios::trunc);
        file.close();
    }
    std::map<std::string, std::shared_ptr<wolkabout::Attribute>> m_attributes;
    std::map<std::string, wolkabout::Parameter> m_parameters;
    std::map<std::string, std::vector<std::shared_ptr<wolkabout::Reading>>> getRead()
    {
        std::map<std::string, std::vector<std::shared_ptr<wolkabout::Reading>>> reading;
        std::string line;
        std::ifstream file(m_PERSISTENCE_FILE_PATH);
        if (!file.is_open())
        {
            std::cout << "Couldn't open persistence file to read from" << std::endl;
            return {};
        }
        while (std::getline(file, line))
        {
            std::size_t nbE_pos = line.find("nbE+");
            std::size_t reading_pos = line.find("Reading:");
            if (nbE_pos == std::string::npos || reading_pos == std::string::npos)
            {
                continue;
            }
            std::size_t key_start = nbE_pos;
            std::size_t key_end = line.find(" ", key_start);
            std::string key = line.substr(key_start, key_end - key_start);
            size_t position = key.find('+');
            std::string testKey = key.substr(position + 1);

            std::size_t value_start = reading_pos + 9;
            std::string value = line.substr(value_start);
            std::uint64_t now = static_cast<std::uint64_t>(std::time(nullptr));
            auto r = std::make_shared<wolkabout::Reading>(testKey, value, 0);
            reading[key].push_back(r);
        }
        file.close();
        return reading;
    }
};
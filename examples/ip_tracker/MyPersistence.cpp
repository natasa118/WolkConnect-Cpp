
#include "MyPersistence.h"

#include "core/persistence/Persistence.h"

#include <fstream>
#include <iostream>
#include <netdb.h>
#include <string>
#include <vector>

namespace natasa
{
MyPersistence::MyPersistence() {}
MyPersistence::MyPersistence(std::string path = natasa::defaultPersistenceFile) : m_persistenceFilePath{std::move(path)}
{
}

bool MyPersistence::putReading(const std::string& key, const wolkabout::Reading& reading)
{
    std::ofstream file;
    file.open(m_persistenceFilePath, std::ios_base::app);

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
std::vector<std::shared_ptr<wolkabout::Reading>> MyPersistence::getReadings(const std::string& key,
                                                                            std::uint_fast64_t count)
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

void MyPersistence::removeReadings(const std::string& key, uint_fast64_t count)
{
    std::map<std::string, std::vector<std::shared_ptr<wolkabout::Reading>>> myReading = getRead();
    myReading.erase(key);

    emptyFile();

    std::ofstream file;
    file.open(m_persistenceFilePath, std::ios_base::app);

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
std::vector<std::string> MyPersistence::getReadingsKeys()
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
bool MyPersistence::putAttribute(const std::string& key, std::shared_ptr<wolkabout::Attribute> attribute)
{
    return true;
}
std::map<std::string, std::shared_ptr<wolkabout::Attribute>> MyPersistence::getAttributes()
{
    return {};
}
std::shared_ptr<wolkabout::Attribute> MyPersistence::getAttributeUnderKey(const std::string& key)
{
    return {};
}
void MyPersistence::removeAttributes() {}
void MyPersistence::removeAttributes(const std::string& key) {}
std::vector<std::string> MyPersistence::getAttributeKeys()
{
    return {};
}
bool MyPersistence::putParameter(const std::string& key, wolkabout::Parameter parameter)
{
    return false;
}
std::map<std::string, wolkabout::Parameter> MyPersistence::getParameters()
{
    return {};
}
wolkabout::Parameter MyPersistence::getParameterForKey(const std::string& key)
{
    return {};
}
void MyPersistence::removeParameters() {}
void MyPersistence::removeParameters(const std::string& key) {}
std::vector<std::string> MyPersistence::getParameterKeys()
{
    return {};
}
bool MyPersistence::isEmpty()
{
    std::map<std::string, std::vector<std::shared_ptr<wolkabout::Reading>>> reading = getRead();
    if (reading.empty())
    {
        return true;
    }
    return false;
}

void MyPersistence::emptyFile()
{
    std::ofstream file(m_persistenceFilePath, std::ios::trunc);
    file.close();
}
std::map<std::string, std::vector<std::shared_ptr<wolkabout::Reading>>> MyPersistence::getRead()
{
    std::map<std::string, std::vector<std::shared_ptr<wolkabout::Reading>>> reading;
    std::string line;
    std::ifstream file(m_persistenceFilePath);
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
}    // namespace natasa

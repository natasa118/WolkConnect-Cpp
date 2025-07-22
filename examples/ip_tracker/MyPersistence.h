#ifndef MYPERSISTENCE_H
#define MYPERSISTENCE_H

#include "core/model/Attribute.h"
#include "core/persistence/Persistence.h"
#include "wolk/WolkBuilder.h"
#include "wolk/WolkSingle.h"

class MyPersistence : public wolkabout::Persistence
{
public:
    MyPersistence();
    MyPersistence(std::string path);
    ~MyPersistence() override = default;

    bool putReading(const std::string& key, const wolkabout::Reading& reading) override;

    std::vector<std::shared_ptr<wolkabout::Reading>> getReadings(const std::string& key,
                                                                 std::uint_fast64_t count) override;

    void removeReadings(const std::string& key, std::uint_fast64_t count) override;

    std::vector<std::string> getReadingsKeys() override;

    bool putAttribute(const std::string& key, std::shared_ptr<wolkabout::Attribute> attribute) override;

    std::map<std::string, std::shared_ptr<wolkabout::Attribute>> getAttributes() override;

    std::shared_ptr<wolkabout::Attribute> getAttributeUnderKey(const std::string& key) override;

    void removeAttributes() override;

    void removeAttributes(const std::string& key) override;

    std::vector<std::string> getAttributeKeys() override;

    bool putParameter(const std::string& key, wolkabout::Parameter parameter) override;

    std::map<std::string, wolkabout::Parameter> getParameters() override;

    wolkabout::Parameter getParameterForKey(const std::string& key) override;

    void removeParameters() override;

    void removeParameters(const std::string& key) override;

    std::vector<std::string> getParameterKeys() override;

    bool isEmpty() override;

private:
    std::string m_persistenceFilePath = "./log_files/persistence_file";
    void emptyFile();
    std::map<std::string, std::vector<std::shared_ptr<wolkabout::Reading>>> getRead();
};

#endif

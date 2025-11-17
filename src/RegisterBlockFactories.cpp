#include <PySysLinkBase/IBlockFactory.h>
#include "BlockFactoryPython.h"
#include "spdlog/spdlog.h"
#include <iostream>
#include "LoggerInstance.h"

extern "C" void RegisterBlockFactories(std::map<std::string, std::shared_ptr<PySysLinkBase::IBlockFactory>>& registry, std::map<std::string, PySysLinkBase::ConfigurationValue> pluginConfiguration) {
    std::cout << "Call to RegisterBlockFactories" << std::endl;
    registry["BasicPython"] = std::make_shared<BlockTypeSupports::BasicPythonSupport::BlockFactoryPython>(pluginConfiguration);
    std::cout << "End of function" << std::endl;
}

extern "C" void RegisterSpdlogLogger(std::shared_ptr<spdlog::logger> logger) {
    BlockTypeSupports::BasicPythonSupport::LoggerInstance::SetLogger(logger);
    BlockTypeSupports::BasicPythonSupport::LoggerInstance::GetLogger()->debug("Logger from plugin BlockTypeSupportsBasicPython!");
}

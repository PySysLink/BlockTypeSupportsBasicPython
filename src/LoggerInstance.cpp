#include "LoggerInstance.h"

namespace BlockTypeSupports::BasicPythonSupport
{
    std::shared_ptr<spdlog::logger> LoggerInstance::s_logger = nullptr;
} // namespace BlockTypeSupports::BasicPythonSupport

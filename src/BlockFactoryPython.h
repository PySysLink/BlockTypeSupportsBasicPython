#ifndef SRC_PYTHON_BLOCK_FACTORY_H
#define SRC_PYTHON_BLOCK_FACTORY_H

#include <PySysLinkBase/IBlockFactory.h>
#include <PySysLinkBase/IBlockEventsHandler.h>
#include <PySysLinkBase/ConfigurationValue.h>

#include "SimulationBlockPython.h"
#include <Python.h>
#include <memory>
#include <string>
#include <map>
#include <stdexcept>
#include "spdlog/spdlog.h"

namespace BlockTypeSupports::BasicPythonSupport
{

class BlockFactoryPython : public PySysLinkBase::IBlockFactory
{
public:
    BlockFactoryPython(std::map<std::string, PySysLinkBase::ConfigurationValue> pluginConfiguration)
    {
        // Initialize Python interpreter only once
        if (!Py_IsInitialized())
        {
            Py_Initialize();
        }

        std::vector<std::string> pythonModulePaths;
        try {
            pythonModulePaths = PySysLinkBase::ConfigurationValueManager::TryGetConfigurationValue<std::vector<std::string>>("PythonModulePaths", pluginConfiguration);
        } catch (std::out_of_range&) {
            pythonModulePaths.push_back("."); // fallback: current folder
        }

        PyGILState_STATE gstate = PyGILState_Ensure();

        PyObject* sysPath = PySys_GetObject("path"); // borrowed reference

        for (const auto& pathStr : pythonModulePaths) {
            PyObject* path = PyUnicode_FromString(pathStr.c_str());
            if (PyList_Append(sysPath, path) != 0)
            {
                spdlog::warn("Failed to append {} to Python sys.path", pathStr);
            }
            Py_DECREF(path);
        }
            
        PyGILState_Release(gstate);
    }

    std::shared_ptr<PySysLinkBase::ISimulationBlock>
    CreateBlock(std::map<std::string, PySysLinkBase::ConfigurationValue> blockConfiguration,
                std::shared_ptr<PySysLinkBase::IBlockEventsHandler> eventHandler) override
    {
        std::string signalType = "Double";
        try
        {
            signalType = PySysLinkBase::ConfigurationValueManager::TryGetConfigurationValue<std::string>("SignalType", blockConfiguration);
        }
        catch (std::out_of_range&)
        {
            // default: Double
        }

        spdlog::debug("Creating BasicPython block with signal type {}", signalType);

        if (signalType == "Double")
        {
            return std::make_shared<SimulationBlockPython<double>>(blockConfiguration, eventHandler);
        }
        else if (signalType == "Complex")
        {
            return std::make_shared<SimulationBlockPython<std::complex<double>>>(blockConfiguration, eventHandler);
        }
        else
        {
            throw std::invalid_argument("Unsupported SignalType: " + signalType);
        }
    }
};

} // namespace BlockTypeSupports::BasicPythonSupport

#endif // SRC_PYTHON_BLOCK_FACTORY_H

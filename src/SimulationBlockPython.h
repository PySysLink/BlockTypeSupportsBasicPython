#ifndef SRC_PYTHON_SIMULATION_BLOCK_H
#define SRC_PYTHON_SIMULATION_BLOCK_H

#pragma once

#include <Python.h>

#include <memory>
#include <string>
#include <vector>
#include <complex>
#include <stdexcept>
#include <type_traits>

#include <PySysLinkBase/ISimulationBlock.h>
#include <PySysLinkBase/IBlockEventsHandler.h>
#include <PySysLinkBase/PortsAndSignalValues/InputPort.h>
#include <PySysLinkBase/PortsAndSignalValues/OutputPort.h>
#include <PySysLinkBase/SampleTime.h>
#include <PySysLinkBase/ConfigurationValue.h>
#include <spdlog/spdlog.h>

#include "ConfigurationValueManager.h"
#include "SimulationBlockPythonConversions.h"

namespace BlockTypeSupports::BasicPythonSupport
{

/*
 * Minimal templated Python-backed ISimulationBlock.
 *
 * Supported T: double, std::complex<double>
 *
 * Expected Python API:
 *   class MyBlock:
 *       def __init__(self, config: dict):
 *           ...
 *       def compute(self, inputs: list, t: float) -> list:
 *           # return list of outputs (same length as NumOutputs)
 */

template <typename T>
class SimulationBlockPython : public PySysLinkBase::ISimulationBlock
{

public:
    SimulationBlockPython(std::map<std::string, PySysLinkBase::ConfigurationValue> blockConfiguration,
                          std::shared_ptr<PySysLinkBase::IBlockEventsHandler> eventsHandler)
        : ISimulationBlock(blockConfiguration, eventsHandler)
    {
        // read configuration
        moduleName = PySysLinkBase::ConfigurationValueManager::TryGetConfigurationValue<std::string>("PythonModule", blockConfiguration);
        className = PySysLinkBase::ConfigurationValueManager::TryGetConfigurationValue<std::string>("PythonClass", blockConfiguration);

        std::vector<PySysLinkBase::SampleTimeType> supportedSampleTimeTypes = {};
        supportedSampleTimeTypes.push_back(PySysLinkBase::SampleTimeType::continuous);
        supportedSampleTimeTypes.push_back(PySysLinkBase::SampleTimeType::discrete);
        this->sampleTime = std::make_shared<PySysLinkBase::SampleTime>(PySysLinkBase::SampleTimeType::inherited, supportedSampleTimeTypes);

        // optional: number of ports
        try {
            numInputs = PySysLinkBase::ConfigurationValueManager::TryGetConfigurationValue<int>("NumInputs", blockConfiguration);
        } catch(std::out_of_range&) {
            numInputs = 1;
        }
        try {
            numOutputs = PySysLinkBase::ConfigurationValueManager::TryGetConfigurationValue<int>("NumOutputs", blockConfiguration);
        } catch(std::out_of_range&) {
            numOutputs = 1;
        }

        // create ports
        for (int i = 0; i < numInputs; ++i)
        {
            std::shared_ptr<PySysLinkBase::UnknownTypeSignalValue> signalValue = std::make_shared<PySysLinkBase::SignalValue<T>>(PySysLinkBase::SignalValue<T>(0.0));
            auto inputPort = std::make_shared<PySysLinkBase::InputPort>(PySysLinkBase::InputPort(false, signalValue));
            inputPorts.push_back(inputPort);
        }
        for (int i = 0; i < numOutputs; ++i)
        {
            std::shared_ptr<PySysLinkBase::UnknownTypeSignalValue> signalValue = std::make_shared<PySysLinkBase::SignalValue<T>>(PySysLinkBase::SignalValue<T>(0.0));
            auto outputPort = std::make_shared<PySysLinkBase::OutputPort>(PySysLinkBase::OutputPort(signalValue));
            this->outputPorts.push_back(outputPort);
        }

        // Prepare python and instantiate the class
        PyGILState_STATE gstate = PyGILState_Ensure();

        PyObject* pyName = PyUnicode_FromString(moduleName.c_str());
        pyModule = PyImport_Import(pyName);
        Py_DECREF(pyName);

        if (!pyModule)
        {
            PyGILState_Release(gstate);
            throw std::runtime_error("SimulationBlockPython: Could not import module: " + moduleName);
        }

        pyClass = PyObject_GetAttrString(pyModule, className.c_str());
        if (!pyClass || !PyCallable_Check(pyClass))
        {
            Py_XDECREF(pyClass);
            Py_DECREF(pyModule);
            PyGILState_Release(gstate);
            throw std::runtime_error("SimulationBlockPython: Class not found or not callable: " + className);
        }

        // build Python dict from blockConfiguration (simple string values)
        PyObject* pyConfig = PyDict_New();
        for (auto& kv : blockConfiguration) {
            PyObject* pyVal = PySysLinkBase::ConfigurationValueToPyObject(kv.second);
            PyDict_SetItemString(pyConfig, kv.first.c_str(), pyVal);
            Py_DECREF(pyVal);
        }

        // instantiate: cls(config)
        pyInstance = PyObject_CallFunctionObjArgs(pyClass, pyConfig, NULL);
        Py_DECREF(pyConfig);

        if (!pyInstance)
        {
            Py_DECREF(pyClass);
            Py_DECREF(pyModule);
            PyGILState_Release(gstate);
            throw std::runtime_error("SimulationBlockPython: Could not instantiate class: " + className);
        }

        PyGILState_Release(gstate);

        // Optionally call initialize() on python side if exists
        CallOptionalVoidMethod("initialize");
    }

    ~SimulationBlockPython()
    {
        PyGILState_STATE gstate = PyGILState_Ensure();
        Py_XDECREF(pyInstance);
        Py_XDECREF(pyClass);
        Py_XDECREF(pyModule);
        PyGILState_Release(gstate);
    }

    // ISimulationBlock required overrides
    const std::shared_ptr<PySysLinkBase::SampleTime> GetSampleTime() const override
    {
        return this->sampleTime;
    }

    void SetSampleTime(std::shared_ptr<PySysLinkBase::SampleTime> st) override
    {
        this->sampleTime = st;
    }

    std::vector<std::shared_ptr<PySysLinkBase::InputPort>> GetInputPorts() const override
    {
        return this->inputPorts;
    }

    const std::vector<std::shared_ptr<PySysLinkBase::OutputPort>> GetOutputPorts() const override
    {
        return this->outputPorts;
    }

    // Main compute bridge: calls python_instance.compute(inputs, currentTime)
    const std::vector<std::shared_ptr<PySysLinkBase::OutputPort>> _ComputeOutputsOfBlock(
        const std::shared_ptr<PySysLinkBase::SampleTime> sampleTime, double currentTime, bool isMinorStep = false) override
    {
        PyGILState_STATE gstate = PyGILState_Ensure();

        // Build Python list of inputs
        PyObject* pyInputs = PyList_New(inputPorts.size());
        for (size_t i = 0; i < inputPorts.size(); ++i)
        {
            auto inputValue = this->inputPorts[i]->GetValue();
            auto inputValueSignal = inputValue->TryCastToTyped<T>();
            PyObject* pyv = ToPyObject<T>(inputValueSignal->GetPayload());
            PyList_SetItem(pyInputs, (Py_ssize_t)i, pyv); // steals reference
        }

        // call compute(inputs, currentTime)
        PyObject* pyResult = PyObject_CallMethod(pyInstance, "compute", "(Od)", pyInputs, currentTime);
        Py_DECREF(pyInputs);

        if (!pyResult)
        {
            // fetch python error as string for better diagnostics (non-fatal here -> throw)
            PyErr_Print();
            PyGILState_Release(gstate);
            throw std::runtime_error("SimulationBlockPython: python compute() call failed");
        }

        // Accept any iterable: try to convert to sequence/list
        PyObject* seq = PySequence_Fast(pyResult, "python compute() must return a sequence");
        if (!seq)
        {
            Py_DECREF(pyResult);
            PyGILState_Release(gstate);
            throw std::runtime_error("SimulationBlockPython: compute() did not return a sequence");
        }

        Py_ssize_t outCount = PySequence_Fast_GET_SIZE(seq);
        if ((size_t)outCount < outputPorts.size())
        {
            Py_DECREF(seq);
            Py_DECREF(pyResult);
            PyGILState_Release(gstate);
            throw std::runtime_error("SimulationBlockPython: compute() returned fewer outputs than NumOutputs");
        }

        for (size_t i = 0; i < outputPorts.size(); ++i)
        {
            PyObject* item = PySequence_Fast_GET_ITEM(seq, (Py_ssize_t)i); // borrowed reference
            T val = FromPyObject<T>(item);
            std::shared_ptr<PySysLinkBase::UnknownTypeSignalValue> outputValue = this->outputPorts[i]->GetValue();
            auto outputValueSignal = outputValue->TryCastToTyped<T>();
            outputValueSignal->SetPayload(val);
            this->outputPorts[i]->SetValue(std::make_shared<PySysLinkBase::SignalValue<T>>(*outputValueSignal));
        }

        Py_DECREF(seq);
        Py_DECREF(pyResult);

        PyGILState_Release(gstate);
        return outputPorts;
    }

    // Minimal config update support (no dynamic changes)
    bool _TryUpdateConfigurationValue(std::string /*keyName*/, PySysLinkBase::ConfigurationValue /*value*/) override
    {
        return false;
    }

private:
    // configuration
    std::string moduleName;
    std::string className;
    int numInputs = 1;
    int numOutputs = 1;

    // ports and sample time
    std::shared_ptr<PySysLinkBase::SampleTime> sampleTime;
    std::vector<std::shared_ptr<PySysLinkBase::InputPort>> inputPorts;
    std::vector<std::shared_ptr<PySysLinkBase::OutputPort>> outputPorts;

    // python objects
    PyObject* pyModule = nullptr;
    PyObject* pyClass = nullptr;
    PyObject* pyInstance = nullptr;

private:
    // Helper: call optional no-arg method on instance
    void CallOptionalVoidMethod(const char* methodName)
    {
        PyGILState_STATE gstate = PyGILState_Ensure();
        if (pyInstance && PyObject_HasAttrString(pyInstance, methodName))
        {
            PyObject* res = PyObject_CallMethod(pyInstance, const_cast<char*>(methodName), nullptr);
            Py_XDECREF(res);
        }
        PyGILState_Release(gstate);
    }
};

} // namespace BlockTypeSupports::BasicPythonSupport

#endif // SRC_PYTHON_SIMULATION_BLOCK_H

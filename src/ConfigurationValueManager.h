#ifndef CONFIGURATION_VALUE_MANAGER_H
#define CONFIGURATION_VALUE_MANAGER_H

#include <Python.h>

namespace PySysLinkBase {

inline PyObject* ConfigurationValueToPyObject(const ConfigurationValue& val)
{
    return std::visit([](auto&& v) -> PyObject* {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<T, int>)
            return PyLong_FromLong(v);
        else if constexpr (std::is_same_v<T, double>)
            return PyFloat_FromDouble(v);
        else if constexpr (std::is_same_v<T, bool>)
            return PyBool_FromLong(v ? 1 : 0);
        else if constexpr (std::is_same_v<T, std::complex<double>>)
            return PyComplex_FromDoubles(v.real(), v.imag());
        else if constexpr (std::is_same_v<T, std::string>)
            return PyUnicode_FromString(v.c_str());
        else if constexpr (std::is_same_v<T, std::vector<int>>) {
            PyObject* list = PyList_New(v.size());
            for (size_t i = 0; i < v.size(); ++i)
                PyList_SET_ITEM(list, i, PyLong_FromLong(v[i]));
            return list;
        }
        else if constexpr (std::is_same_v<T, std::vector<double>>) {
            PyObject* list = PyList_New(v.size());
            for (size_t i = 0; i < v.size(); ++i)
                PyList_SET_ITEM(list, i, PyFloat_FromDouble(v[i]));
            return list;
        }
        else if constexpr (std::is_same_v<T, std::vector<bool>>) {
            PyObject* list = PyList_New(v.size());
            for (size_t i = 0; i < v.size(); ++i)
                PyList_SET_ITEM(list, i, PyBool_FromLong(v[i]));
            return list;
        }
        else if constexpr (std::is_same_v<T, std::vector<std::complex<double>>>) {
            PyObject* list = PyList_New(v.size());
            for (size_t i = 0; i < v.size(); ++i)
                PyList_SET_ITEM(list, i, PyComplex_FromDoubles(v[i].real(), v[i].imag()));
            return list;
        }
        else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
            PyObject* list = PyList_New(v.size());
            for (size_t i = 0; i < v.size(); ++i)
                PyList_SET_ITEM(list, i, PyUnicode_FromString(v[i].c_str()));
            return list;
        }
        else if constexpr (std::is_same_v<T, ConfigurationValuePrimitive>) {
            return ConfigurationValueToPyObject(v); // recurse
        }
        else if constexpr (std::is_same_v<T, std::vector<ConfigurationValuePrimitive>>) {
            PyObject* list = PyList_New(v.size());
            for (size_t i = 0; i < v.size(); ++i)
                PyList_SET_ITEM(list, i, ConfigurationValueToPyObject(v[i]));
            return list;
        }
        else {
            PyErr_SetString(PyExc_TypeError, "Unsupported configuration value type");
            return nullptr;
        }
    }, val);
}

} // namespace PySysLinkBase

#endif // CONFIGURATION_VALUE_MANAGER_H

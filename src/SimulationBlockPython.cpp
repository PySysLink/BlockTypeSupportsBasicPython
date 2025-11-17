#include "SimulationBlockPython.h"
#include <stdexcept>
#include <complex>

namespace BlockTypeSupports::BasicPythonSupport
{

// double implementation
template <typename T>
double SimulationBlockPython<T>::FromPyObject(PyObject* obj) const
{
    if (PyFloat_Check(obj))
        return PyFloat_AsDouble(obj);
    if (PyLong_Check(obj))
        return static_cast<double>(PyLong_AsLong(obj));
    throw std::runtime_error("FromPyObject<double>: object is not a number");
}

template <typename T>
PyObject* SimulationBlockPython<T>::ToPyObject(const double& v) const
{
    return PyFloat_FromDouble(v);
}

// complex<double> implementation
template <typename T>
std::complex<double> SimulationBlockPython<T>::FromPyObject(PyObject* obj) const
{
    if (PyComplex_Check(obj))
        return {PyComplex_RealAsDouble(obj), PyComplex_ImagAsDouble(obj)};
    if (PyFloat_Check(obj) || PyLong_Check(obj))
        return {PyFloat_AsDouble(obj), 0.0};
    throw std::runtime_error("FromPyObject<complex<double>>: object is not a number or complex");
}

template <typename T>
PyObject* SimulationBlockPython<T>::ToPyObject(const std::complex<double>& v) const
{
    return PyComplex_FromDoubles(v.real(), v.imag());
}

// Explicit instantiations of the class template
template class SimulationBlockPython<double>;
template class SimulationBlockPython<std::complex<double>>;

} // namespace BlockTypeSupports::BasicPythonSupport

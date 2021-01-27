/**
 * @file Utils.h
 *
 * \brief Useful utilities.
 *
 * @author Dimitar Stanev <jimstanev@gmail.com>
 */
#ifndef UTILS_H
#define UTILS_H

#include "internal/CommonExports.h"

#include <SimTKcommon/internal/BigMatrix.h>
#include <SimTKcommon/internal/negator.h>
#include <fstream>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>

namespace OpenSimRT {

/**
 * Converts an OpenSim::Array to std container <U> (e.g., vector<double>).
 */
template <typename T, typename U>
void osimToStd(const T& srcArray, U& dstVector) {
    dstVector.clear();
    int size = srcArray.getSize();
    dstVector.resize(size);
    for (int i = 0; i < size; ++i) { dstVector.at(i) = srcArray.get(i); }
}

/**
 * Converts a Simbody (e.g., Vector) to std container (e.g., vector<double>).
 */
template <typename T, typename U>
void simtkToStd(const T& srcArray, U& dstVector) {
    dstVector.clear();
    int size = srcArray.size();
    dstVector.resize(size);
    for (int i = 0; i < size; ++i) { dstVector.at(i) = srcArray[i]; }
}

/**
 * Converts <T> to string and with defined precision (in case of number).
 */
template <typename T>
std::string toString(const T& value,
                     int precision = std::numeric_limits<int>::infinity()) {
    std::ostringstream oss;
    if (precision != std::numeric_limits<int>::infinity()) {
        oss << std::setprecision(precision);
    }
    oss << value;
    return oss.str();
}

/**
 * Separates (delimiter) the values of the std container into a single line
 * string.
 */
template <typename T>
std::string dump(const T& vec, std::string delimiter,
                 int precision = std::numeric_limits<int>::infinity()) {
    std::string row = toString(vec.at(0));
    for (int i = 1; i < vec.size(); ++i) {
        row += delimiter;
        row += toString(vec.at(i));
    }
    return row;
}

/**
 * Vec<T> to double*
 */
template <int T>
void vecToDouble(const SimTK::Vec<T>& source, double* destination) {
    for (int i = 0; i < source.size(); ++i) destination[i] = source[i];
}
/**
 * Vector_<T> to double*
 */
template <typename T>
void vectorToDouble(const SimTK::Vector_<T>& source, double* destination) {
    for (int i = 0; i < source.size(); ++i) destination[i] = source[i];
}

} // namespace OpenSimRT

template <typename T> bool isVectorFinite(const SimTK::Vector_<T>& v) {
    for (size_t i = 0; i < v.size(); ++i) {
        if (!SimTK::isFinite(v[i])) return false;
    }
    return true;
}

// basic sliding window implementation
template <typename T> struct SlidingWindow {
    std::vector<T> data;  // sliding window data
    std::size_t capacity; // sliding window size

    // set initial values
    void init(std::vector<T>&& aData) {
        data = std::forward<std::vector<T>>(aData);
        capacity = data.size();
    }

    // insert element
    void insert(const T& x) {
        if (data.size() == capacity) data.erase(data.begin());
        data.push_back(x);
    }

    // reserve space in memory
    void setSize(const std::size_t& size) {
        capacity = size;
        data.reserve(size);
    }

    // compute mean value
    T mean() {
        return 1.0 * std::accumulate(data.begin(), data.end(), T()) /
               int(data.size());
    }
};

// Compute the projection of a point or vector on a arbitrary plane.
static SimTK::Vec3 projectionOnPlane(const SimTK::Vec3& point, const SimTK::Vec3& planeOrigin,
                              const SimTK::Vec3& planeNormal) {
    return point - SimTK::dot(point - planeOrigin, planeNormal) * planeNormal;
}

#endif

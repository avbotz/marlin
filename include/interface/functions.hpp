#ifndef INTERFACE_FUNCTIONS_HPP
#define INTERFACE_FUNCTIONS_HPP

#include <string>

#include "interface/data.hpp"

bool hydrophones(Data*, const std::string);

bool vision(Data*, const std::string, const std::string, size_t, size_t);

bool camera(Data*, const std::string, const std::string, size_t);

bool mission(Data*, const std::string, const std::string);
bool modeling(Data*, const std::string, const std::string);
bool control(Data*, const std::string, const std::string);

#endif


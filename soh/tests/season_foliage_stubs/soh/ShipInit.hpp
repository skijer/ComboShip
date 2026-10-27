#pragma once
#include <functional>
#include <set>
#include <string>
struct RegisterShipInitFunc {
    RegisterShipInitFunc(std::function<void()>, const std::set<std::string>& = {}) {
    }
};

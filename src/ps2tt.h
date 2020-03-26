#pragma once

#include <nlohmann/json.hpp>

nlohmann::json Ps2Tt(const nlohmann::json &glyf, double errorBound = 1);

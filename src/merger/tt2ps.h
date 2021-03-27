#pragma once

#include <nlohmann/json.hpp>

nlohmann::json Tt2Ps(const nlohmann::json &glyf, bool roundToInt = true);

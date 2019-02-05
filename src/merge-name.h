#pragma once

#include <vector>

#include <nlohmann/json.hpp>

nlohmann::json MergeNameTable(const std::vector<nlohmann::json> &nametables);

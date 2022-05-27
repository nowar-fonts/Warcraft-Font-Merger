#pragma once

#include <string>
#include <tuple>
#include <vector>

#include <nlohmann/json.hpp>

void MergeNameTable(std::vector<nlohmann::json> &nameTables,
                    nlohmann::json &font, const std::string &overrideNameStyle);

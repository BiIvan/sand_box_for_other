#pragma once
//2026-04-26 10:32
#include "json.h"

namespace json_reader {

  json::Document ProcessRequests(const json::Document& input, std::string map_str = "");

}
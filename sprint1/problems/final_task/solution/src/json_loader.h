#pragma once

#include <filesystem>


#include "model.h"

namespace json_loader
{

    model::Game LoadGame(const std::filesystem::path &json_path);
    std::string ReadTextFile(const std::filesystem::path &json_path);
    model::Game ParseGame(const std::string &json_str);

} // namespace json_loader

#pragma once

#include <filesystem>
#include <exception>
#include <string>
#include <boost/json.hpp>

#include "model.h"

namespace json = boost::json;

namespace extra_data
{
    class ExtraData
    {
    public:
        void SetLootType(std::string map_id, boost::json::array loot_types)
        {
            map_id_to_loot_types_[map_id] = loot_types;
        }

        std::optional<boost::json::array> GetLootType(std::string map_id)
        {
            if (map_id_to_loot_types_.count(map_id))
            {
                return map_id_to_loot_types_.at(map_id);
            }
            return std::nullopt;
        }

    private:
        std::map<std::string, boost::json::array> map_id_to_loot_types_;
    };
} // namespace extra_data

namespace json_loader
{

    class JsonLoaderException : public std::exception
    {
    private:
        std::string message;

    public:
        JsonLoaderException(const std::string &msg) : message(msg) {}

        const char *what() const noexcept override
        {
            return message.c_str();
        }
    };

    struct JsonStrConst
    {
        JsonStrConst() = delete;
        constexpr static boost::json::string_view x = "x";
        constexpr static boost::json::string_view x0 = "x0";
        constexpr static boost::json::string_view x1 = "x1";
        constexpr static boost::json::string_view y = "y";
        constexpr static boost::json::string_view y0 = "y0";
        constexpr static boost::json::string_view y1 = "y1";
        constexpr static boost::json::string_view h = "h";
        constexpr static boost::json::string_view w = "w";
        constexpr static boost::json::string_view id = "id";
        constexpr static boost::json::string_view dx = "offsetX";
        constexpr static boost::json::string_view dy = "offsetY";
        constexpr static boost::json::string_view name = "name";
        constexpr static boost::json::string_view maps = "maps";
        constexpr static boost::json::string_view roads = "roads";
        constexpr static boost::json::string_view buildings = "buildings";
        constexpr static boost::json::string_view offices = "offices";
    };

    model::Game LoadGame(const std::filesystem::path &json_path);
    std::string ReadTextFile(const std::filesystem::path &json_path);
    model::Game ParseGame(const std::string &json_str);
    model::Road ParseRoad(boost::json::value js_road);
    model::Building ParseBuilding(boost::json::value building);
    model::Office ParseOffice(boost::json::value office);
    model::Map ParseMap(boost::json::value map, double def_dog_speed);

    extra_data::ExtraData LoadExtraData(const std::filesystem::path &json_path);

} // namespace json_loader

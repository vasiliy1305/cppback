#include "json_loader.h"

#include <fstream>
#include <iostream>
#include <string>

#include <boost/json.hpp>
// // Этот заголовочный файл надо подключить в одном и только одном .cpp-файле программы
// #include <boost/json/src.hpp>

namespace json = boost::json;
using namespace std::literals;

namespace json_loader
{

    std::string ReadTextFile(const std::filesystem::path &json_path)
    {
        // Check if the file exists
        if (!std::filesystem::exists(json_path))
        {
            std::cerr << "File does not exist!\n";
            // можно добавить исключение
        }

        // Open the file
        std::ifstream file(json_path);

        // Check if the file was opened successfully
        if (!file.is_open())
        {
            std::cerr << "Unable to open file!\n";
            // можно добавить исключение
        }

        // Read the entire file into a string using std::istreambuf_iterator
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        // Close the file
        file.close();

        return content;
    }

    model::Game LoadGame(const std::filesystem::path &json_path)
    {
        // Загрузить содержимое файла json_path, например, в виде строки
        auto content = ReadTextFile(json_path);

        return ParseGame(content);
    }

    model::Game ParseGame(const std::string &json_str)
    {
        // Распарсить строку как JSON, используя boost::json::parse
        auto value = json::parse(json_str);
        // Загрузить модель игры из файла
        model::Game game;

        auto maps = value.as_object().at("maps").as_array();
        // std::cout << maps << std::endl;

        for (const auto &map : maps)
        {
            std::string id(map.at("id").as_string());
            std::string name(map.at("name").as_string());

            model::Map model_map(model::Map::Id(id), name);

            auto roads = map.at("roads").as_array();
            for (auto road : roads)
            {
                model::Point start_point;
                start_point.x = road.as_object().at("x0").as_int64();
                start_point.y = road.as_object().at("y0").as_int64();

                if (road.as_object().contains("x1"))
                {
                    auto x1 = road.as_object().at("x1").as_int64();
                    model::Road model_road(model::Road::HORIZONTAL, start_point, x1);
                    model_map.AddRoad(model_road);

                }
                if (road.as_object().contains("y1"))
                {
                    auto y1 = road.as_object().at("y1").as_int64();
                    model::Road model_road(model::Road::VERTICAL, start_point, y1);
                    model_map.AddRoad(model_road);
                }
            }

            auto buildings = map.at("buildings").as_array();
            for (auto building : buildings)
            {
                model::Point position;
                position.x = building.as_object().at("x").as_int64();
                position.y = building.as_object().at("y").as_int64();
 
                model::Size size;
                size.height = building.as_object().at("h").as_int64();
                size.width = building.as_object().at("w").as_int64();

                model::Building model_building(model::Rectangle(position, size));
                model_map.AddBuilding(model_building);
            }

            auto offices = map.at("offices").as_array();
            for (auto office : offices)
            {
                std::string id(office.at("id").as_string());

                model::Point position;
                position.x = office.as_object().at("x").as_int64();
                position.y = office.as_object().at("y").as_int64();

                model::Offset offset;
                offset.dx = office.as_object().at("offsetX").as_int64();
                offset.dy = office.as_object().at("offsetY").as_int64();

                model::Office model_office(model::Office::Id(id), position, offset);
                model_map.AddOffice(model_office);
            }

            game.AddMap(model_map);
        }

        return game;
    }

} // namespace json_loader

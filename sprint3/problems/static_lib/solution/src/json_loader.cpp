#include "json_loader.h"

#include <fstream>
#include <iostream>
#include <string>

// // Этот заголовочный файл надо подключить в одном и только одном .cpp-файле программы
// #include <boost/json/src.hpp>


using namespace std::literals;

namespace extra_data
{

} // namespace extra_data

namespace json_loader
{
    double getRandomNumberFrom0To1()
    {
        // Initialize random number generator
        std::random_device rd;
        std::mt19937 gen(rd());

        // Initialize uniform distribution
        std::uniform_real_distribution<> dis(0.0, 1.0);

        return dis(gen);
    }

    std::string ReadTextFile(const std::filesystem::path &json_path)
    {
        // Check if the file exists
        if (!std::filesystem::exists(json_path))
        {
            std::cerr << "File does not exist!\n";
            throw JsonLoaderException("Json file does not exist");
        }

        // Open the file
        std::ifstream file(json_path);

        // Check if the file was opened successfully
        if (!file.is_open())
        {
            std::cerr << "Unable to open file!\n";
            throw JsonLoaderException("Unable to open json file!");
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
        // lootGeneratorConfig
        auto loot_gen_config = value.as_object().at("lootGeneratorConfig").as_object();
        double probability = loot_gen_config.at("probability").as_double();
        double period = loot_gen_config.at("period").as_double();
        loot_gen::LootGenerator::TimeInterval interval(static_cast<int64_t>(period * 1000));

        loot_gen::LootGenerator loot_gen(interval, probability, getRandomNumberFrom0To1); // todo - добавить генератор

        model::Game game(loot_gen);

        if (value.as_object().contains("defaultDogSpeed"))
        {
            double defaultDogSpeed = value.as_object().at("defaultDogSpeed").as_double();
            game.SetDfaultDogSpeed(defaultDogSpeed);
        }

        auto maps = value.as_object().at(JsonStrConst::maps).as_array();

        for (const auto &map : maps)
        {
            game.AddMap(ParseMap(map, game.GetDefDogSpeed()));
        }

        return game;
    }

    model::Road ParseRoad(boost::json::value js_road)
    {
        model::Point start_point;
        start_point.x = js_road.as_object().at(JsonStrConst::x0).as_int64();
        start_point.y = js_road.as_object().at(JsonStrConst::y0).as_int64();

        if (js_road.as_object().contains(JsonStrConst::x1))
        {
            auto x1 = js_road.as_object().at(JsonStrConst::x1).as_int64();
            model::Road model_road(model::Road::HORIZONTAL, start_point, x1);
            return model_road;
        }
        else
        {
            auto y1 = js_road.as_object().at(JsonStrConst::y1).as_int64();
            model::Road model_road(model::Road::VERTICAL, start_point, y1);
            return model_road;
        }
    }

    model::Building ParseBuilding(boost::json::value building)
    {
        model::Point position;
        position.x = building.as_object().at(JsonStrConst::x).as_int64();
        position.y = building.as_object().at(JsonStrConst::y).as_int64();

        model::Size size;
        size.height = building.as_object().at(JsonStrConst::h).as_int64();
        size.width = building.as_object().at(JsonStrConst::w).as_int64();

        model::Building model_building(model::Rectangle(position, size));
        return model_building;
    }

    model::Office ParseOffice(boost::json::value office)
    {
        std::string id(office.at(JsonStrConst::id).as_string());

        model::Point position;
        position.x = office.as_object().at(JsonStrConst::x).as_int64();
        position.y = office.as_object().at(JsonStrConst::y).as_int64();

        model::Offset offset;
        offset.dx = office.as_object().at(JsonStrConst::dx).as_int64();
        offset.dy = office.as_object().at(JsonStrConst::dy).as_int64();

        model::Office model_office(model::Office::Id(id), position, offset);
        return model_office;
    }

    model::Map ParseMap(boost::json::value map, double def_dog_speed)
    {
        std::string id(map.at(JsonStrConst::id).as_string());
        std::string name(map.at(JsonStrConst::name).as_string());

        auto loot_types = map.at("lootTypes").as_array();
        int loot_types_size = loot_types.size();

        model::Map model_map(model::Map::Id(id), name, def_dog_speed, loot_types_size);

        if (map.as_object().contains("dogSpeed"))
        {
            double dog_speed(map.at("dogSpeed").as_double());
            model_map.SetDogSpeed(dog_speed);
        }

        auto roads = map.at(JsonStrConst::roads).as_array();
        for (auto road : roads)
        {
            model_map.AddRoad(ParseRoad(road));
        }

        auto buildings = map.at(JsonStrConst::buildings).as_array();
        for (auto building : buildings)
        {
            model_map.AddBuilding(ParseBuilding(building));
        }

        auto offices = map.at(JsonStrConst::offices).as_array();
        for (auto office : offices)
        {
            model_map.AddOffice(ParseOffice(office));
        }
        return model_map;
    }

    extra_data::ExtraData LoadExtraData(const std::filesystem::path &json_path)
    {
        auto content = ReadTextFile(json_path);
        auto value = json::parse(content);
        auto maps = value.as_object().at(JsonStrConst::maps).as_array();
        extra_data::ExtraData ext_data;

        for (auto map : maps)
        {
            std::string id(map.at(JsonStrConst::id).as_string());
            auto loot_types = map.at("lootTypes").as_array();
            ext_data.SetLootType(id, loot_types);
        }
        return ext_data;
    }

}
// namespace json_loader

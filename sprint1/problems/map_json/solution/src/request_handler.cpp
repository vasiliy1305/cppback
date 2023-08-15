#include "request_handler.h"
#include <boost/json.hpp>

#include <algorithm>
#include <iterator>

namespace http_handler
{

    StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                      bool keep_alive,
                                      std::string_view content_type)
    {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);

        return response;
    }

    Request ParsePath(const std::string &path)
    {
        Request result;
        std::stringstream ss(path);
        std::string token;

        while (std::getline(ss, token, '/'))
        {
            if (!token.empty())
            {
                result.parts.push_back(token);
            }
        }

        if (path.size() == 0)
        {
            result.type = RequestType::Error;
            return result;
        }
        else if (path == "/api/v1/maps"s)
        {
            result.type = RequestType::Maps;
            return result;
        }
        else if (result.parts.size() == 4 && result.parts.at(0) == "api" && result.parts.at(1) == "v1" && result.parts.at(2) == "maps")
        {
            result.type = RequestType::Map;
            return result;
        }
        else if (result.parts.size() != 0 && result.parts.at(0) == "api")
        {
            result.type = RequestType::BadRequest;
            return result;
        }
        else
        {
            result.type = RequestType::Error;
            return result;
        }
    }

    std::string RequestHandler::GetMapsAsJS()
    {
        boost::json::array arr;
        for (const auto &map : game_.GetMaps())
        {
            boost::json::object obj;
            obj["id"] = *map.GetId();
            obj["name"] = map.GetName();
            arr.push_back(obj);
        }
        return boost::json::serialize(arr);
    }

    bool RequestHandler::IsMapExist(std::string id)
    {
        const auto &maps = game_.GetMaps();
        auto map_it = std::find_if(maps.begin(), maps.end(), [&](model::Map map)
                                   { return *map.GetId() == id; });
        return map_it != maps.end();
    }

    std::string RequestHandler::GetMapAsJS(std::string id)
    {
        const auto &maps = game_.GetMaps();
        auto map_it = std::find_if(maps.begin(), maps.end(), [&](model::Map map)
                                   { return *map.GetId() == id; });

        if (map_it == maps.end())
        {
            return "";
        }
        auto map = *map_it;

        boost::json::object map_obj;
        map_obj["id"] = *map.GetId();
        map_obj["name"] = map.GetName();

        // Добавление дорог
        boost::json::array roads_obj;
        for (const auto &road : map.GetRoads())
        {
            if (road.IsHorizontal())
            {
                roads_obj.push_back({{"x0", road.GetStart().x}, {"y0", road.GetStart().y}, {"x1", road.GetEnd().x}});
            }
            else
            {
                roads_obj.push_back({{"x0", road.GetStart().x}, {"y0", road.GetStart().y}, {"y1", road.GetEnd().y}});
            }
        }
        map_obj["roads"] = roads_obj;

        // Добавление зданий
        boost::json::array buildings;
        for (const auto &building : map.GetBuildings())
        {
            buildings.push_back({{"x", building.GetBounds().position.x}, {"y", building.GetBounds().position.y}, {"w", building.GetBounds().size.width}, {"h", building.GetBounds().size.height}});
        }
        map_obj["buildings"] = buildings;

        // Добавление офисов
        boost::json::array offices;
        for (const auto &office : map.GetOffices())
        {
            offices.push_back({{"id", *office.GetId()}, {"x", office.GetPosition().x}, {"y", office.GetPosition().y}, {"offsetX", office.GetOffset().dx}, {"offsetY", office.GetOffset().dy}});
        }
        map_obj["offices"] = offices;

        return boost::json::serialize(map_obj);
    }

} // namespace http_handler

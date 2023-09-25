#include "application.h"

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version, bool keep_alive, std::string_view content_type,
                                  std::vector<std::pair<http::field, std::string>> http_fields)
{
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    // response.set(http::field::cache_control, "no-cache");
    for (auto field : http_fields)
    {
        response.set(field.first, field.second);
    }

    return response;
}

std::vector<std::string> SplitRequest(const std::string &str_req)
{
    std::vector<std::string> result;
    std::stringstream ss(str_req);
    std::string token;

    while (std::getline(ss, token, '/')) // сделать констаной /
    {
        if (!token.empty())
        {
            result.push_back(token);
        }
    }
    return result;
}

namespace app
{
    StringResponse Application::ReturnMethodNotAllowed(const StringRequest &req, std::string_view text, std::string allow)
    {
        return MakeStringResponse(http::status::method_not_allowed,
                                  text, req.version(),
                                  req.keep_alive(),
                                  ContentType::API_JSON,
                                  {{http::field::cache_control, "no-cache"}, {http::field::allow, allow}});
    }

    StringResponse Application::ReturnJsonContent(const StringRequest &req, http::status status, std::string_view text)
    {
        return MakeStringResponse(status, text, req.version(), req.keep_alive(), ContentType::API_JSON, {{http::field::cache_control, "no-cache"}});
    }

    StringResponse Application::GetMaps(const StringRequest &req)
    {
        if (req.method() == http::verb::get) // возможно добавить head
        {
            return ReturnJsonContent(req, http::status::ok, GetMapsAsJS());
        }
        return ReturnMethodNotAllowed(req, MakeMessege("invalidMethod", "Only GET method is expected"), "GET");
    }

    std::string Application::GetMapsAsJS()
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

    bool Application::IsMapExist(std::string id)
    {
        const auto &maps = game_.GetMaps();
        auto map_it = std::find_if(maps.begin(), maps.end(), [&](model::Map map)
                                   { return *map.GetId() == id; });
        return map_it != maps.end();
    }

    std::string Application::GetMapAsJS(std::string id)
    {
        const auto &maps = game_.GetMaps();
        auto map_it = std::find_if(maps.begin(), maps.end(), [&](model::Map map)
                                   { return *map.GetId() == id; });

        if (map_it == maps.end())
        {
            return "";
        }
        auto map = *map_it;

        return boost::json::serialize(MapToJsonObj(map));
    }

    boost::json::value Application::RoadToJsonObj(const model::Road &road)
    {
        if (road.IsHorizontal())
        {
            return {{"x0", road.GetStart().x}, {"y0", road.GetStart().y}, {"x1", road.GetEnd().x}};
        }
        return {{"x0", road.GetStart().x}, {"y0", road.GetStart().y}, {"y1", road.GetEnd().y}};
    }

    boost::json::value Application::BuildingToJsonObj(const model::Building &building)
    {
        return {{"x", building.GetBounds().position.x}, {"y", building.GetBounds().position.y}, {"w", building.GetBounds().size.width}, {"h", building.GetBounds().size.height}};
    }

    boost::json::value Application::OfficeToJsonObj(const model::Office &office)
    {
        return {{"id", *office.GetId()}, {"x", office.GetPosition().x}, {"y", office.GetPosition().y}, {"offsetX", office.GetOffset().dx}, {"offsetY", office.GetOffset().dy}};
    }

    boost::json::value Application::MapToJsonObj(const model::Map &map)
    {
        boost::json::object map_obj;
        map_obj["id"] = *map.GetId();
        map_obj["name"] = map.GetName();

        // Добавление дорог
        boost::json::array roads_obj;
        for (const auto &road : map.GetRoads())
        {
            roads_obj.push_back(RoadToJsonObj(road));
        }
        map_obj["roads"] = roads_obj;

        // Добавление зданий
        boost::json::array buildings;
        for (const auto &building : map.GetBuildings())
        {
            buildings.push_back(BuildingToJsonObj(building));
        }
        map_obj["buildings"] = buildings;

        // Добавление офисов
        boost::json::array offices;
        for (const auto &office : map.GetOffices())
        {
            offices.push_back(OfficeToJsonObj(office));
        }

        // добавить lootTypes
        auto loot_types = extra_data_.GetLootType(*map.GetId());
        map_obj["lootTypes"] = *loot_types;

        map_obj["offices"] = offices;
        return map_obj;
    }

    StringResponse Application::GetMap(const StringRequest &req)
    {
        if (req.method() == http::verb::get || req.method() == http::verb::head)
        {
            auto target_bsv = req.target();
            std::string target_str(target_bsv.begin(), target_bsv.end());
            auto request_parts = SplitRequest(target_str);

            if (IsMapExist(request_parts.at(3)))
            {
                return ReturnJsonContent(req, http::status::ok, GetMapAsJS(request_parts.at(3)));
            }
            return ReturnJsonContent(req, http::status::not_found, MakeMessege("mapNotFound", "Map not found"));
        }
        return ReturnMethodNotAllowed(req, MakeMessege("invalidMethod", "Only GET, HEAD method is expected"), "GET, HEAD");
    }

    StringResponse Application::JoinGame(const StringRequest &req)
    {
        if (req.method() == http::verb::post)
        {
            auto [body, status] = Join(req.body().c_str());
            return ReturnJsonContent(req, status, body);
        }
        return ReturnMethodNotAllowed(req, MakeMessege("invalidMethod", "Only POST method is expected"), "POST");
    }

    std::pair<std::string, http::status> Application::Join(const std::string json_str)
    {
        http::status status;
        std::string body;
        try
        {
            auto value = json::parse(json_str);
            std::string user_name(value.as_object()["userName"].as_string());
            std::string map_id(value.as_object()["mapId"].as_string());

            auto [player, token] = game_.Join(user_name, map_id);

            if (user_name == "")
            {
                status = http::status::bad_request;
                body = MakeMessege("invalidArgument", "Invalid name");
                return {body, status};
            }

            if (player)
            {
                boost::json::object resualt;
                resualt["authToken"] = *token;
                resualt["playerId"] = *(player->GetId());
                status = http::status::ok;
                body = boost::json::serialize(resualt);
            }
            else
            {
                status = http::status::not_found;
                body = MakeMessege("mapNotFound", "Map not found");
            }
            return {body, status};
        }
        catch (...)
        {
            status = http::status::bad_request;
            body = MakeMessege("invalidArgument", "Join game request parse error");
            return {body, status};
        }
    }

    std::pair<std::string, http::status> Application::Players(const std::string token)
    {

        http::status status;
        std::string body;
        if (token.size() == 32)
        {
            auto dogs = game_.GetDogsByToken(token);
            if (dogs.size()) // todo сделать нормально
            {

                boost::json::object js_players;
                for (auto dog : dogs)
                {
                    auto id = dog->GetId();
                    js_players[std::to_string(*id)] = DogToJsonObj(*dog);
                }
                body = json::serialize(js_players);
                status = http::status::ok;
                return {body, status};
            }
            status = http::status::unauthorized;
            body = MakeMessege("unknownToken", "Player token has not been found");
            return {body, status};
        }
        status = http::status::unauthorized;
        body = MakeMessege("invalidToken", "Authorization header is missing");
        return {body, status};
    }

    std::pair<std::string, http::status> Application::State(const std::string token)
    {
        http::status status;
        std::string body;
        if (token.size() == 32)
        {
            auto dogs = game_.GetDogsByToken(token);
            if (dogs.size()) // todo!!!
            {
                // std::dogs->size()
                boost::json::object js_players;
                for (auto dog : dogs)
                {
                    auto id = dog->GetId();
                    js_players[std::to_string(*id)] = DogToJsonObj(*dog);
                }
                boost::json::object resualt;
                resualt["players"] = js_players;

                const auto lost_loots = game_.GetLootsByToken(token);
                boost::json::object js_lost_loots;
                int i = 0;
                for (const auto loot : lost_loots)
                {
                    js_lost_loots[std::to_string(i)] = LootToJsonObj(loot);
                }
                resualt["lostObjects"] = js_lost_loots;

                body = json::serialize(resualt);
                status = http::status::ok;
                return {body, status};
            }
            status = http::status::unauthorized;
            body = MakeMessege("unknownToken", "Player token has not been found");
            return {body, status};
        }
        status = http::status::unauthorized;
        body = MakeMessege("invalidToken", "Authorization header is missing");
        return {body, status};
    }

    boost::json::value Application::DogToJsonObj(model::Dog &dog)
    {
        boost::json::array pos = {dog.GetPos().x, dog.GetPos().y};
        boost::json::array speed = {dog.GetSpeed().x, dog.GetSpeed().y};
        boost::json::string dir(dog.GetDir());

        boost::json::object player;
        player["pos"] = pos;
        player["speed"] = speed;
        player["dir"] = dir;

        return player;
    }

    boost::json::value Application::LootToJsonObj(const model::Loot &loot)
    {
        const boost::json::array pos = {loot.GetPos().x, loot.GetPos().y};
        boost::json::object loot_js;
        loot_js["pos"] = pos;
        loot_js["type"] = loot.GetType();
        return loot_js;
    }

    StringResponse Application::GetState(const StringRequest &req)
    {
        if (req.method() == http::verb::get || req.method() == http::verb::head)
        {
            auto [body, status] = State(GetToken(req));
            return ReturnJsonContent(req, status, body);
        }
        else
        {
            return ReturnMethodNotAllowed(req, MakeMessege("invalidMethod", "Only GET method is expected"), "GET, HEAD");
        }
    }

    StringResponse Application::GetPlayers(const StringRequest &req)
    {
        if (req.method() == http::verb::get || req.method() == http::verb::head)
        {

            auto [body, status] = Players(GetToken(req));

            return ReturnJsonContent(req, status, body);
        }
        return ReturnMethodNotAllowed(req, MakeMessege("invalidMethod", "Only GET and HEAD method is expected"), "GET, HEAD");
    }

    std::string Application::GetToken(const StringRequest &req)
    {
        auto auth = req[boost::beast::http::field::authorization];
        std::string auth_str(auth.begin(), auth.end());
        std::istringstream iss(auth_str);
        std::string trash, token;
        iss >> trash >> token;
        if (token.size() != 32)
        {
            return "";
        }
        return token;
    }

    StringResponse Application::SetPlayerAction(const StringRequest &req)
    {
        if (req.method() == http::verb::post)
        {
            auto player = game_.GetPlayerByToken(GetToken(req));
            if (player != nullptr)
            {
                auto value = json::parse(req.body().c_str());
                std::string direction(value.as_object().at("move").as_string());
                player->GetDog()->SetDir(direction);
                return ReturnJsonContent(req, http::status::ok, "{}");
            }
            return ReturnJsonContent(req, http::status::unauthorized, MakeMessege("invalidToken", "Authorization header is missing"));
        }
        return ReturnMethodNotAllowed(req, MakeMessege("invalidMethod", "Only POST method is expected"), "POST");
    }

    StringResponse Application::SetTimeDelta(const StringRequest &req)
    {
        if (req.method() == http::verb::post)
        {
            try
            {
                auto value = json::parse(req.body().c_str());
                int64_t time_delta(value.as_object().at("timeDelta").as_int64());
                UpdateTime(time_delta);
                return ReturnJsonContent(req, http::status::ok, "{}");
            }
            catch (const std::exception &e)
            {
                return ReturnJsonContent(req, http::status::bad_request, MakeMessege("invalidArgument", "Failed to parse tick request JSON"));
            }
        }
        return ReturnMethodNotAllowed(req, MakeMessege("invalidMethod", "Only POST method is expected"), "POST");
    }

    void Application::UpdateTime(int delta_time)
    {
        game_.UpdateTime(delta_time);
    }

    std::string Application::MakeMessege(std::string code, std::string message)
    {
        json::object msg;
        msg["code"] = code;
        msg["message"] = message;

        return json::serialize(msg);
    }
} // end namespace app
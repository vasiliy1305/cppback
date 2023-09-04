#include "request_handler.h"

#include <algorithm>
#include <iterator>

namespace http_handler
{

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

    FileResponse MakeFileResponse(http::status status,
                                  fs::path file_path,
                                  unsigned http_version,
                                  bool keep_alive)
    {
        http::file_body::value_type file;
        if (sys::error_code ec; file.open(file_path.c_str(), beast::file_mode::read, ec), ec)
        {
            // todo добавить ошибку?
            FileResponse response(http::status::not_found, http_version);
            response.keep_alive(keep_alive);
            response.set(http::field::content_type, ContentType::TEXT_PLAIN);
            response.prepare_payload();
            return response;
        }
        else
        {
            FileResponse response(status, http_version);
            response.keep_alive(keep_alive);
            response.body() = std::move(file);
            auto file_name = file_path.filename();
            auto file_ext = getFileExtension(file_name);
            auto content_type = FileExtensionToContentType(file_ext);
            response.set(http::field::content_type, content_type);
            response.prepare_payload();
            return response;
        }
    }

    fs::path buildPath(const fs::path &base, const std::vector<std::string> &dirs)
    {
        std::filesystem::path result = base;
        for (const auto &dir : dirs)
        {
            result /= dir;
        }
        return result;
    }

    bool IsSubPath(fs::path path, fs::path base)
    {
        // Приводим оба пути к каноничному виду (без . и ..)
        path = fs::weakly_canonical(path);
        base = fs::weakly_canonical(base);

        // Проверяем, что все компоненты base содержатся внутри path
        for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p)
        {
            if (p == path.end() || *p != *b)
            {
                return false;
            }
        }
        return true;
    }

    std::string getFileExtension(const std::string &filename)
    {
        size_t pos = filename.rfind('.');
        if (pos == std::string::npos)
        {
            // No extension found
            return "";
        }
        std::string ext = filename.substr(pos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }

    std::string_view FileExtensionToContentType(const std::string &file_extension)
    {
        if (file_extension == "txt")
        {
            return ContentType::TEXT_PLAIN;
        }
        else if (file_extension == "htm" || file_extension == "html")
        {
            return ContentType::TEXT_HTML;
        }
        else if (file_extension == "css")
        {
            return ContentType::TEXT_CSS;
        }
        else if (file_extension == "js")
        {
            return ContentType::TEXT_JS;
        }
        else if (file_extension == "json")
        {
            return ContentType::API_JSON;
        }
        else if (file_extension == "xml")
        {
            return ContentType::API_XML;
        }
        else if (file_extension == "png")
        {
            return ContentType::IMAGE_PNG;
        }
        else if (file_extension == "jpg" || file_extension == "jpe" || file_extension == "jpeg")
        {
            return ContentType::IMAGE_JPG;
        }
        else if (file_extension == "gif")
        {
            return ContentType::IMAGE_GIF;
        }
        else if (file_extension == "bmp")
        {
            return ContentType::IMAGE_BMP;
        }
        else if (file_extension == "ico")
        {
            return ContentType::IMAGE_ICO;
        }
        else if (file_extension == "tiff" || file_extension == "tif")
        {
            return ContentType::IMAGE_TIFF;
        }
        else if (file_extension == "svg" || file_extension == "svgz")
        {
            return ContentType::IMAGE_SVG;
        }
        else if (file_extension == "mp3")
        {
            return ContentType::AUDIO_MPEG;
        }
        else
        {
            return ContentType::API_OCT;
        }
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

    ApiRequestType GetApiReqType(const std::string &path)
    {
        auto request_parts = SplitRequest(path);
        if (path == "/api/v1/maps"s)
        {
            return ApiRequestType::MAPS;
        }
        else if (path == "/api/v1/game/join"s)
        {
            return ApiRequestType::JOIN;
        }
        else if (path == "/api/v1/game/players"s)
        {
            return ApiRequestType::PLAYERS;
        }
        else if (path == "/api/v1/game/state"s)
        {
            return ApiRequestType::STATE;
        }
        else if ((request_parts.size() == 4) && (request_parts.at(0) == "api") && (request_parts.at(1) == "v1") && (request_parts.at(2) == "maps"))
        {
            return ApiRequestType::MAP;
        }
        else
        {
            return ApiRequestType::BADREQUEST;
        }
    }

    std::string ApiHandler::GetMapsAsJS()
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

    bool ApiHandler::IsMapExist(std::string id)
    {
        const auto &maps = game_.GetMaps();
        auto map_it = std::find_if(maps.begin(), maps.end(), [&](model::Map map)
                                   { return *map.GetId() == id; });
        return map_it != maps.end();
    }

    std::string ApiHandler::GetMapAsJS(std::string id)
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

    boost::json::value ApiHandler::RoadToJsonObj(const model::Road &road)
    {
        if (road.IsHorizontal())
        {
            return {{"x0", road.GetStart().x}, {"y0", road.GetStart().y}, {"x1", road.GetEnd().x}};
        }
        else
        {
            return {{"x0", road.GetStart().x}, {"y0", road.GetStart().y}, {"y1", road.GetEnd().y}};
        }
    }

    boost::json::value ApiHandler::BuildingToJsonObj(const model::Building &building)
    {
        return {{"x", building.GetBounds().position.x}, {"y", building.GetBounds().position.y}, {"w", building.GetBounds().size.width}, {"h", building.GetBounds().size.height}};
    }

    boost::json::value ApiHandler::OfficeToJsonObj(const model::Office &office)
    {
        return {{"id", *office.GetId()}, {"x", office.GetPosition().x}, {"y", office.GetPosition().y}, {"offsetX", office.GetOffset().dx}, {"offsetY", office.GetOffset().dy}};
    }

    boost::json::value ApiHandler::MapToJsonObj(const model::Map &map)
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
        map_obj["offices"] = offices;
        return map_obj;
    }

    std::pair<std::string, http::status> ApiHandler::Join(const std::string json_str)
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
                body = "{\"code\": \"invalidArgument\", \"message\": \"Invalid name\"}";
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
                body = "{\"code\": \"mapNotFound\", \"message\": \"Map not found\"}";
            }
            return {body, status};
        }
        catch (...)
        {
            status = http::status::bad_request;
            body = "{\"code\": \"invalidArgument\", \"message\": \"Join game request parse error\"}";
            return {body, status};
        }
    }

    std::pair<std::string, http::status> ApiHandler::Players(const std::string token)
    {
        // std::cerr << "Players {" << std::endl;
        // std::cerr << "    token = " << token << std::endl;
        // std::cerr << "    token.size() = " << token.size() << std::endl;
        http::status status;
        std::string body;
        if (token.size() == 32)
        {
            auto dogs = game_.GetPlayersByToken(token);
            if (dogs)
            {
                // std::cerr << "dogs !=0" << std::endl;
                boost::json::object js_players;
                for (auto dog : *dogs)
                {
                    auto id = dog.GetId();
                    js_players[std::to_string(*id)] = DogToJsonObj(dog);
                }
                body = json::serialize(js_players);
                status = http::status::ok;
                return {body, status};
            }
            else
            {
                // std::cerr << "dogs ==0" << std::endl;
                status = http::status::unauthorized;
                body = "{\"code\": \"unknownToken\", \"message\": \"Player token has not been found\"}";
                return {body, status};
            }
        }
        else
        {
            status = http::status::unauthorized;
            body = "{\"code\": \"invalidToken\", \"message\": \"Authorization header is missing\"}";
            return {body, status};
        }
        // std::cerr << "} Players" << std::endl;
    }

        std::pair<std::string, http::status> ApiHandler::State(const std::string token)
    {
        http::status status;
        std::string body;
        if (token.size() == 32)
        {
            auto dogs = game_.GetPlayersByToken(token);
            if (dogs)
            {
                boost::json::object js_players;
                for (auto dog : *dogs)
                {
                    auto id = dog.GetId();
                    js_players[std::to_string(*id)] = DogToJsonObj(dog);
                }
                body = json::serialize(js_players);
                status = http::status::ok;
                return {body, status};
            }
            else
            {
                status = http::status::unauthorized;
                body = "{\"code\": \"unknownToken\", \"message\": \"Player token has not been found\"}";
                return {body, status};
            }
        }
        else
        {
            status = http::status::unauthorized;
            body = "{\"code\": \"invalidToken\", \"message\": \"Authorization header is missing\"}";
            return {body, status};
        }
    }

    bool RequestHandler::IsApi(const std::string &request)
    {
        return request.substr(0, 4) == "/api";
    }

    boost::json::value ApiHandler::DogToJsonObj(model::Dog &dog)
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

} // namespace http_handler

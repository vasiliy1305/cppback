#include "request_handler.h"

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

    FileResponse MakeFileResponse(http::status status,
                                  fs::path file_path,
                                  unsigned http_version,
                                  bool keep_alive)
    {
        http::file_body::value_type file;
        if (sys::error_code ec; file.open(file_path.c_str(), beast::file_mode::read, ec), ec)
        {
            std::cout << "faleed to open file" << std::endl;
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

    Request ParsePath(const std::string &path)
    {
        Request result;
        std::stringstream ss(path);
        std::string token;

        while (std::getline(ss, token, '/')) // сделать констаной /
        {
            if (!token.empty())
            {
                result.parts.push_back(token);
            }
        }

        if (path.size() == 0)
        {
            result.type = RequestType::Index;
            return result;
        }
        else if (path == "/api/v1/maps"s)
        {
            result.type = RequestType::Maps;
            return result;
        }
        else if ((result.parts.size() == 4) && (result.parts.at(0) == "api") && (result.parts.at(1) == "v1") && result.parts.at(2) == "maps")
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
            result.type = RequestType::Static;
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

        return boost::json::serialize(MapToJsonObj(map));
    }

    boost::json::value RequestHandler::RoadToJsonObj(const model::Road &road)
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

    boost::json::value RequestHandler::BuildingToJsonObj(const model::Building &building)
    {
        return {{"x", building.GetBounds().position.x}, {"y", building.GetBounds().position.y}, {"w", building.GetBounds().size.width}, {"h", building.GetBounds().size.height}};
    }

    boost::json::value RequestHandler::OfficeToJsonObj(const model::Office &office)
    {
        return {{"id", *office.GetId()}, {"x", office.GetPosition().x}, {"y", office.GetPosition().y}, {"offsetX", office.GetOffset().dx}, {"offsetY", office.GetOffset().dy}};
    }

    boost::json::value RequestHandler::MapToJsonObj(const model::Map &map)
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

} // namespace http_handler


#pragma once

#include "model.h"
#include "http_server.h"
#include <iostream>
#include <string_view>
#include <vector>
#include <boost/json.hpp>

using namespace std::literals;
namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(file, "File", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(line, "Line", int)

namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;
namespace sys = boost::system;
namespace net = boost::asio;
namespace json = boost::json;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status,
                                  std::string_view body,
                                  unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type,
                                  std::vector<std::pair<http::field, std::string>> http_fields);

struct ContentType
{
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view TEXT_CSS = "text/css"sv;
    constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
    constexpr static std::string_view TEXT_JS = "text/javascript"sv;
    constexpr static std::string_view API_JSON = "application/json"sv;
    constexpr static std::string_view API_XML = "application/xml"sv;
    constexpr static std::string_view API_OCT = "application/octet-stream"sv;
    constexpr static std::string_view IMAGE_PNG = "image/png"sv;
    constexpr static std::string_view IMAGE_JPG = "image/jpeg"sv;
    constexpr static std::string_view IMAGE_GIF = "image/gif"sv;
    constexpr static std::string_view IMAGE_BMP = "image/bmp"sv;
    constexpr static std::string_view IMAGE_ICO = "image/vnd.microsoft.icon"sv;
    constexpr static std::string_view IMAGE_TIFF = "image/tiff"sv;
    constexpr static std::string_view IMAGE_SVG = "image/svg+xml"sv;
    constexpr static std::string_view AUDIO_MPEG = "audio/mpeg"sv;
    // При необходимости внутрь ContentType можно добавить и другие типы контента
};

std::vector<std::string> SplitRequest(const std::string &str_req);
namespace app
{

    class Application
    {
    public:
        Application(model::Game &game) : game_{game}
        {
        }

        StringResponse GetMaps(const StringRequest &req);
        StringResponse GetMap(const StringRequest &req);
        StringResponse GetPlayers(const StringRequest &req);
        StringResponse GetState(const StringRequest &req);
        StringResponse SetPlayerAction(const StringRequest &req);
        StringResponse JoinGame(const StringRequest &req);
        StringResponse SetTimeDelta(const StringRequest &req);

        void UpdateTime(int delta_time);

    private:
        model::Game game_;

        std::string GetToken(const StringRequest &req);

        std::string MakeMessege(std::string code, std::string message);

        StringResponse ReturnMethodNotAllowed(const StringRequest &req, std::string_view text, std::string allow);
        StringResponse ReturnJsonContent(const StringRequest &req, http::status status, std::string_view text);

        // json
        bool IsMapExist(std::string id);
        std::string GetMapsAsJS();
        std::string GetMapAsJS(std::string id);
        boost::json::value RoadToJsonObj(const model::Road &road);
        boost::json::value BuildingToJsonObj(const model::Building &building);
        boost::json::value OfficeToJsonObj(const model::Office &office);
        boost::json::value MapToJsonObj(const model::Map &map);
        boost::json::value DogToJsonObj(model::Dog &dog);

        std::pair<std::string, http::status> Players(const std::string token);
        std::pair<std::string, http::status> State(const std::string token);
        std::pair<std::string, http::status> Join(const std::string json_str);
    };

} // end namespace app

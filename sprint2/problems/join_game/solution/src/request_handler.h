#pragma once
#include "http_server.h"
#include "model.h"

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

namespace http_handler
{
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace fs = std::filesystem;
    namespace sys = boost::system;
    namespace net = boost::asio;
    namespace json = boost::json;

    using namespace std::literals;

    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;
    // Ответ, тело которого представлено в виде файла
    using FileResponse = http::response<http::file_body>;

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

    // возвращает расширение в нижнем регистре
    std::string getFileExtension(const std::string &filename);

    // file extension to content type
    std::string_view FileExtensionToContentType(const std::string &file_extension);

    std::vector<std::string> SplitRequest(const std::string &str_req);

    // Создаёт StringResponse с заданными параметрами
    StringResponse MakeStringResponse(http::status status,
                                      std::string_view body,
                                      unsigned http_version,
                                      bool keep_alive,
                                      std::string_view content_type,
                                      std::vector<std::pair<http::field, std::string>> http_fields);

    // Создаёт FileResponse с заданными параметрами
    FileResponse MakeFileResponse(http::status status,
                                  fs::path file_path,
                                  unsigned http_version,
                                  bool keep_alive);

    fs::path buildPath(const fs::path &base, const std::vector<std::string> &dirs); // delete this func

    // Возвращает true, если каталог p содержится внутри base_path.
    bool IsSubPath(fs::path path, fs::path base);

    class ContentHandler
    {
    public:
        ContentHandler(fs::path static_dir) : static_dir_(static_dir)
        {
        }

        template <typename Body, typename Allocator, typename Send>
        void Do(http::request<Body, http::basic_fields<Allocator>> &req, Send &send)
        {
            auto target_bsv = req.target();
            std::string target_str(target_bsv.begin(), target_bsv.end());
            auto request_parts = SplitRequest(target_str);

            auto target_file_path = buildPath(static_dir_, request_parts);
            // проверить что таргет не корневая директория
            if (target_file_path == static_dir_)
            {
                target_file_path = buildPath(static_dir_, {"index.html"}); // поменять на index
            }

            // проверить что файл находится в поддиректории статик
            if (IsSubPath(target_file_path, static_dir_))
            {
                // проверить что файл существует
                if (fs::exists(target_file_path))
                {
                    send(MakeFileResponse(http::status::ok, target_file_path, req.version(), req.keep_alive()));
                }
                else
                {
                    // todo возможно сообщение об ошибке
                    send(MakeStringResponse(http::status::not_found, "wrong page", req.version(), req.keep_alive(), ContentType::TEXT_PLAIN, {{http::field::cache_control, "no-cache"}}));
                }
            }
            else
            {
                // todo возможно сообщение об ошибке
                send(MakeStringResponse(http::status::bad_request, "xxx", req.version(), req.keep_alive(), ContentType::TEXT_PLAIN, {{http::field::cache_control, "no-cache"}}));
            }
        }

    private:
        fs::path static_dir_;
    };

    enum class ApiRequestType
    {
        MAP,
        MAPS,
        JOIN,
        PLAYERS,
        BADREQUEST
    };

    ApiRequestType GetApiReqType(const std::string &path);

    class ApiHandler
    {
    public:
        ApiHandler(model::Game &game) : game_{game}
        {
        }

        template <typename Body, typename Allocator, typename Send>
        void Do(http::request<Body, http::basic_fields<Allocator>> &req, Send &send)
        {

            const auto json_response = [&req](http::status status, std::string_view text)
            {
                return MakeStringResponse(status, text, req.version(), req.keep_alive(), ContentType::API_JSON, {{http::field::cache_control, "no-cache"}});
            };

            const auto method_not_allowed_response = [&req](std::string_view text, std::string allow)
            {
                return MakeStringResponse(http::status::method_not_allowed, text, req.version(), req.keep_alive(), ContentType::API_JSON, {{http::field::cache_control, "no-cache"}, {http::field::allow, allow}});
            };

            auto target_bsv = req.target();
            std::string target_str(target_bsv.begin(), target_bsv.end());
            auto request_type = GetApiReqType(target_str);
            auto request_parts = SplitRequest(target_str);

            if (request_type == ApiRequestType::MAPS)
            {
                if (req.method() == http::verb::get)
                {
                    send(json_response(http::status::ok, GetMapsAsJS()));
                }
                else
                {
                    send(json_response(http::status::method_not_allowed, ""sv));
                }
            }
            else if (request_type == ApiRequestType::MAP)
            {
                if (req.method() == http::verb::get)
                {
                    if (IsMapExist(request_parts.at(3)))
                    {
                        send(json_response(http::status::ok, GetMapAsJS(request_parts.at(3))));
                    }
                    else
                    {
                        send(json_response(http::status::not_found, "{\"code\": \"mapNotFound\",\"message\": \"Map not found\"}"));
                    }
                }
                else
                {
                    send(json_response(http::status::method_not_allowed, ""sv));
                }
            }
            else if (request_type == ApiRequestType::PLAYERS)
            {
                if (req.method() == http::verb::get || req.method() == http::verb::head)
                {
                    auto auth = req[boost::beast::http::field::authorization];
                    std::string auth_str(auth.begin(), auth.end());
                    std::istringstream iss(auth_str);
                    std::string trash, token;
                    iss >> trash >> token;
                    auto [body, status] = Players(token);
                    send(json_response(status, body));
                }
                else
                {
                    send(json_response(http::status::method_not_allowed, ""sv));
                }
            }
            else if (request_type == ApiRequestType::BADREQUEST)
            {
                send(json_response(http::status::bad_request, "{\"code\": \"badRequest\", \"message\": \"Bad request\"}"));
            }
            else if (request_type == ApiRequestType::JOIN)
            {
                if (req.method() == http::verb::post)
                {
                    auto [body, status] = Join(req.body().c_str());
                    send(json_response(status, body));
                }
                else
                {
                    send(method_not_allowed_response("{\"code\": \"invalidMethod\", \"message\": \"Only POST method is expected\"}" , "POST"));
                }
            }
        }

    private:
        model::Game &game_;

        bool IsMapExist(std::string id);
        std::string GetMapsAsJS();
        std::string GetMapAsJS(std::string id);
        boost::json::value RoadToJsonObj(const model::Road &road);
        boost::json::value BuildingToJsonObj(const model::Building &building);
        boost::json::value OfficeToJsonObj(const model::Office &office);
        boost::json::value MapToJsonObj(const model::Map &map);
        boost::json::value DogToJsonObj(const model::Dog &dog);
        std::pair<std::string, http::status> Join(const std::string json_str);
        std::pair<std::string, http::status> Players(const std::string token);
    };

    //
    class RequestHandler
    {
    public:
        explicit RequestHandler(model::Game &game, fs::path static_dir) : api_handler_{game}, content_handler_(static_dir)
        {
        }

        RequestHandler(const RequestHandler &) = delete;
        RequestHandler &operator=(const RequestHandler &) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>> &&req, Send &&send)
        {
            const auto text_response = [&req](http::status status, std::string_view text)
            {
                return MakeStringResponse(status, text, req.version(), req.keep_alive(), ContentType::API_JSON, {{http::field::cache_control, "no-cache"}});
            };

            auto target_bsv = req.target();
            std::string target_str(target_bsv.begin(), target_bsv.end());

            if (IsApi(target_str))
            {
                api_handler_.Do(req, send);
            }
            else
            {
                content_handler_.Do(req, send);
            }
        }

    private:
        ContentHandler content_handler_;
        ApiHandler api_handler_;

        bool IsApi(const std::string &request);
    };

} // namespace http_handler

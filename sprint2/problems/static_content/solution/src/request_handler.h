#pragma once
#include "http_server.h"
#include "model.h"

#include <iostream>
#include <string_view>
#include <vector>
#include <boost/json.hpp>

namespace http_handler
{
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace fs = std::filesystem;
    namespace sys = boost::system;

    using namespace std::literals;

    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;
    // Ответ, тело которого представлено в виде файла
    using FileResponse = http::response<http::file_body>;

    enum class RequestType
    {
        Error,
        Maps,
        Map,
        BadRequest,
        Static,
        Index
    };

    struct Request
    {
        RequestType type;
        std::vector<std::string> parts;
    };

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

    // разбор строки API
    Request ParsePath(const std::string &path);

    // Создаёт StringResponse с заданными параметрами
    StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                      bool keep_alive,
                                      std::string_view content_type = ContentType::TEXT_HTML);

    FileResponse MakeFileResponse(http::status status,
                                  fs::path file_path,
                                  unsigned http_version,
                                  bool keep_alive);

    fs::path buildPath(const fs::path &base, const std::vector<std::string> &dirs);

    // Возвращает true, если каталог p содержится внутри base_path.
    bool IsSubPath(fs::path path, fs::path base);

    class RequestHandler
    {
    public:
        explicit RequestHandler(model::Game &game, fs::path static_dir) : game_{game}, static_dir_(static_dir)
        {
        }

        RequestHandler(const RequestHandler &) = delete;
        RequestHandler &operator=(const RequestHandler &) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>> &&req, Send &&send)
        {
            const auto text_response = [&req](http::status status, std::string_view text)
            {
                return MakeStringResponse(status, text, req.version(), req.keep_alive(), ContentType::API_JSON);
            };

            if (req.method() == http::verb::get)
            {
                auto target_bsv = req.target();
                std::string target_str(target_bsv.begin(), target_bsv.end());
                auto request = ParsePath(target_str);

                if (request.type == RequestType::Maps)
                {
                    send(text_response(http::status::ok, GetMapsAsJS()));
                }
                else if (request.type == RequestType::Map)
                {
                    if (IsMapExist(request.parts.at(3)))
                    {
                        send(text_response(http::status::ok, GetMapAsJS(request.parts.at(3))));
                    }
                    else
                    {
                        send(text_response(http::status::not_found, "{\"code\": \"mapNotFound\",\"message\": \"Map not found\"}"));
                    }
                }
                else if (request.type == RequestType::BadRequest)
                {
                    send(text_response(http::status::bad_request, "{\"code\": \"badRequest\", \"message\": \"Bad request\"}"));
                }
                else if (request.type == RequestType::Static)
                {
                    auto target_file_path = buildPath(static_dir_, request.parts);
                    // проверить что файл находится в поддиректории статик
                    if (IsSubPath(target_file_path, static_dir_))
                    {
                        if (target_file_path == static_dir_)
                        {
                            target_file_path = buildPath(static_dir_, {"index.html"});
                            send(MakeFileResponse(http::status::ok, target_file_path, req.version(), req.keep_alive()));
                        }
                        else
                        {
                            send(MakeFileResponse(http::status::ok, target_file_path, req.version(), req.keep_alive()));
                        }
                    }
                }
                else if (request.type == RequestType::Index)
                {
                    auto target_file_path = buildPath(static_dir_, {"index.html"});
                    // проверить что файл находится в поддиректории статик
                    std::cout << "index" << std::endl;
                    if (IsSubPath(target_file_path, static_dir_))
                    {
                        send(MakeFileResponse(http::status::ok, target_file_path, req.version(), req.keep_alive()));
                    }
                }
            }

            else if (req.method() == http::verb::head)
            {
                auto target_bsv = req.target();
                std::string target_str(target_bsv.begin(), target_bsv.end());
                auto request = ParsePath(target_str);

                if (request.type == RequestType::Index)
                {
                    auto target_file_path = buildPath(static_dir_, {"index.html"});
                    // проверить что файл находится в поддиректории статик
                    if (IsSubPath(target_file_path, static_dir_))
                    {
                        send(MakeFileResponse(http::status::ok, target_file_path, req.version(), req.keep_alive()));
                    }
                }
            }
            else
            {
                send(text_response(http::status::method_not_allowed, ""sv));
            }
        }

    private:
        model::Game &game_;
        fs::path static_dir_;

        std::string GetMapsAsJS();
        boost::json::value RoadToJsonObj(const model::Road &road);
        boost::json::value BuildingToJsonObj(const model::Building &building);
        boost::json::value OfficeToJsonObj(const model::Office &office);
        boost::json::value MapToJsonObj(const model::Map &map);
        bool IsMapExist(std::string id);
        std::string GetMapAsJS(std::string id);
    };

} // namespace http_handler

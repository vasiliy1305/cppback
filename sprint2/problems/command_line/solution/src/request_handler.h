#pragma once

#include "application.h"

namespace http_handler
{
    using namespace std::literals;

    // Ответ, тело которого представлено в виде файла
    using FileResponse = http::response<http::file_body>;

    // возвращает расширение в нижнем регистре
    std::string getFileExtension(const std::string &filename);

    // file extension to content type
    std::string_view FileExtensionToContentType(const std::string &file_extension);

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
                    send(MakeStringResponse(http::status::not_found, "wrong page", req.version(), req.keep_alive(), ContentType::TEXT_PLAIN, {{http::field::cache_control, "no-cache"}}));
                }
            }
            else
            {
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
        STATE,
        MOVE,
        BADREQUEST,
        TICK
    };

    ApiRequestType GetApiReqType(const std::string &path);

    class ApiHandler
    {
    public:
        ApiHandler(model::Game &game, net::io_context &ioc) : app_{game} , ioc_(ioc)
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
                send(app_.GetMaps(req));
            }
            else if (request_type == ApiRequestType::MAP)
            {
                send(app_.GetMap(req));
            }
            else if (request_type == ApiRequestType::PLAYERS)
            {
                send(app_.GetPlayers(req));
            }
            else if (request_type == ApiRequestType::STATE)
            {
                send(app_.GetState(req));
            }
            else if (request_type == ApiRequestType::BADREQUEST)
            {
                send(json_response(http::status::bad_request, "{\"code\": \"badRequest\", \"message\": \"Bad request\"}"));
            }
            else if (request_type == ApiRequestType::JOIN)
            {
                send(app_.JoinGame(req));
            }
            else if (request_type == ApiRequestType::MOVE)
            {
                send(app_.SetPlayerAction(req));
            }
            else if (request_type == ApiRequestType::TICK)
            {
                send(app_.SetTimeDelta(req));
            }
        }

    private:
        app::Application app_;
        net::io_context &ioc_;
    };


    class RequestHandler
    {
    public:
        explicit RequestHandler(model::Game &game, fs::path static_dir, int tick, net::io_context &ioc) : api_handler_{game, ioc},
                                                                                                     content_handler_(static_dir),
                                                                                                     tick_(tick)
                                                                                                
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
        int tick_;

        bool IsApi(const std::string &request);
    };

} // namespace http_handler

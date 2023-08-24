#pragma once
#include "http_server.h"
#include "model.h"

#include <iostream>
#include <string_view>
#include <vector>
<<<<<<< HEAD
=======
#include <boost/json.hpp>
>>>>>>> review_final_1

namespace http_handler
{
    namespace beast = boost::beast;
    namespace http = beast::http;

    using namespace std::literals;

    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;

    enum class RequestType
    {
        Error,
        Maps,
        Map,
        BadRequest
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
        constexpr static std::string_view API_JSON = "application/json"sv;
        // При необходимости внутрь ContentType можно добавить и другие типы контента
    };

    // разбор строки API
    Request ParsePath(const std::string &path);

    // Создаёт StringResponse с заданными параметрами
    StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                      bool keep_alive,
                                      std::string_view content_type = ContentType::TEXT_HTML);

    class RequestHandler
    {
    public:
        explicit RequestHandler(model::Game &game) : game_{game}
        {
        }

        RequestHandler(const RequestHandler &) = delete;
        RequestHandler &operator=(const RequestHandler &) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>> &&req, Send &&send)
        {
            // Обработать запрос request и отправить ответ, используя send

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
                else if(request.type == RequestType::BadRequest)
                {
                    send(text_response(http::status::bad_request, "{\"code\": \"badRequest\", \"message\": \"Bad request\"}"));
                }
            }

            else if (req.method() == http::verb::head)
            {
                // return text_response(http::status::ok, ""sv);
            }
            else
            {
                send(text_response(http::status::method_not_allowed, ""sv));
            }
        }

    private:
        model::Game &game_;

        std::string GetMapsAsJS();
<<<<<<< HEAD
=======
        boost::json::value RoadToJsonObj(const model::Road &road);
        boost::json::value BuildingToJsonObj(const model::Building &building);
        boost::json::value OfficeToJsonObj(const model::Office &office);
        boost::json::value MapToJsonObj(const model::Map &map);
>>>>>>> review_final_1
        bool IsMapExist(std::string id);
        std::string GetMapAsJS(std::string id);
    };

} // namespace http_handler

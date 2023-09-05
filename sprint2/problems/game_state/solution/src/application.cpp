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

namespace app
{
    StringResponse Application::ReturnMethodNotAllowed(const StringRequest &req, std::string_view text, std::string allow)
    {
        return MakeStringResponse(http::status::method_not_allowed, text, req.version(), req.keep_alive(), ContentType::API_JSON, {{http::field::cache_control, "no-cache"}, {http::field::allow, allow}});
    }

    StringResponse Application::GetMaps(const StringRequest &req)
    {
        if (req.method() == http::verb::get) // возможно добавить head
        {
            // send(json_response(http::status::ok, GetMapsAsJS()));
        }
        else
        {
            // send(json_response(http::status::method_not_allowed, ""sv));
            return ReturnMethodNotAllowed(req, "{\"code\": \"invalidMethod\", \"message\": \"Only GET method is expected\"}" , "GET");
        }
    }

} // end namespace app
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
        else
        {
            return ReturnMethodNotAllowed(req, "{\"code\": \"invalidMethod\", \"message\": \"Only GET method is expected\"}", "GET");
        }
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

} // end namespace app
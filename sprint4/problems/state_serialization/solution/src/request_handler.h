#pragma once

#include "application.h"

namespace http_handler
{
    using Clock = std::chrono::steady_clock;
    
    class Ticker : public std::enable_shared_from_this<Ticker>
    {
    public:
        using Strand = net::strand<net::io_context::executor_type>;
        using Handler = std::function<void(std::chrono::milliseconds delta)>;

        // Функция handler будет вызываться внутри strand с интервалом period
        Ticker(Strand strand, std::chrono::milliseconds period, Handler handler)
            : strand_{strand}, period_{period}, handler_{std::move(handler)}
        {
        }

        void Start()
        {
            net::dispatch(strand_, [self = shared_from_this()]
                          {
            self->SetLastTick(Clock::now());
            self->ScheduleTick(); });
        }
        void SetLastTick(std::chrono::steady_clock::time_point last_tick)
        {
            last_tick_ = last_tick;
        }

    private:
        void ScheduleTick()
        {
            // assert(strand_.running_in_this_thread());
            timer_.expires_after(period_);
            timer_.async_wait([self = shared_from_this()](sys::error_code ec)
                              { self->OnTick(ec); });
        }

        void OnTick(sys::error_code ec)
        {
            using namespace std::chrono;
            // assert(strand_.running_in_this_thread());

            if (!ec)
            {
                auto this_tick = Clock::now();
                auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
                last_tick_ = this_tick;
                try
                {
                    handler_(delta);
                }
                catch (...)
                {
                }
                ScheduleTick();
            }
        }

        

        Strand strand_;
        std::chrono::milliseconds period_;
        net::steady_timer timer_{strand_};
        Handler handler_;
        std::chrono::steady_clock::time_point last_tick_;
    };

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
        ApiHandler(model::Game &game, net::io_context &ioc, int period, extra_data::ExtraData &extra_data, std::string state_file, bool save_periodical, int save_period) : app_{game, extra_data, state_file, save_periodical, save_period}, ioc_(ioc), period_(period)
        {
            if (period)
            {
                StartTicker(); // todo 
            }
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
                if (period_ < 0)
                {
                    send(app_.SetTimeDelta(req));
                }
                else
                {
                    send(json_response(http::status::bad_request, "{\"code\": \"badRequest\", \"message\": \"Bad request\"}"));
                }
            }
        }

        void StartTicker()
        {
            // strand, используемый для доступа к API
            auto api_strand = net::make_strand(ioc_);

            // Настраиваем вызов метода Application::Tick каждые 50 миллисекунд внутри strand
            auto ticker = std::make_shared<Ticker>(api_strand, 50ms,
                                                   [this](std::chrono::milliseconds delta)
                                                   { app_.UpdateTime(delta.count()); });
            ticker->Start();
        }

    private:
        app::Application app_;
        net::io_context &ioc_;
        std::shared_ptr<Ticker> tiker_sh_ptr_;
        int period_;
    };

    class RequestHandler
    {
    public:
        explicit RequestHandler(model::Game &game, fs::path static_dir, int period, net::io_context &ioc, extra_data::ExtraData &extra_data, std::string state_file, bool save_periodical, int save_period) : api_handler_{game, ioc, period, extra_data, state_file, save_periodical, save_period},
                                                                                                            content_handler_(static_dir)

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

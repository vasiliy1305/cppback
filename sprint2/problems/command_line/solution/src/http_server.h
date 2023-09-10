#pragma once
#include "sdk.h"
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <filesystem>
#include <iostream>
#include <chrono>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL
#include <boost/log/core.hpp>        // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/json.hpp>
#include <cmath>
namespace http_server
{
    namespace net = boost::asio;
    using tcp = net::ip::tcp;
    namespace beast = boost::beast;
    namespace http = beast::http;

    namespace keywords = boost::log::keywords;
    namespace sinks = boost::log::sinks;
    namespace logging = boost::log;
    namespace sys = boost::system;

    using namespace std::literals;

    void LogJson(std::string msg, boost::json::object data);
    void LogServerStarted(net::ip::address addres, net::ip::port_type port);
    void LogRequestReceived(std::string IP, std::string URI, std::string method);
    void LogResponseSent(int response_time, int code, std::string content_type);
    void MyFormatter(logging::record_view const &rec, logging::formatting_ostream &strm);
    void LogServerExited(int code, std::string exception);
    void LogError(int code, std::string text, std::string where);
    void InitBoostLog();

    void ReportError(beast::error_code ec, std::string_view what);

    class SessionBase
    {
    public:
        // Запрещаем копирование и присваивание объектов SessionBase и его наследников
        SessionBase(const SessionBase &) = delete;
        SessionBase &operator=(const SessionBase &) = delete;
        void Run();

    protected:
        using HttpRequest = http::request<http::string_body>;
        std::chrono::_V2::system_clock::time_point start_read_;

        virtual ~SessionBase() = default;

        explicit SessionBase(tcp::socket &&socket)
            : stream_(std::move(socket))
        {
        }

        template <typename Body, typename Fields>
        void Write(http::response<Body, Fields> &&response)
        {

            auto content_type_bsv = response.base().at("Content-Type");
            std::string content_type_str(content_type_bsv.begin(), content_type_bsv.end());

            auto finish = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = finish - start_read_;
            auto elapsed_int  = static_cast<int>(std::round(elapsed.count()));

            LogResponseSent(elapsed_int, response.result_int(), content_type_str);
            // Запись выполняется асинхронно, поэтому response перемещаем в область кучи
            auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(response));

            auto self = GetSharedThis();
            http::async_write(stream_, *safe_response,
                              [safe_response, self](beast::error_code ec, std::size_t bytes_written)
                              {
                                  self->OnWrite(safe_response->need_eof(), ec, bytes_written);
                              });
        }

    private:
        // tcp_stream содержит внутри себя сокет и добавляет поддержку таймаутов
        beast::tcp_stream stream_;
        beast::flat_buffer buffer_;
        HttpRequest request_;

        void Read();
        void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read);
        void Close();
        void OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written);

        // Обработку запроса делегируем подклассу
        virtual void HandleRequest(HttpRequest &&request) = 0;
        virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;
    };

    template <typename RequestHandler>
    class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>>
    {
    public:
        template <typename Handler>
        Session(tcp::socket &&socket, Handler &&request_handler)
            : SessionBase(std::move(socket)), request_handler_(std::forward<Handler>(request_handler))
        {
        }

    private:
        RequestHandler request_handler_;

        std::shared_ptr<SessionBase> GetSharedThis() override
        {
            return this->shared_from_this();
        }

        void HandleRequest(HttpRequest &&request) override
        {
            // Захватываем умный указатель на текущий объект Session в лямбде,
            // чтобы продлить время жизни сессии до вызова лямбды.
            // Используется generic-лямбда функция, способная принять response произвольного типа
            request_handler_(std::move(request), [self = this->shared_from_this()](auto &&response)
                             { self->Write(std::move(response)); });
        }
    };

    template <typename RequestHandler>
    class Listener : public std::enable_shared_from_this<Listener<RequestHandler>>
    {
    public:
        template <typename Handler>
        Listener(net::io_context &ioc, const tcp::endpoint &endpoint, Handler &&request_handler)
            : ioc_(ioc)
              // Обработчики асинхронных операций acceptor_ будут вызываться в своём strand
              ,
              acceptor_(net::make_strand(ioc)), request_handler_(std::forward<Handler>(request_handler))
        {
            // Открываем acceptor, используя протокол (IPv4 или IPv6), указанный в endpoint
            acceptor_.open(endpoint.protocol());

            // После закрытия TCP-соединения сокет некоторое время может считаться занятым,
            // чтобы компьютеры могли обменяться завершающими пакетами данных.
            // Однако это может помешать повторно открыть сокет в полузакрытом состоянии.
            // Флаг reuse_address разрешает открыть сокет, когда он "наполовину закрыт"
            acceptor_.set_option(net::socket_base::reuse_address(true));
            // Привязываем acceptor к адресу и порту endpoint
            acceptor_.bind(endpoint);
            // Переводим acceptor в состояние, в котором он способен принимать новые соединения
            // Благодаря этому новые подключения будут помещаться в очередь ожидающих соединений
            acceptor_.listen(net::socket_base::max_listen_connections);
        }

        void Run()
        {
            DoAccept();
        }

    private:
        net::io_context &ioc_;
        tcp::acceptor acceptor_;
        RequestHandler request_handler_;

        void DoAccept()
        {
            acceptor_.async_accept(
                // Передаём последовательный исполнитель, в котором будут вызываться обработчики
                // асинхронных операций сокета
                net::make_strand(ioc_),
                // С помощью bind_front_handler создаём обработчик, привязанный к методу OnAccept
                // текущего объекта.
                // Так как Listener — шаблонный класс, нужно подсказать компилятору, что
                // shared_from_this — метод класса, а не свободная функция.
                // Для этого вызываем его, используя this
                // Этот вызов bind_front_handler аналогичен
                // namespace ph = std::placeholders;
                // std::bind(&Listener::OnAccept, this->shared_from_this(), ph::_1, ph::_2)
                beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
        }

        // Метод socket::async_accept создаст сокет и передаст его передан в OnAccept
        void OnAccept(sys::error_code ec, tcp::socket socket)
        {
            using namespace std::literals;

            if (ec)
            {
                LogError(ec.value(), ec.message(), "accept");
                return ReportError(ec, "accept"sv);
            }

            // Асинхронно обрабатываем сессию
            AsyncRunSession(std::move(socket));

            // Принимаем новое соединение
            DoAccept();
        }

        void AsyncRunSession(tcp::socket &&socket)
        {
            std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_)->Run();
        }
    };

    template <typename RequestHandler>
    void ServeHttp(net::io_context &ioc, const tcp::endpoint &endpoint, RequestHandler &&handler)
    {
        // При помощи decay_t исключим ссылки из типа RequestHandler,
        // чтобы Listener хранил RequestHandler по значению
        using MyListener = Listener<std::decay_t<RequestHandler>>;

        std::make_shared<MyListener>(ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
    }

} // namespace http_server
// 123

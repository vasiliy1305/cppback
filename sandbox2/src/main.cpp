#include <math.h>
#include <vector>
#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <iomanip>
#include <syncstream>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>

namespace net = boost::asio;
using namespace std::literals;
namespace sys = boost::system;


// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

// Ядро асинхронного HTTP-сервера будет располагаться в пространстве имён http_server
namespace http_server {

namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace beast = boost::beast;
namespace sys = boost::system;

template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
public:
    template <typename Handler>
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& request_handler)
        : ioc_(ioc)
        // Обработчики асинхронных операций acceptor_ будут вызываться в своём strand
        , acceptor_(net::make_strand(ioc))
        , request_handler_(std::forward<Handler>(request_handler)) {
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

private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    RequestHandler request_handler_;
};

}  // namespace http_server

// // Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn &fn)
{
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n)
    {
        workers.emplace_back(fn);
    }
    fn();
}

int main()
{
    const unsigned num_threads = std::thread::hardware_concurrency();

    std::cout << "num_threads = " << num_threads << std::endl;

    net::io_context ioc(num_threads);

    // Подписываемся на сигналы и при их получении завершаем работу сервера
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](const sys::error_code &ec, [[maybe_unused]] int signal_number)
                       {
        if (!ec) {
            std::cout << "Signal "sv << signal_number << " received"sv << std::endl;
            ioc.stop();
        } });

    net::steady_timer t{ioc, 30s};
    t.async_wait([](sys::error_code ec)
                 { std::cout << "Timer expired"s << std::endl; });

    RunWorkers(num_threads, [&ioc]
               { ioc.run(); });
    std::cout << "Shutting down"sv << std::endl;
}
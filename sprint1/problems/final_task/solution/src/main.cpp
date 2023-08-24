#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <thread>
#include <boost/asio/signal_set.hpp>

#include "json_loader.h"
#include "request_handler.h"

#include "tests.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

namespace
{
    // Запускает функцию fn на n потоках, включая текущий
    template <typename Fn>
<<<<<<< HEAD
    void RunWorkers(unsigned n, const Fn &fn)
    {
        n = std::max(1u, n);
        std::vector<std::jthread> workers;
        workers.reserve(n - 1);
        // Запускаем n-1 рабочих потоков, выполняющих функцию fn
        while (--n)
=======
    void RunWorkers(unsigned thread_number, const Fn &fn)
    {
        thread_number = std::max(1u, thread_number);
        std::vector<std::jthread> workers;
        workers.reserve(thread_number - 1);
        // Запускаем n-1 рабочих потоков, выполняющих функцию fn
        while (--thread_number)
>>>>>>> review_final_1
        {
            workers.emplace_back(fn);
        }
        fn();
    }
} // namespace

// int main()
// {
//     AllTests();
// }

int main(int argc, const char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: game_server <game-config-json>"sv << std::endl;
        return EXIT_FAILURE;
    }
    try
    {
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(argv[1]);

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        // После своего старта сервер должен реагировать на сигналы SIGINT и SIGTERM и корректно завершать свою работу при получении этих сигналов.
        // Подписываемся на сигналы и при их получении завершаем работу сервера
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code &ec, [[maybe_unused]] int signal_number)
                           {
        if (!ec) 
        {
            ioc.stop();
        } });

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        http_handler::RequestHandler handler{game};

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, {address, port}, [&](auto &&req, auto &&send)
                               { handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send)); });

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        std::cout << "Server has started..."sv << std::endl;

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc]
                   { ioc.run(); });
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
<<<<<<< HEAD
=======
    return 0;
>>>>>>> review_final_1
}

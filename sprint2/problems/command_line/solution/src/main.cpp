#include "sdk.h"

#include <boost/asio/io_context.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
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
    void RunWorkers(unsigned thread_number, const Fn &fn)
    {
        thread_number = std::max(1u, thread_number);
        std::vector<std::jthread> workers;
        workers.reserve(thread_number - 1);
        // Запускаем n-1 рабочих потоков, выполняющих функцию fn
        while (--thread_number)
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

struct Args
{
    int tick_period = -1;
    std::string config_file;
    std::string www_root;
    bool randomize_spawn_points = false;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char *const argv[])
{
    namespace po = boost::program_options;

    po::options_description desc{"Allowed options"s};

    Args args;
    desc.add_options()("help,h", "produce help message")("tick-period,t", po::value(&args.tick_period)->value_name("milliseconds"s), "set tick period")("config-file,c", po::value(&args.config_file)->value_name("file"s), "set config file path")("www-root,w", po::value(&args.www_root)->value_name("dir"s), "set static files root")("randomize-spawn-points", "spawn dogs at random positions");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s))
    {
        // Если был указан параметр --help, то выводим справку и возвращаем nullopt
        std::cout << desc;
        return std::nullopt;
    }
    if (!vm.contains("config-file"s))
    {
        throw std::runtime_error("config-file have not been specified"s);
    }
    if (!vm.contains("www-root"s))
    {
        throw std::runtime_error("www-root dir is not specified"s);
    }
    if (!vm.contains("tick-period"s))
    {
        args.tick_period = -1;
    }
    if (vm.contains("randomize-spawn-points"))
    {
        args.randomize_spawn_points = true;
    }
    return args;
}

int main(int argc, const char *argv[])
{
    try
    {

        if (auto args = ParseCommandLine(argc, argv))
        {

            // 0. init log
            http_server::InitBoostLog();

            // 1. Загружаем карту из файла и построить модель игры
            model::Game game = json_loader::LoadGame(args->config_file);
            game.SetRandomizeSpawnPoints(args->randomize_spawn_points);

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
            http_handler::RequestHandler handler{game, args->www_root, args->tick_period, ioc};
            // http_handler::LoggingRequestHandler logging_handler{handler};

            // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
            const auto address = net::ip::make_address("0.0.0.0");
            constexpr net::ip::port_type port = 8080;
            http_server::ServeHttp(ioc, {address, port}, [&](auto &&req, auto &&send)
                                   { handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send)); });

            // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
            // std::cout << "Server has started..." << std::endl;
            http_server::LogServerStarted(address, port);

            // 6. Запускаем обработку асинхронных операций
            RunWorkers(std::max(1u, num_threads), [&ioc]
                       { ioc.run(); });
        }
    }
    catch (const std::exception &ex)
    {
        http_server::LogServerExited(EXIT_FAILURE, ex.what());
        return EXIT_FAILURE;
    }
    http_server::LogServerExited(0, "");
    return 0;
}
// 123

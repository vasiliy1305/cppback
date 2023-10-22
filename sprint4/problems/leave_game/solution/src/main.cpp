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

struct Args
{
    int tick_period = -1;
    int save_state_period = -1;
    std::string config_file;
    std::string www_root;
    std::string state_file;
    bool randomize_spawn_points = false;
    bool use_state = false;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char *const argv[])
{
    namespace po = boost::program_options;

    po::options_description desc{"Allowed options"s};

    Args args;
    desc.add_options()("help,h", "produce help message")("tick-period,t", po::value(&args.tick_period)->value_name("milliseconds"s), "set tick period")("save-state-period", po::value(&args.save_state_period)->value_name("milliseconds"s), "set save state period")("config-file,c", po::value(&args.config_file)->value_name("file"s), "set config file path")("state-file", po::value(&args.state_file)->value_name("file"s), "set state file path")("www-root,w", po::value(&args.www_root)->value_name("dir"s), "set static files root")("randomize-spawn-points", "spawn dogs at random positions");

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
    if (!vm.contains("save-state-period"s))
    {
        args.save_state_period = -1;
    }
    if (vm.contains("randomize-spawn-points"))
    {
        args.randomize_spawn_points = true;
    }
    if (vm.contains("state-file"))
    {
        args.use_state = true;
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
            logger::InitBoostLog();

            // 1. Загружаем карту из файла и построить модель игры либо из созраненого состояния

            model::Game game = json_loader::LoadGame(args->config_file);
            extra_data::ExtraData extra_data = json_loader::LoadExtraData(args->config_file);
            game.SetRandomizeSpawnPoints(args->randomize_spawn_points);

            if (args->use_state)
            {
                LoadGameFromFile(args->state_file, game);

                // todo add lootgen
            }

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

                const char* db_url = std::getenv("GAME_DB_URL");
                if (!db_url) {
                    throw std::runtime_error("GAME_DB_URL is not specified");
                }

                ConnectionPool pool{12, [db_url] {
                                     auto conn = std::make_shared<pqxx::connection>(db_url);
                                     return conn;
                                 }};
            // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры

            http_handler::RequestHandler handler{game, args->www_root, 
            args->tick_period, ioc, 
            extra_data, args->state_file, 
            (args->save_state_period > 0), 
            args->save_state_period
            ,pool };

            // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
            const auto address = net::ip::make_address("0.0.0.0");
            constexpr net::ip::port_type port = 8080;
            http_server::ServeHttp(ioc, {address, port}, [&](auto &&req, auto &&send)
                                   { handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send)); });

            // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
            // std::cout << "Server has started..." << std::endl;
            logger::LogServerStarted(address, port);

            // 6. Запускаем обработку асинхронных операций
            RunWorkers(std::max(1u, num_threads), [&ioc]
                       { ioc.run(); });

            // В этой точке все асинхронные операции уже завершены и можно
            // сохранить состояние сервера в файл
            // <-----------------------------------
            std::cerr << " exit " << std::endl;
            if (args->use_state)
            {
                SaveGameToFile(args->state_file, game);
            }
        }
    }
    catch (const std::exception &ex)
    {
        logger::LogServerExited(EXIT_FAILURE, ex.what());
        return EXIT_FAILURE;
    }
    logger::LogServerExited(0, "");
    return 0;
}
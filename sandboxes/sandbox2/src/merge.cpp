#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <optional>
#include <vector>

using namespace std::literals;

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

    if (vm.contains("randomize-spawn-points"))
    {
        args.randomize_spawn_points = true;
    }
    return args;
}

int main(int argc, char *argv[])
{
    try
    {
        if (auto args = ParseCommandLine(argc, argv))
        {
            std::cerr << "config_file = " << args->config_file << std::endl
                      << "www_root = " << args->www_root << std::endl
                      << "randomize_spawn_points = " << args->randomize_spawn_points << std::endl
                      << "tick_period = " << args->tick_period << std::endl;
        }
        return EXIT_SUCCESS;
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
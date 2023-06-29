#ifdef WIN32
#include <sdkddkver.h>
#endif

#include "seabattle.h"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <string_view>

namespace net = boost::asio;
using net::ip::tcp;
using namespace std::literals;

void PrintFieldPair(const SeabattleField &left, const SeabattleField &right)
{
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i)
    {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket &socket)
{
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec)
    {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket &socket, std::string_view data)
{
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()), ec);

    return !ec;
}

class SeabattleAgent
{
public:
    SeabattleAgent(const SeabattleField &field)
        : my_field_(field)
    {
    }

    void StartGame(tcp::socket &socket, bool my_initiative)
    {
        // TODO: реализуйте самостоятельно
        while (!IsGameEnded())
        {
            PrintFields();
            if (my_initiative)
            {
                // получить кординаты из вв
                std::cout << "введите координаты" << std::endl;
                std::string coords_str;
                std::cin >> coords_str;
                auto coords = ParseMove(coords_str);
                // стрельнуть через сеть
                SendMove(socket, coords);
                // получить результат выстрела
                auto resualt = ReadResult(socket);
                // отмечаем на чужой карте
                if (resualt == SeabattleField::ShotResult::HIT)
                {
                    other_field_.MarkHit(coords.value().first, coords.value().second);
                }
                else if (resualt == SeabattleField::ShotResult::KILL)
                {
                    other_field_.MarkKill(coords.value().first, coords.value().second);
                }
                else
                {
                    other_field_.MarkMiss(coords.value().first, coords.value().second);
                    my_initiative = false;
                }
                // если подбил или ранел повторяем
            }
            else
            {
                // ожидаем встрел
                auto coords = ReadMove(socket);
                // применяем ко своему полю
                auto resualt = my_field_.Shoot(coords.value().first, coords.value().second);
                // отправлям результат
                SendResult(socket, resualt);
                // если расчитываем my iniative
                if (resualt == SeabattleField::ShotResult::MISS)
                {
                    my_initiative = true;
                }
            }
        }
    }

private:
    static std::optional<std::pair<int, int>> ParseMove(const std::string_view &sv)
    {
        std::cout << sv << std::endl;
        std::cout << sv.size() << std::endl;
        if (sv.size() != 2)
            return std::nullopt;

        int p1 = sv[0] - 'A', p2 = sv[1] - '1';

        if (p1 < 0 || p1 > 8)
            return std::nullopt;
        if (p2 < 0 || p2 > 8)
            return std::nullopt;

        return {{p1, p2}};
    }

    static std::string MoveToString(std::pair<int, int> move)
    {
        char buff[] = {static_cast<char>(move.first) + 'A', static_cast<char>(move.second) + '1'};
        return {buff, 2};
    }

    void PrintFields() const
    {
        PrintFieldPair(my_field_, other_field_);
    }

    bool IsGameEnded() const
    {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    // TODO: добавьте методы по вашему желанию
    std::optional<std::pair<int, int>> ReadMove(tcp::socket &socket)
    {
        std::cout << "ReadMove begin - " << std::endl;
        net::streambuf stream_buf;
        boost::system::error_code ec;
        net::read_until(socket, stream_buf, '\n', ec); // CHECK: возможно нужно отправлять \n в методе SendMove
        std::string client_data{std::istreambuf_iterator<char>(&stream_buf), std::istreambuf_iterator<char>()};
        std::cout << "ReadMove - " << client_data << std::endl;

        if (ec)
        {
            std::cout << "ReadMove Error - " << ec.what() << std::endl;
            // return;
        }
        std::cout << "ReadMove end - " << std::endl;
        return ParseMove(client_data.substr(0, 2));
    }

    SeabattleField::ShotResult ReadResult(tcp::socket &socket)
    {
        std::cout << "ReadResult 0" << std::endl;
        net::streambuf stream_buf;
        std::cout << "ReadResult 1" << std::endl;
        boost::system::error_code ec;
        std::cout << "ReadResult 2" << std::endl;
        net::read_until(socket, stream_buf, '\n', ec);
        std::cout << "ReadResult 3" << std::endl;
        std::string client_data{std::istreambuf_iterator<char>(&stream_buf), std::istreambuf_iterator<char>()};
        client_data = client_data.substr(0, 1);

        if (ec)
        {
            std::cout << "Error rReadResult - "sv << ec.what() << std::endl;
        }
        if (client_data == "0")
        {
            return SeabattleField::ShotResult::HIT;
        }
        else if (client_data == "1")
        {
            return SeabattleField::ShotResult::KILL;
        }
        else
        {
            return SeabattleField::ShotResult::MISS;
        }
        std::cout << "ReadResult 4" << std::endl;
    }

    void SendMove(tcp::socket &socket, std::optional<std::pair<int, int>> coords)
    {
        std::cout << "SendMove begin" << std::endl;
        boost::system::error_code ec;
        std::string coords_str = MoveToString(coords.value());
        socket.write_some(net::buffer(coords_str + "\n"s), ec);
        if (ec)
        {
            std::cout << "SendMove Error - " << ec.what() << std::endl;
        }
        std::cout << "SendMove end" << std::endl;
    }

    void SendResult(tcp::socket &socket, SeabattleField::ShotResult resualt)
    {
        std::cout << "SendResult be=gin" << std::endl;
        boost::system::error_code ec;
        if (resualt == SeabattleField::ShotResult::HIT)
        {
            socket.write_some(net::buffer("0\n"s), ec);
        }
        if (resualt == SeabattleField::ShotResult::KILL)
        {
            socket.write_some(net::buffer("1\n"s), ec);
        }
        if (resualt == SeabattleField::ShotResult::MISS)
        {
            socket.write_some(net::buffer("2\n"s), ec);
        }
        if (ec)
        {
            std::cout << "SendResult Error - " << ec.what() << std::endl;
        }
        std::cout << "SendResult end" << std::endl;
    }

private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField &field, unsigned short port)
{
    SeabattleAgent agent(field);

    // TODO: реализуйте самостоятельно
    net::io_context io_context;

    // используем конструктор tcp::v4 по умолчанию для адреса 0.0.0.0
    tcp::acceptor acceptor(io_context, tcp::endpoint(net::ip::make_address("127.0.0.1"), port));
    std::cout << "Waiting for connection..."sv << std::endl;

    boost::system::error_code ec;
    tcp::socket socket{io_context};
    acceptor.accept(socket, ec);

    if (ec)
    {
        std::cout << "Can't accept connection"sv << std::endl;
        return;
    }

    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField &field, const std::string &ip_str, unsigned short port)
{
    SeabattleAgent agent(field);

    // TODO: реализуйте самостоятельно
    boost::system::error_code ec;

    auto endpoint = tcp::endpoint(net::ip::make_address(ip_str, ec), port);

    if (ec)
    {
        std::cout << "Wrong IP format"sv << std::endl;
        return;
    }

    net::io_context io_context;
    tcp::socket socket{io_context};
    socket.connect(endpoint, ec);

    if (ec)
    {
        std::cout << "Can't connect to server"sv << std::endl;
        return;
    }

    agent.StartGame(socket, true);
};

int main(int argc, const char **argv)
{
    if (argc != 3 && argc != 4)
    {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    if (argc == 3)
    {
        StartServer(fieldL, std::stoi(argv[2]));
    }
    else if (argc == 4)
    {
        StartClient(fieldL, argv[2], std::stoi(argv[3]));
    }
}

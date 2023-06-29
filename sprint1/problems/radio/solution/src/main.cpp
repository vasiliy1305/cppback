#include "audio.h"
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <string_view>

namespace net = boost::asio;
using net::ip::udp;

using namespace std::literals;

void SendToServer(std::string ip, int port, std::vector<char> data)
{
    size_t max_buffer_size = 1024;
    try
    {
        net::io_context io_context;

        // Перед отправкой данных нужно открыть сокет.
        // При открытии указываем протокол (IPv4 или IPv6) вместо endpoint.
        udp::socket socket(io_context, udp::v4());

        boost::system::error_code ec;
        auto endpoint = udp::endpoint(net::ip::make_address(ip, ec), port);

        socket.send_to(net::buffer(data), endpoint); // перепроверить какие данные можно передавать в констр buffer
        // socket.close()
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}

std::vector<char> ReciveFromClient(int port)
{

    try
    {
        const size_t max_buffer_size = 65000;
        boost::asio::io_context io_context;

        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

        // Создаём буфер достаточного размера, чтобы вместить датаграмму.
        std::array<char, max_buffer_size> recv_buf;
        udp::endpoint remote_endpoint;

        auto size = socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);

        std::vector<char> resualt(begin(recv_buf), end(recv_buf));

        return resualt;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}

void StartClient(uint16_t port)
{
    std::cout << "Client Started ..." << std::endl;
    Recorder recorder(ma_format_u8, 1);
    std::string ip = "127.0.0.1"; // оменять на динамическое
    while (true)
    {
        std::string str;
        std::cout << "Press Enter to record message..." << std::endl;
        std::getline(std::cin, str);

        auto rec_result = recorder.Record(65000, 1.5s);
        std::cout << "Recording done" << std::endl;

        SendToServer(ip, port, rec_result.data);
    }
}

void StartServer(uint16_t port)
{
    std::cout << "Server Started ..." << std::endl;
    Player player(ma_format_u8, 1);
    while (true)
    {
        Recorder::RecordingResult rec_result; // get data from udp
        rec_result.data = ReciveFromClient(port);
        rec_result.frames = 65000; // переделать
        std::cout << "UDP Data" << std::endl;

        player.PlayBuffer(rec_result.data.data(), rec_result.frames, 1.5s);
        std::cout << "Playing done" << std::endl;
    }
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        return 1; // обавить сообщение #todo
    }

    std::string type = argv[1];
    int port = std::stoi(argv[2]);

    if (type == "client")
    {
        StartClient(port);
    }
    else if (type == "server")
    {
        StartServer(port);
    }
    else
    {
        std::cout << "Wrong type" << std::endl;
    }
}

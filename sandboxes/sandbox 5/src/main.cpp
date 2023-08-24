#include <boost/beast.hpp>
#include <iostream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;

int main(int argc, char* argv[]) {
    using namespace http;
    using namespace std::literals;

    if (argc != 2) {
        std::cout << "Usage: "sv << argv[0] << " <file name>"sv << std::endl;
        return EXIT_FAILURE;
    }

    response<file_body> res;
    res.version(11);  // HTTP/1.1
    res.result(status::ok);
    res.insert(field::content_type, "text/plain"sv);

    file_body::value_type file;

    if (sys::error_code ec; file.open(argv[1], beast::file_mode::read, ec), ec) {
        std::cout << "Failed to open file "sv << argv[1] << std::endl;
        return EXIT_FAILURE;
    }

    res.body() = std::move(file);
    // Метод prepare_payload заполняет заголовки Content-Length и Transfer-Encoding
    // в зависимости от свойств тела сообщения
    res.prepare_payload();
}
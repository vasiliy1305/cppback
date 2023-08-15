#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server
{
    using namespace std::literals;

    void ReportError(beast::error_code ec, std::string_view what)
    {
        std::cerr << what << ": "sv << ec.message() << std::endl;
    }

    void SessionBase::Run()
    {
        // Вызываем метод Read, используя executor объекта stream_.
        // Таким образом вся работа со stream_ будет выполняться, используя его executor
        net::dispatch(stream_.get_executor(), beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
    }

    void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written)
    {
        if (ec)
        {
            return ReportError(ec, "write"sv);
        }

        if (close)
        {
            // Семантика ответа требует закрыть соединение
            return Close();
        }

        // Считываем следующий запрос
        Read();
    }

} // namespace http_server

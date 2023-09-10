#include "http_server.h"

namespace http_server
{

    void MyFormatter(logging::record_view const &rec, logging::formatting_ostream &strm)
    {
        strm << rec[logging::expressions::smessage];
    }

    void LogJson(std::string msg, boost::json::object data)
    {
        boost::json::object obj;
        boost::posix_time::ptime currentTime = boost::posix_time::second_clock::local_time();
        std::string isoTimestamp = to_iso_extended_string(currentTime);
        obj["timestamp"] = isoTimestamp;
        obj["data"] = data;
        obj["message"] = msg;

        BOOST_LOG_TRIVIAL(trace) << obj;
    }

    void LogServerStarted(net::ip::address addres, net::ip::port_type port)
    {
        boost::json::object data;
        data["port"] = port;
        data["address"] = addres.to_string();
        LogJson("server started", data);
    }

    void InitBoostLog()
    {
        logging::add_console_log(
            std::cout,
            keywords::format = &MyFormatter,
            keywords::auto_flush = true);
    }

    void LogRequestReceived(std::string IP, std::string URI, std::string method)
    {
        boost::json::object data;
        data["ip"] = IP;
        data["URI"] = URI;
        data["method"] = method;
        LogJson("request received", data);
    }

    void LogResponseSent(int response_time, int code, std::string content_type)
    {
        boost::json::object data;
        data["response_time"] = response_time;
        data["code"] = code;
        if (content_type != "")
        {
            data["content_type"] = content_type;
        }
        else
        {
            data["content_type"] = "null";
        }

        LogJson("response sent", data);
    }

    void LogServerExited(int code, std::string exception)
    {
        boost::json::object data;
        data["code"] = code;
        data["exception"] = exception;
        LogJson("server exited", data);
    }

    void LogError(int code, std::string text, std::string where)
    {
        boost::json::object data;
        data["code"] = code;
        data["text"] = text;
        data["where"] = where;
        LogJson("server exited", data);
    }

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
            LogError(ec.value(), ec.message(), "write");
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

    void SessionBase::Read()
    {
        start_read_ = std::chrono::high_resolution_clock::now();
        // Очищаем запрос от прежнего значения (метод Read может быть вызван несколько раз)
        request_ = {};
        stream_.expires_after(30s);
        // Считываем request_ из stream_, используя buffer_ для хранения считанных данных
        http::async_read(stream_, buffer_, request_,
                         // По окончании операции будет вызван метод OnRead
                         beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
    }

    void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read)
    {
        using namespace std::literals;
        if (ec == http::error::end_of_stream)
        {
            // Нормальная ситуация - клиент закрыл соединение
            LogError(ec.value(), ec.message(), "read");
            return Close();
        }
        if (ec)
        {
            LogError(ec.value(), ec.message(), "read");
            return ReportError(ec, "read"sv);
        }

        auto IP = stream_.socket().remote_endpoint().address().to_string();

        auto target_bsv = request_.target();
        std::string target_str(target_bsv.begin(), target_bsv.end());

        auto method_bsv = request_.method_string();
        std::string method_str(method_bsv.begin(), method_bsv.end());

        LogRequestReceived(IP, target_str, method_str);
        
        HandleRequest(std::move(request_));
    }

    void SessionBase::Close()
    {
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    }

} // namespace http_server

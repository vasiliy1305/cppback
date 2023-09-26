#include "logger.h"

namespace logger
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

} // namespace logger
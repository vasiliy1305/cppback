#pragma once

#include <boost/json.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL
#include <boost/log/core.hpp>        // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace net = boost::asio;

namespace logger
{
    namespace logging = boost::log;
        namespace keywords = boost::log::keywords;
    namespace sinks = boost::log::sinks;

    void LogJson(std::string msg, boost::json::object data);
    void LogServerStarted(net::ip::address addres, net::ip::port_type port);
    void LogRequestReceived(std::string IP, std::string URI, std::string method);
    void LogResponseSent(int response_time, int code, std::string content_type);
    void MyFormatter(logging::record_view const &rec, logging::formatting_ostream &strm);
    void LogServerExited(int code, std::string exception);
    void LogError(int code, std::string text, std::string where);
    void InitBoostLog();

} // namespace logger
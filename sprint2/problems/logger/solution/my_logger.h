#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>
#include <iostream>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger
{
    auto GetTime() const
    {
        if (manual_ts_)
        {
            return *manual_ts_;
        }

        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const
    {
        auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const
    {
        auto now = GetTime();
        if (manual_ts_)
        {
            now = *manual_ts_;
        }
        auto tt = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&tt);

        std::stringstream ss;
        ss << "/var/log/sample_log_" << std::put_time(&tm, "%Y_%m_%d") << ".log";
        return ss.str();
    }

    Logger() = default;
    Logger(const Logger &) = delete;

public:
    static Logger &GetInstance()
    {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template <class... Ts>
    void Log(const Ts &...args)
    {
        std::lock_guard<std::mutex> lock(mtx); 
        OpenFile();
        log_file_ << GetTimeStamp() << ": ";
        // Используем initializer_list и лямбду для вывода каждого аргумента
        (void)std::initializer_list<int>{(log_file_ << args, 0)...};
        log_file_ << std::endl;
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts)
    {
        std::lock_guard<std::mutex> lock(mtx); 
        manual_ts_ = ts; // add mutex?
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    std::ofstream log_file_;
    std::string curr_file_path_;
    std::mutex mtx;

    void OpenFile()
    {
        auto file_path = GetFileTimeStamp();
        if (curr_file_path_ != file_path)
        {
            curr_file_path_ = file_path;
            log_file_.close();
            log_file_.open(curr_file_path_, std::ios::app);
        }
    }
};

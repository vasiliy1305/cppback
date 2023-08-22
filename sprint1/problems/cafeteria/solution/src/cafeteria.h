#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/bind_executor.hpp>
#include <memory>
#include <iostream>
#include <chrono>
#include <syncstream>
#include <thread>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;
using namespace std::chrono;
using namespace std::literals;
using Timer = net::steady_timer;

class Logger
{
public:
    explicit Logger(std::string id)
        : id_(std::move(id))
    {
    }

    void LogMessage(std::string_view message) const
    {
        std::osyncstream os{std::cout};
        os << id_ << "> ["sv << duration<double>(steady_clock::now() - start_time_).count()
           << "s] "sv << message << std::endl;
    }

private:
    std::string id_;
    steady_clock::time_point start_time_{steady_clock::now()};
};

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

class Order : public std::enable_shared_from_this<Order>
{
public:
    Order(net::io_context &io, int id, HotDogHandler handler, std::shared_ptr<Sausage> sausage, std::shared_ptr<Bread> bread, std::shared_ptr<GasCooker> gas_cooker)
        : io_{io}, id_{id}, handler_{std::move(handler)}, sausage_(sausage), bread_(bread), gas_cooker_(gas_cooker)
    {
    }

    // Запускает асинхронное выполнение заказа
    void Execute()
    {
        // logger_.LogMessage("Order has been started."sv);
        BakeBread();
        FrySausage();
    }

private:
    net::io_context &io_;
    net::strand<net::io_context::executor_type> strand_{net::make_strand(io_)};
    int id_;
    HotDogHandler handler_;

    std::shared_ptr<Sausage> sausage_;
    std::shared_ptr<Bread> bread_;
    std::shared_ptr<GasCooker> gas_cooker_;

    bool bread_done = false;
    bool sausage_done = false;

    Logger logger_{std::to_string(id_)};
    Timer bake_timer_{io_, 1100ms};
    Timer fry_timer_{io_, 1600ms};

    void BakeBread()
    {
        Bread::Handler bh = [self = shared_from_this()]()
        {
            self->logger_.LogMessage("Start Bake Bread"sv);
            self->bake_timer_.async_wait(net::bind_executor(self->strand_, [self = self->shared_from_this()](sys::error_code ec)
                                                            { self->OnBaked(ec); }));
        };
        bread_->StartBake(*gas_cooker_, bh);
    }

    void FrySausage()
    {
        Sausage::Handler sh = [self = shared_from_this()]()
        {
            self->logger_.LogMessage("Start Fry Sausage"sv);
            self->fry_timer_.async_wait(net::bind_executor(self->strand_, [self = self->shared_from_this()](sys::error_code ec)
                                                           { self->OnFried(ec); }));
        };
        sausage_->StartFry(*gas_cooker_, sh);
    }

    void OnBaked(sys::error_code ec)
    {
        logger_.LogMessage("On Baked"sv);
        bread_->StopBaking();
        bread_done = true;
        CheckReadiness();
    }

    void OnFried(sys::error_code ec)
    {
        logger_.LogMessage("On Fried"sv);
        sausage_->StopFry();
        sausage_done = true;
        CheckReadiness();
    }

    void CheckReadiness()
    {
        if (IsReadyToPack())
        {
            handler_(Result(HotDog(id_, sausage_, bread_)));
        }
    }

    bool IsReadyToPack()
    {
        return bread_done && sausage_done;
    }
};

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria
{
public:
    explicit Cafeteria(net::io_context &io)
        : io_{io}
    {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void  OrderHotDog(HotDogHandler handler)
    {
        // TODO: Реализуйте метод самостоятельно
        // При необходимости реализуйте дополнительные классы
        const int id = hot_dog_id_++;
        auto sausage = store_.GetSausage();
        auto bread = store_.GetBread();
        auto hd = std::make_shared<Order>(io_, id, std::move(handler), sausage, bread, gas_cooker_);
        hd->Execute();
    }

private:
    net::io_context &io_;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
    int hot_dog_id_ = 0;
};

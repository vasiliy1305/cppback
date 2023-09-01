#include "model.h"

#include <stdexcept>

namespace model
{
    using namespace std::literals;

    void Map::AddOffice(Office office)
    {
        if (warehouse_id_to_index_.contains(office.GetId()))
        {
            throw std::invalid_argument("Duplicate warehouse");
        }

        const size_t index = offices_.size();
        Office &o = offices_.emplace_back(std::move(office));
        try
        {
            warehouse_id_to_index_.emplace(o.GetId(), index);
        }
        catch (...)
        {
            // Удаляем офис из вектора, если не удалось вставить в unordered_map
            offices_.pop_back();
            throw;
        }
    }

    void Game::AddMap(Map map)
    {
        const size_t index = maps_.size();
        if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted)
        {
            throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
        }
        else
        {
            try
            {
                maps_.emplace_back(std::move(map));
            }
            catch (...)
            {
                map_id_to_index_.erase(it);
                throw;
            }
        }
    }

    void Game::CreateSession(Map::Id map_id)
    {
        const size_t index = sessions_.size();
        if (auto [it, inserted] = map_id_to_game_index_.emplace(map_id, index); !inserted)
        {
            throw std::invalid_argument("Session with map_id "s + *map_id + " already exists"s);
        }
        else
        {
            try
            {
                sessions_.emplace_back(GameSession(map_id));
            }
            catch (...)
            {
                map_id_to_game_index_.erase(it);
                throw;
            }
        }
    }

    std::shared_ptr<Player> Players::Add(std::shared_ptr<Dog> dog, std::shared_ptr<GameSession> session, std::string name)
    {
        uint32_t id = players_.size();
        map_and_dog_to_index_[session->GetMapId()][dog->GetId()] = id;
        players_.emplace_back(Player(session, dog, id, name));
        return std::make_shared<Player>(players_.at(id));
    }

    std::shared_ptr<Player> Players::FindByDogIdAndMapId(Dog::Id dog_id, Map::Id map_id)
    {
        if (map_and_dog_to_index_.count(map_id))
        {
            if (map_and_dog_to_index_.at(map_id).count(dog_id))
            {
                auto id = map_and_dog_to_index_.at(map_id).at(dog_id);
                return std::make_shared<Player>(players_.at(id));
            }
        }
        return nullptr;
    }

    std::shared_ptr<Player> PlayerTokens::FindPlayerByToken(Token token)
    {
        if (token_to_player_.count(token))
        {
            return token_to_player_.at(token);
        }
        else
        {
            return nullptr;
        }
    }

    Token PlayerTokens::AddPlayer(Player &player)
    {
        Token token(GenerateToken());
        token_to_player_[token] = std::make_shared<Player>(player);
        return token;
    }

    std::string GenerateToken()
    {
        std::random_device random_device_;
        std::mt19937_64 generator1_{[&]
                                    {
                                        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
                                        return dist(random_device_);
                                    }()};
        std::mt19937_64 generator2_{[&]
                                    {
                                        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
                                        return dist(random_device_);
                                    }()};
        // Чтобы сгенерировать токен, получите из generator1_ и generator2_
        // два 64-разрядных числа и, переведя их в hex-строки, склейте в одну.
        // Вы можете поэкспериментировать с алгоритмом генерирования токенов,
        // чтобы сделать их подбор ещё более затруднительным
        // Получение чисел из генераторов
        uint64_t part1 = generator1_();
        uint64_t part2 = generator2_();

        // Преобразование чисел в hex-строки
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(16) << part1
           << std::hex << std::setfill('0') << std::setw(16) << part2;

        return ss.str();
    }

    std::pair<std::shared_ptr<model::Player>, Token> Game::Join(const std::string &user_name, const std::string &map_id)
    {
        // 1. проверить что карта сущ
        if (map_id_to_index_.count(Map::Id(map_id)))
        {
            // 2. если карта сущ. проверить нет ли уже сесии с этой картой
            if (!map_id_to_game_index_.count(Map::Id(map_id)))
            {
                // если сессия нет создать сессию
                CreateSession(Map::Id(map_id));
            }
            // 2. создаем собаку на сесии
            auto dog_ptr = FindSession(Map::Id(map_id))->AddDog(Dog::Id(curr_dog_id_++)); // что то тут не так
            // 3 создать игрока - выдать ему собаку -вернуть токен и id
            auto player = players_.Add(dog_ptr, FindSession(Map::Id(map_id)), user_name);
            // зарегестировать ишрока выдать токен
            auto token = tokens_.AddPlayer(*player);
            return {player, token};
        }
        else
        {
            // если карты нет 404
            return {nullptr, Token("")};
        }
    }

    std::optional<std::vector<model::Dog>> Game::GetPlayersByToken(const std::string &token_str)
    {
        Token token(token_str);
        auto player_ptr = tokens_.FindPlayerByToken(token);
        if (player_ptr)
        {
            return player_ptr->GetSession()->GetDogs();
        }
        else
        {
            return std::nullopt;
        }
    }

} // namespace model

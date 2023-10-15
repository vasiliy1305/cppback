#include "model.h"

#include <stdexcept>
#include <math.h>
#include <set>

namespace model
{
    using namespace std::literals;

    double randomDouble(double start, double end)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<> distrib(start, end);

        return distrib(gen);
    }

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

    void Game::CreateSession(std::shared_ptr<Map> map_ptr)
    {
        const size_t index = sessions_.size();
        if (auto [it, inserted] = map_id_to_game_index_.emplace(map_ptr->GetId(), index); !inserted)
        {
            throw std::invalid_argument("Session with map_id "s + *(map_ptr->GetId()) + " already exists"s);
        }
        else
        {
            try
            {
                sessions_.push_back(std::make_shared<GameSession>(map_ptr, loot_gen_));
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

        map_and_dog_to_index_[session->GetMap()->GetId()][dog->GetId()] = id;
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
            if (map_id_to_game_index_.count(Map::Id(map_id)) == 0)
            {
                // если сессия нет создать сессию
                CreateSession(std::make_shared<Map>(maps_.at(map_id_to_index_.at(Map::Id(map_id)))));
            }

            auto rnd_road_pnt = sessions_.at(sessions_.size() - 1)->GetRandomRoadPoint(randomize_spawn_points_);

            // auto rnd_road_pnt = GetRandomRoadPoint(map_id);

            // 2. создаем собаку на сене

            auto dog_ptr = FindSession(Map::Id(map_id))->AddDog(Dog::Id(curr_dog_id_++), rnd_road_pnt); //

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

    model::TwoDimVector GameSession::GetRandomRoadPoint(bool randomize_spawn_points)
    {
        // get random road
        std::random_device random_device_;
        std::mt19937_64 generator_{[&]
                                   {
                                       std::uniform_int_distribution<std::mt19937_64::result_type> dist;
                                       return dist(random_device_);
                                   }()};

        auto roads = map_ptr_->GetRoads();
        model::Road rnd_road = roads.at(generator_() % roads.size());

        if (!randomize_spawn_points)
        {
            rnd_road = roads.at(0);
        }

        auto road_start = rnd_road.GetStart();
        auto road_end = rnd_road.GetEnd();
        double rnd_x = randomDouble(road_start.x, road_end.x);
        double rnd_y = randomDouble(road_start.y, road_end.y);

        if (!randomize_spawn_points)
        {
            rnd_x = road_start.x;
            rnd_y = road_start.y;
        }

        return {rnd_x, rnd_y};
    }

    int GameSession::GetRandomNumber(int size)
    {
        std::random_device random_device_;
        std::mt19937_64 generator_{[&]
                                   {
                                       std::uniform_int_distribution<std::mt19937_64::result_type> dist;
                                       return dist(random_device_);
                                   }()};

        return generator_() % size;
    }

    std::vector<std::shared_ptr<Dog>> Game::GetDogsByToken(const std::string &token_str)
    {
        Token token(token_str);
        auto player_ptr = tokens_.FindPlayerByToken(token);
        if (player_ptr)
        {
            return player_ptr->GetSession()->GetDogs();
        }
        else
        {
            return {};
        }
    }

    void Game::UpdateTime(int delta)
    {
        for (auto session : sessions_)
        {
            session->UpdateTime(delta);
        }
    }

    void GameSession::UpdateTime(int delta_t) // todo на будущ декомпозировать ф-ию на три 1- переместить собак 2- создать стаф 3- собрать стаф
    {
        for (auto dog : dogs_) // move dog
        {
            auto dt = ((delta_t + 0.0) / 1000.0);
            auto curr_pos = dog->GetPos();
            auto curr_speed = dog->GetSpeed();
            auto next_pos = curr_pos + curr_speed * dt;
            // нужно определить что не вышли за границы дорог
            std::vector<Road> curr_roads;
            bool not_jump = false;
            for (auto road : map_ptr_->GetRoads())
            {
                if ((DistanceBetweenRoadAndPoint(road, curr_pos) <= ROAD_WIDTH) && (DistanceBetweenRoadAndPoint(road, next_pos) <= ROAD_WIDTH))
                {
                    not_jump = true;
                }

                if (DistanceBetweenRoadAndPoint(road, curr_pos) <= ROAD_WIDTH)
                {
                    curr_roads.push_back(road);
                }
            }
            if (not_jump)
            {
                dog->SetPos(next_pos);
            }
            else
            {
                // мы можем находится на нескольких дорогах нужно выбрать ту у которой в выбранном направлении мы продвинемся дальше
                next_pos = curr_pos; //
                for (auto road : curr_roads)
                {
                    auto border_pos = GetBorderPoint(road, dog);

                    if (dog->GetDir() == "U")
                    {
                        if (border_pos.y < next_pos.y)
                        {
                            next_pos = border_pos;
                        }
                    }
                    else if (dog->GetDir() == "D")
                    {
                        if (border_pos.y > next_pos.y)
                        {
                            next_pos = border_pos;
                        }
                    }
                    else if (dog->GetDir() == "L")
                    {
                        if (border_pos.x < next_pos.x)
                        {
                            next_pos = border_pos;
                        }
                    }
                    else if (dog->GetDir() == "R")
                    {
                        if (border_pos.x > next_pos.x)
                        {
                            next_pos = border_pos;
                        }
                    }
                    else
                    {
                    }
                }
                dog->SetPos(next_pos);
                dog->SetDir(""); // stop
            }
        }

        // пробуем создать loot
        for (int i = 0; i < loot_gen_.Generate(std::chrono::milliseconds(delta_t), loots_.size(), dogs_.size()); i++)
        {
            auto pos = GetRandomRoadPoint(true);
            auto type = GetRandomNumber(map_ptr_->GetLootTypeSize());
            loots_.push_back(Loot(pos, type, curr_loot_id_++));
        }

        // сбор
        auto gathering_event = collision_detector::FindGatherEvents((*this));
        if (gathering_event.size() != 0)
        {
            std::vector<int> items;
            items.resize(gathering_event.size()); 

            for (auto ge_it = gathering_event.begin(); ge_it != gathering_event.end(); ge_it++) 
            {
                if (IsOffice(ge_it->item_id))
                {
                    dogs_.at(ge_it->gatherer_id)->ClearLoots();
                }
                else
                {
                    if (dogs_[ge_it->gatherer_id]->GetLootSize() < map_ptr_->GetBagCapacity())
                    {
                        bool procc = false;
                        for (auto it_local_proc = items.begin(); it_local_proc != items.end(); it_local_proc++)
                        {
                            if (*it_local_proc == ge_it->item_id)
                                procc = true;
                        }

                        if (!procc)
                        {
                            dogs_[ge_it->gatherer_id]->AddLoot({{0.0, 0.0}, loots_[ge_it->item_id - OfficesCount()].GetType(), static_cast<int>(ge_it->item_id)});
                            items.push_back(ge_it->item_id);
                            Eraseloot(ge_it->item_id - OfficesCount());
                        }
                    }
                }
            }
        }
    }

    TwoDimVector operator+(const TwoDimVector &lhs, const TwoDimVector &rhs)
    {
        TwoDimVector result;
        result.x = lhs.x + rhs.x;
        result.y = lhs.y + rhs.y;
        return result;
    }

    TwoDimVector operator*(const TwoDimVector &vec, double scalar)
    {
        TwoDimVector result;
        result.x = vec.x * scalar;
        result.y = vec.y * scalar;
        return result;
    }

    TwoDimVector operator*(double scalar, const TwoDimVector &vec)
    {
        return vec * scalar;
    }

    TwoDimVector operator-(const TwoDimVector &lhs, const TwoDimVector &rhs)
    {
        return lhs + (rhs * (-1.0));
    }

    double ChebyshevDistance(TwoDimVector v1, TwoDimVector v2)
    {
        auto delta = v1 - v2;
        return std::max(std::abs(delta.x), std::abs(delta.y));
    }

    double DistanceBetweenRoadAndPoint(Road road, TwoDimVector pos)
    {
        if (road.IsHorizontal())
        {
            auto road_min_x = std::min(road.GetStart().x, road.GetEnd().x);
            auto road_max_x = std::max(road.GetStart().x, road.GetEnd().x);

            if (pos.x < road_min_x)
            {
                return ChebyshevDistance(pos, TwoDimVector(road_min_x, road.GetEnd().y)); // так как дорога горизонтальная у начала и конца совпадают
            }
            else if (pos.x > road_max_x)
            {
                return ChebyshevDistance(pos, TwoDimVector(road_max_x, road.GetEnd().y));
            }
            else
            {
                return std::abs(pos.y - road.GetEnd().y);
            }
        }
        else
        {
            auto road_min_y = std::min(road.GetStart().y, road.GetEnd().y);
            auto road_max_y = std::max(road.GetStart().y, road.GetEnd().y);

            if (pos.y < road_min_y)
            {
                return ChebyshevDistance(pos, TwoDimVector(road.GetEnd().x, road_min_y));
            }
            else if (pos.y > road_max_y)
            {
                return ChebyshevDistance(pos, TwoDimVector(road.GetEnd().x, road_max_y));
            }
            else
            {
                return std::abs(pos.x - road.GetEnd().x);
            }
        }
    }

    bool OnRoad(Road road, TwoDimVector pos)
    {
        return DistanceBetweenRoadAndPoint(road, pos) <= ROAD_WIDTH;
    }

    TwoDimVector GetBorderPoint(Road road, std::shared_ptr<model::Dog> dog)
    {
        auto road_min_x = std::min(road.GetStart().x, road.GetEnd().x) - ROAD_WIDTH; // L самая левая точка дороги
        auto road_max_x = std::max(road.GetStart().x, road.GetEnd().x) + ROAD_WIDTH; // R самая правая точка
        auto road_min_y = std::min(road.GetStart().y, road.GetEnd().y) - ROAD_WIDTH; // U самая верхняя
        auto road_max_y = std::max(road.GetStart().y, road.GetEnd().y) + ROAD_WIDTH; // D нижняя

        TwoDimVector curr_pos = dog->GetPos();

        if (dog->GetDir() == "U")
        {
            curr_pos.y = road_min_y; // идем до самой верхней точки дороги
        }
        else if (dog->GetDir() == "D")
        {
            curr_pos.y = road_max_y;
        }
        else if (dog->GetDir() == "L")
        {
            curr_pos.x = road_min_x;
        }
        else if (dog->GetDir() == "R")
        {
            curr_pos.x = road_max_x;
        }
        else
        {
        }
        return curr_pos;
    }

    TwoDimVector Dog::GetDirectionVec()
    {
        if (dir_str_ == "L")
        {
            return {-1, 0};
        }
        else if (dir_str_ == "R")
        {
            return {1, 0};
        }
        else if (dir_str_ == "U")
        {
            return {0, -1};
        }
        else if (dir_str_ == "D")
        {
            return {0, 1};
        }
        else
        {
            return {0, 0};
        }
    }

    void Dog::SetDir(std::string dir)
    {
        dir_str_ = dir;
        if (dir == "L")
        {
            speed_ = {-abs_speed_, 0};
            dir_ = Direction::WEST;
        }
        else if (dir == "R")
        {
            speed_ = {abs_speed_, 0};
            dir_ = Direction::EAST;
        }
        else if (dir == "U")
        {
            speed_ = {0, -abs_speed_};
            dir_ = Direction::NORTH;
        }
        else if (dir == "D")
        {
            speed_ = {0, abs_speed_};
            dir_ = Direction::SOUTH;
        }
        else
        {
            speed_ = {0, 0};
        }
    }

    const std::vector<Loot> &Game::GetLootsByToken(const std::string &token_str)
    {
        Token token(token_str);
        auto player_ptr = tokens_.FindPlayerByToken(token);
        if (player_ptr)
        {
            return player_ptr->GetSession()->GetLoots();
        }
        else
        {
            return empty_;
        }
    }

} // namespace model

#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <random>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <utility>
#include <optional>
#include <algorithm>

#include "tagged.h"
#include "loot_generator.h"
#include "collision_detector.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/unordered_map.hpp>

namespace model
{
    const double ROAD_WIDTH = 0.4;

    using Dimension = int;
    using Coord = Dimension;

    double randomDouble(double start, double end);

    struct Point
    {
        Coord x, y;
    };

    template <typename Archive>
    void serialize(Archive &ar, Point &p, [[maybe_unused]] const unsigned version)
    {
        ar & p.x;
        ar & p.y;
    }

    struct Size
    {
        Dimension width, height;
    };

    template <typename Archive>
    void serialize(Archive &ar, Size &s, [[maybe_unused]] const unsigned version)
    {
        ar & s.width;
        ar & s.height;
    }

    struct Rectangle
    {
        Point position;
        Size size;
    };

    template <typename Archive>
    void serialize(Archive &ar, Rectangle &r, [[maybe_unused]] const unsigned version)
    {
        ar & r.position;
        ar & r.size;
    }

    struct Offset
    {
        Dimension dx, dy;
    };

    template <typename Archive>
    void serialize(Archive &ar, Offset &off, [[maybe_unused]] const unsigned version)
    {
        ar & off.dx;
        ar & off.dy;
    }

    struct TwoDimVector
    {
        double x;
        double y;
    };

    TwoDimVector operator+(const TwoDimVector &lhs, const TwoDimVector &rhs);

    TwoDimVector operator-(const TwoDimVector &lhs, const TwoDimVector &rhs);

    TwoDimVector operator*(const TwoDimVector &vec, double scalar);

    TwoDimVector operator*(double scalar, const TwoDimVector &vec);

    bool operator==(const TwoDimVector &lhs, const TwoDimVector &rhs);

    std::ostream &operator<<(std::ostream &os, const TwoDimVector &obj);

    template <typename Archive>
    void serialize(Archive &ar, TwoDimVector &vec, [[maybe_unused]] const unsigned version)
    {
        ar & vec.x;
        ar & vec.y;
    }

    class Road
    {
        struct HorizontalTag
        {
            HorizontalTag() = default;
        };

        struct VerticalTag
        {
            VerticalTag() = default;
        };

    public:
        constexpr static HorizontalTag HORIZONTAL{};
        constexpr static VerticalTag VERTICAL{};

        Road() = default;

        Road(HorizontalTag, Point start, Coord end_x) noexcept
            : start_{start}, end_{end_x, start.y}
        {
        }

        Road(VerticalTag, Point start, Coord end_y) noexcept
            : start_{start}, end_{start.x, end_y}
        {
        }

        bool IsHorizontal() const noexcept
        {
            return start_.y == end_.y;
        }

        bool IsVertical() const noexcept
        {
            return start_.x == end_.x;
        }

        const Point GetStart() const noexcept
        {
            return start_;
        }

        const Point GetEnd() const noexcept
        {
            return end_;
        }

        template <class Archive>
        void serialize(Archive &ar, [[maybe_unused]] const unsigned int version)
        {
            ar & start_;
            ar & end_;
        }

    private:
        Point start_;
        Point end_;
    };

    class Building
    {
    public:
        Building() = default;
        explicit Building(Rectangle bounds) noexcept
            : bounds_{bounds}
        {
        }

        const Rectangle &GetBounds() const noexcept
        {
            return bounds_;
        }

        template <class Archive>
        void serialize(Archive &ar, [[maybe_unused]] const unsigned int version)
        {
            ar & bounds_;
        }

    private:
        Rectangle bounds_;
    };

    class Office
    {
    public:
        using Id = util::Tagged<std::string, Office>;

        Office() = default;

        Office(Id id, Point position, Offset offset) noexcept
            : id_{std::move(id)}, position_{position}, offset_{offset}
        {
        }

        const Id &GetId() const noexcept
        {
            return id_;
        }

        const Point GetPosition() const noexcept
        {
            return position_;
        }

        const Offset GetOffset() const noexcept
        {
            return offset_;
        }

        double GetWidth() const
        {
            return WIDTH;
        }

        template <class Archive>
        void serialize(Archive &ar, [[maybe_unused]] const unsigned int version)
        {
            ar &*id_;
            ar & position_;
            ar & offset_;
        }

    private:
        Id id_;
        Point position_;
        Offset offset_;

        double WIDTH = 0.5;
    };

    class Map
    {
    public:
        using Id = util::Tagged<std::string, Map>;
        using Roads = std::vector<Road>;
        using Buildings = std::vector<Building>;
        using Offices = std::vector<Office>;

        Map() = default;
        Map(Id id, std::string name, double dog_speed, int loot_types_size, std::vector<int> loot_scores) noexcept
            : id_(std::move(id)), name_(std::move(name)), dog_speed_(dog_speed), loot_types_size_(loot_types_size), loot_scores_(loot_scores)
        {
        }

        const Id &GetId() const noexcept
        {
            return id_;
        }

        const std::string &GetName() const noexcept
        {
            return name_;
        }

        const Buildings &GetBuildings() const noexcept
        {
            return buildings_;
        }

        const Roads &GetRoads() const noexcept
        {
            return roads_;
        }

        const Offices &GetOffices() const noexcept
        {
            return offices_;
        }

        void AddRoad(const Road &road)
        {
            roads_.emplace_back(road);
        }

        void AddBuilding(const Building &building)
        {
            buildings_.emplace_back(building);
        }

        void AddOffice(Office office);

        void SetDogSpeed(double spead)
        {
            dog_speed_ = spead;
        }

        double GetDogSpeed()
        {
            return dog_speed_;
        }

        int GetLootTypeSize()
        {
            return loot_types_size_;
        }

        void SetBagCapacity(int capacity)
        {
            bag_capacity_ = capacity;
        }

        int GetBagCapacity()
        {
            return bag_capacity_;
        }

        int GetLootScoreById(int id)
        {
            return loot_scores_.at(id);
        }

        template <class Archive>
        void serialize(Archive &ar, [[maybe_unused]] const unsigned int version)
        {
            ar &*id_;
            ar & name_;
            ar & roads_;
            ar & buildings_;
            ar & dog_speed_;
            ar & bag_capacity_;
            ar & warehouse_id_to_index_;
            ar & offices_;
            ar & loot_types_size_;
            ar & loot_scores_;
        }

    private:
        using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

        Id id_;
        std::string name_;
        Roads roads_;
        Buildings buildings_;
        double dog_speed_;
        int bag_capacity_;

        OfficeIdToIndex warehouse_id_to_index_;
        Offices offices_;
        int loot_types_size_;
        std::vector<int> loot_scores_;
    };

    // в этой задаче удобнее использовать такую метрику так как дороги прямоугольные
    double ChebyshevDistance(TwoDimVector x1, TwoDimVector x2);

    double DistanceBetweenRoadAndPoint(Road road, TwoDimVector pos);

    bool OnRoad(Road road, TwoDimVector pos);

    enum class Direction : char
    {
        NORTH = 'U',
        SOUTH = 'D',
        WEST = 'L',
        EAST = 'R'
    };

    class Loot
    {
    public:
        Loot() = default;
        Loot(TwoDimVector pos, int type, int id, int value) : pos_(pos), type_(type), id_(id), value_(value)
        {
        }

        TwoDimVector GetPos() const
        {
            return pos_;
        }

        int GetType() const
        {
            return type_;
        }

        int GetId() const
        {
            return id_;
        }

        double GetWidth() const
        {
            return WIDTH;
        }

        int GetValue() const
        {
            return value_;
        }

        bool operator==(const Loot &other) const
        {
            return (type_ == other.type_) && (pos_ == other.pos_) && (id_ == other.id_) && (value_ == other.value_) && (WIDTH == other.WIDTH);
        }

        template <class Archive>
        void serialize(Archive &ar, [[maybe_unused]] const unsigned int version)
        {
            ar & pos_;
            ar & type_;
            ar & id_;
            ar & value_;
            ar & WIDTH;
        }

    private:
        TwoDimVector pos_;
        int type_;
        int id_;
        int value_;
        double WIDTH = 0.0;
    };

    class Dog
    {
    public:
        using Id = util::Tagged<uint32_t, Dog>;

        Dog() = default;
        Dog(Id id, TwoDimVector pos, double abs_speed) : id_(id), pos_(pos), abs_speed_(abs_speed)
        {
            speed_ = {0.0, 0.0};
            dir_ = Direction::NORTH;
            dir_str_ = "";
        }

        const Id &GetId() const // добавил при сериализации
        {
            return id_;
        }

        const TwoDimVector GetPos() const // добавил при сериализации
        {
            return pos_;
        }

        const TwoDimVector GetSpeed() const // добавил при сериализации
        {
            return speed_;
        }

        const std::string GetDir() const // добавил при сериализации
        {
            return std::string(1, static_cast<char>(dir_));
        }

        void SetDir(std::string dir);

        void SetSpeed(TwoDimVector speed)
        {
            speed_ = speed;
        }

        void SetPos(TwoDimVector pos)
        {
            previous_pos_ = pos_;
            pos_ = pos;
        }

        void SetPreviousPos(TwoDimVector pos)
        {
            previous_pos_ = pos;
        }

        TwoDimVector GetPreviousPos() const // добавил при сериализации
        {
            return previous_pos_;
        }

        void AddLoot(Loot loot)
        {
            loots_.push_back(loot);
        }

        std::vector<Loot> GetLoots() const // добавил при сериализации
        {
            return loots_;
        }

        void ClearLoots()
        {
            // начислить очков и очистить рюкзак
            for (auto loot : loots_)
            {
                score_ += loot.GetValue();
            }
            loots_ = {};
        }

        size_t GetLootSize() const // добавил при сериализации
        {
            return GetLoots().size();
        }

        // единичный вектор направления двиижения
        TwoDimVector GetDirectionVec(); // добавил при сериализации

        double GetWidth() const
        {
            return WIDTH;
        }

        int GetScore() const // добавил при сериализации
        {
            return score_;
        }

        double GetAbsSpead() const // добавил при сериализации
        {
            return abs_speed_;
        }

        Direction GetDirection() const // добавил при сериализации
        {
            return dir_;
        }

        void SetScore(int score)
        {
            score_ = score;
        }

        template <class Archive>
        void serialize(Archive &ar, [[maybe_unused]] const unsigned int version)
        {
            ar &*id_;
            ar & speed_;
            ar & pos_;
            ar & previous_pos_;
            ar & dir_;
            ar & dir_str_;
            ar & dir_str_;
            ar & score_;
            ar & loots_;
            ar & WIDTH;
        }

    private:
        Id id_;
        TwoDimVector speed_;
        TwoDimVector pos_;
        TwoDimVector previous_pos_;
        Direction dir_;
        std ::string dir_str_;
        double abs_speed_;
        int score_ = 0;
        std::vector<Loot> loots_;
        double WIDTH = 0.6;
    };

    class GameSession : public collision_detector::ItemGathererProvider
    {
    public:
        using DogIdHasher = util::TaggedHasher<Dog::Id>;

        GameSession() = default;

        GameSession(std::shared_ptr<Map> map_ptr, loot_gen::LootGenerator loot_gen) : map_ptr_(map_ptr), loot_gen_(loot_gen)
        {
        }

        // GameSession() = delete;
        // GameSession(const GameSession &) = delete;
        // GameSession &operator=(const GameSession &) = delete;

        ~GameSession()
        {
        }

        std::shared_ptr<Map> GetMap() const
        {
            return map_ptr_;
        }

        std::vector<std::shared_ptr<Dog>> GetDogs() const
        {
            return dogs_;
        }

        std::shared_ptr<Dog> GetDog(Dog::Id dog_id)
        {
            if (dog_id_to_index_.count(dog_id))
            {
                return dogs_.at(dog_id_to_index_.at(dog_id));
            }
            return nullptr;
        }

        std::shared_ptr<Dog> AddDog(Dog::Id id, TwoDimVector pos)
        {

            dog_id_to_index_[id] = dogs_.size();
            dogs_.push_back(std::make_shared<Dog>(Dog(id, pos, map_ptr_->GetDogSpeed())));

            return (dogs_.at(dogs_.size() - 1));
        }

        void UpdateTime(int delta_t);
        model::TwoDimVector GetRandomRoadPoint(bool randomize_spawn_points);
        int GetRandomNumber(int size);

        const std::vector<Loot> &GetLoots() const
        {
            return loots_;
        }

        // реализация интерфейса ItemGathererProvider

        size_t OfficesCount() const
        {
            return map_ptr_->GetOffices().size();
        }

        size_t ItemsCount() const
        {
            return OfficesCount() + loots_.size(); // офисы тоже элементы
        }

        size_t GatherersCount() const
        {
            return dogs_.size();
        }

        bool IsOffice(size_t idx) const
        {
            return idx < OfficesCount(); // кажется что можно сделать элегантнее но я не понимаю как
        }

        collision_detector::Item GetItem(size_t idx) const
        {
            if (IsOffice(idx))
            {
                collision_detector::Item item = {{static_cast<double>(map_ptr_->GetOffices().at(idx).GetPosition().x),
                                                  static_cast<double>(map_ptr_->GetOffices().at(idx).GetPosition().y)},
                                                 map_ptr_->GetOffices().at(idx).GetWidth()};
                return item;
            }
            idx -= OfficesCount();
            collision_detector::Item item = {{loots_.at(idx).GetPos().x, loots_.at(idx).GetPos().y}, loots_.at(idx).GetWidth()};
            return item;
        }

        collision_detector::Gatherer GetGatherer(size_t idx) const
        {
            collision_detector::Gatherer gather = {{dogs_[idx]->GetPos().x, dogs_[idx]->GetPos().y}, {dogs_[idx]->GetPreviousPos().x, dogs_[idx]->GetPreviousPos().y}, dogs_[idx]->GetWidth()};
            return gather;
        }

        void Eraseloot(size_t idx) // возможна потенциальная ошибка
        {
            loots_.erase(loots_.begin() + idx);
        }

        int GetCurrLootId() const
        {
            return curr_loot_id_;
        }

        std::unordered_map<Dog::Id, uint32_t, DogIdHasher> GetDogIdToIndex() const
        {
            return dog_id_to_index_;
        }

        void InsertDog(Dog dog)
        {
            dogs_.push_back(std::make_shared<Dog>(dog));
        }

        void AddLoot(Loot loot)
        {
            loots_.push_back(loot);
        }

        void InsertIndexId(Dog::Id id, uint32_t index)
        {
            dog_id_to_index_[id] = index;
        }

        template <class Archive>
        void serialize(Archive &ar, [[maybe_unused]] const unsigned int version)
        {
            ar & dogs_;
            ar & map_ptr_;
            ar & dog_id_to_index_;
            ar & loots_;
            // ar & loot_gen_;
            ar & curr_loot_id_;
        }

        void SetLootGen(loot_gen::LootGenerator loot_gen)
        {
            loot_gen_ = loot_gen;
        }

    private:
        // using DogIdHasher = util::TaggedHasher<Dog::Id>;
        std::vector<std::shared_ptr<Dog>> dogs_; //
        std::shared_ptr<Map> map_ptr_;
        std::unordered_map<Dog::Id, uint32_t, DogIdHasher> dog_id_to_index_;
        std::vector<Loot> loots_;
        loot_gen::LootGenerator loot_gen_;
        int curr_loot_id_ = 0;
    };

    namespace detail
    {
        struct TokenTag
        {
        };
    } // namespace detail

    using Token = util::Tagged<std::string, detail::TokenTag>;

    class Player
    {
    public:
        using Id = util::Tagged<uint32_t, Player>;

        Player() = default;

        Player(std::shared_ptr<GameSession> session, std::shared_ptr<Dog> dog, uint32_t id, std::string name) : session_(session), dog_(dog), id_{id}, name_(name)
        {
        }

        std::shared_ptr<GameSession> GetSession()
        {
            return session_;
        }

        std::shared_ptr<Dog> GetDog() const
        {
            return dog_;
        }

        Id GetId() const
        {
            return id_;
        }

        std::string GetName() const
        {
            return name_;
        }

        std::shared_ptr<GameSession> GetSession() const
        {
            return session_;
        }

        template <class Archive>
        void serialize(Archive &ar, [[maybe_unused]] const unsigned int version)
        {
            ar & session_;
            ar & dog_;
            ar &*id_;
            ar & name_;
        }

    private:
        std::shared_ptr<GameSession> session_;
        std::shared_ptr<Dog> dog_;
        Id id_;
        std::string name_;
    };

    std::string GenerateToken();

    class PlayerTokens
    {
    public:
        PlayerTokens() = default;
        using TokenHasher = util::TaggedHasher<Token>;
        Token AddPlayer(Player &player);
        std::shared_ptr<Player> FindPlayerByToken(Token token);

        std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher> GetTokens() const
        {
            return token_to_player_;
        }

        template <class Archive>
        void serialize(Archive &ar, [[maybe_unused]] const unsigned int version)
        {
            ar & token_to_player_;
        }

    private:
        std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher> token_to_player_;
    };

    class Players
    {
    public:
        Players() = default;

        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using DogIdHasher = util::TaggedHasher<Dog::Id>;
        using MapIdandDogIdToPlayer = std::unordered_map<Map::Id, std::unordered_map<Dog::Id, uint32_t, DogIdHasher>, MapIdHasher>;

        std::shared_ptr<Player> Add(std::shared_ptr<Dog> dog, std::shared_ptr<GameSession> session, std::string name);
        std::shared_ptr<Player> FindByDogIdAndMapId(Dog::Id dog_id, Map::Id map_id);
        std::vector<Player> GetPlayersVec() const
        {
            return players_;
        }
        template <class Archive>
        void serialize(Archive &ar, [[maybe_unused]] const unsigned int version)
        {
            ar & map_and_dog_to_index_;
            ar & players_;
        }

    private:
        MapIdandDogIdToPlayer map_and_dog_to_index_;
        std::vector<Player> players_;
    };

    class Game
    {
    public:
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        Game() = default;

        // Конструктор копирования
        Game(const Game &other)
        {
            std::cout << "Конструктор копирования вызван\n";
        }

        // Оператор присваивания через копирование
        Game &operator=(const Game &other)
        {
            std::cout << "Оператор присваивания через копирование вызван\n";
            return *this;
        }

        Game(loot_gen::LootGenerator loot_gen) : loot_gen_(loot_gen)
        {
        }

        using Maps = std::vector<Map>;

        void AddMap(Map map);

        const Maps &GetMaps() const noexcept
        {
            return maps_;
        }

        const Map *FindMap(const Map::Id &id) const noexcept // отказаться полностб от обычных указателей
        {
            if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end())
            {
                return &maps_.at(it->second);
            }
            return nullptr;
        }

        std::shared_ptr<GameSession> FindSession(const Map::Id &id)
        {
            if (auto it = map_id_to_game_index_.find(id); it != map_id_to_game_index_.end())
            {
                return sessions_.at(it->second);
            }
            return nullptr;
        }

        void CreateSession(std::shared_ptr<Map> map_ptr);

        std::pair<std::shared_ptr<model::Player>, Token> Join(const std::string &user_name, const std::string &map_id);
        std::vector<std::shared_ptr<Dog>> GetDogsByToken(const std::string &token_str);
        const std::vector<Loot> &GetLootsByToken(const std::string &token_str);

        std::shared_ptr<model::Player> GetPlayerByToken(std::string token)
        {
            return tokens_.FindPlayerByToken(Token(token));
        }

        void SetDfaultDogSpeed(double speed)
        {
            default_dog_speed_ = speed;
        }

        double GetDefDogSpeed()
        {
            return default_dog_speed_;
        }

        void UpdateTime(int delta);
        void SetRandomizeSpawnPoints(bool rnd)
        {
            randomize_spawn_points_ = rnd;
        }

        void SetDefaultBagCapacity(int capacity)
        {
            default_bag_capacity_ = capacity;
        }

        int GetDefaultBagCapacity()
        {
            return default_bag_capacity_;
        }

        std::vector<std::shared_ptr<GameSession>> GetSessions() const
        {
            return sessions_;
        }

        Players GetPlayers() const
        {
            return players_;
        }

        uint32_t GetCurrDogId() const
        {
            return curr_dog_id_;
        }

        void SetLootGen(loot_gen::LootGenerator loot_gen)
        {
            loot_gen_ = loot_gen;
            for (auto session : sessions_)
            {
                session->SetLootGen(loot_gen);
            }
        }

        template <class Archive>
        void serialize(Archive &ar, [[maybe_unused]] const unsigned int version)
        {
            ar & sessions_;
            ar & players_;
            ar & tokens_;
            ar & map_id_to_index_;
            ar & curr_dog_id_;
            ar & default_dog_speed_;
            ar & randomize_spawn_points_;

            ar & default_bag_capacity_;
            ar & empty_;
            ar & maps_;
            ar & map_id_to_game_index_;
        }

    private:
        std::vector<std::shared_ptr<GameSession>> sessions_; // +
        Players players_;                                    // +
        PlayerTokens tokens_;                                // +
        MapIdToIndex map_id_to_index_;                       // - MapIdToIndex map_id_to_game_index_;
        uint32_t curr_dog_id_ = 0;                           // +

        double default_dog_speed_ = 1.0;      // -
        bool randomize_spawn_points_ = false; // -
        loot_gen::LootGenerator loot_gen_;    // -
        int default_bag_capacity_ = 3;        // -
        std::vector<Loot> empty_;             // -
        std::vector<Map> maps_;
        MapIdToIndex map_id_to_game_index_; // -
    };

    TwoDimVector GetBorderPoint(Road road, std::shared_ptr<model::Dog> dog);
} // namespace model

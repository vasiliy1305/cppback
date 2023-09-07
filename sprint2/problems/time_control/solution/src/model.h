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

namespace model
{

    double randomDouble(double start, double end);

    using Dimension = int;
    using Coord = Dimension;

    struct Point
    {
        Coord x, y;
    };

    struct Size
    {
        Dimension width, height;
    };

    struct Rectangle
    {
        Point position;
        Size size;
    };

    struct Offset
    {
        Dimension dx, dy;
    };

    const double ROAD_WIDTH = 0.4;

    class Road
    {
        struct HorizontalTag
        {
            explicit HorizontalTag() = default;
        };

        struct VerticalTag
        {
            explicit VerticalTag() = default;
        };

    public:
        constexpr static HorizontalTag HORIZONTAL{};
        constexpr static VerticalTag VERTICAL{};

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

        Point GetStart() const noexcept
        {
            return start_;
        }

        Point GetEnd() const noexcept
        {
            return end_;
        }

    private:
        Point start_;
        Point end_;
    };

    class Building
    {
    public:
        explicit Building(Rectangle bounds) noexcept
            : bounds_{bounds}
        {
        }

        const Rectangle &GetBounds() const noexcept
        {
            return bounds_;
        }

    private:
        Rectangle bounds_;
    };

    class Office
    {
    public:
        using Id = util::Tagged<std::string, Office>;

        Office(Id id, Point position, Offset offset) noexcept
            : id_{std::move(id)}, position_{position}, offset_{offset}
        {
        }

        const Id &GetId() const noexcept
        {
            return id_;
        }

        Point GetPosition() const noexcept
        {
            return position_;
        }

        Offset GetOffset() const noexcept
        {
            return offset_;
        }

    private:
        Id id_;
        Point position_;
        Offset offset_;
    };

    class Map
    {
    public:
        using Id = util::Tagged<std::string, Map>;
        using Roads = std::vector<Road>;
        using Buildings = std::vector<Building>;
        using Offices = std::vector<Office>;

        Map(Id id, std::string name, double dog_speed) noexcept
            : id_(std::move(id)), name_(std::move(name)), dog_speed_(dog_speed)
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

    private:
        using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

        Id id_;
        std::string name_;
        Roads roads_;
        Buildings buildings_;
        double dog_speed_;

        OfficeIdToIndex warehouse_id_to_index_;
        Offices offices_;
    };

    struct TwoDimVector
    {
        double x;
        double y;
    };

    TwoDimVector operator+(const TwoDimVector &lhs, const TwoDimVector &rhs);

    TwoDimVector operator-(const TwoDimVector &lhs, const TwoDimVector &rhs);

    TwoDimVector operator*(const TwoDimVector &vec, double scalar);

    TwoDimVector operator*(double scalar, const TwoDimVector &vec);

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

    class Dog
    {
    public:
        using Id = util::Tagged<uint32_t, Dog>;
        Dog(Id id, TwoDimVector pos, double abs_speed) : id_(id), pos_(pos), abs_speed_(abs_speed)
        {
            speed_ = {0.0, 0.0};
            dir_ = Direction::NORTH;
            dir_str_ = "";
        }

        const Id &GetId()
        {
            return id_;
        }

        TwoDimVector GetPos()
        {
            return pos_;
        }

        TwoDimVector GetSpeed()
        {
            return speed_;
        }

        std::string GetDir()
        {
            return std::string(1, static_cast<char>(dir_));
        }

        void SetDir(std::string dir)
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

        void UpdateTime(int delta);

        void SetPos(TwoDimVector pos)
        {
            pos_ = pos;
        }

        // единичный вектор направления двиижения
        TwoDimVector GetDirectionVec()
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

    private:
        Id id_;
        TwoDimVector speed_;
        TwoDimVector pos_;
        Direction dir_;
        std ::string dir_str_;

        double abs_speed_;
    };

    class GameSession
    {
    public:
        GameSession(std::shared_ptr<Map> map_ptr) : map_ptr_(map_ptr)
        {
        }

        GameSession() = delete;
        GameSession(const GameSession &) = delete;
        GameSession &operator=(const GameSession &) = delete;

        ~GameSession()
        {
        }

        std::shared_ptr<Map> GetMap()
        {
            return map_ptr_;
        }

        std::vector<std::shared_ptr<Dog>> GetDogs()
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

    private:
        using DogIdHasher = util::TaggedHasher<Dog::Id>;
        std::vector<std::shared_ptr<Dog>> dogs_;
        std::shared_ptr<Map> map_ptr_;
        std::unordered_map<Dog::Id, uint32_t, DogIdHasher> dog_id_to_index_;
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

        Player(std::shared_ptr<GameSession> session, std::shared_ptr<Dog> dog, uint32_t id, std::string name) : session_(session), dog_(dog), id_{id}, name_(name)
        {
        }

        std::shared_ptr<GameSession> GetSession()
        {
            return session_;
        }

        std::shared_ptr<Dog> GetDog()
        {
            return dog_;
        }

        Id GetId()
        {
            return id_;
        }

        std::string GetName()
        {
            return name_;
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
        Token AddPlayer(Player &player);
        std::shared_ptr<Player> FindPlayerByToken(Token token);

    private:
        using TokenHasher = util::TaggedHasher<Token>;
        std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher> token_to_player_;
    };

    class Players
    {
    public:
        std::shared_ptr<Player> Add(std::shared_ptr<Dog> dog, std::shared_ptr<GameSession> session, std::string name);
        std::shared_ptr<Player> FindByDogIdAndMapId(Dog::Id dog_id, Map::Id map_id);

    private:
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using DogIdHasher = util::TaggedHasher<Dog::Id>;
        using MapIdandDogIdToPlayer = std::unordered_map<Map::Id, std::unordered_map<Dog::Id, uint32_t, DogIdHasher>, MapIdHasher>;

        MapIdandDogIdToPlayer map_and_dog_to_index_;
        std::vector<Player> players_;
    };

    class Game
    {
    public:
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

    private:
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        std::vector<Map> maps_;
        std::vector<std::shared_ptr<GameSession>> sessions_;
        Players players_;
        PlayerTokens tokens_;

        MapIdToIndex map_id_to_index_;
        MapIdToIndex map_id_to_game_index_;

        uint32_t curr_dog_id_ = 0;
        double default_dog_speed_ = 1.0;
    };

} // namespace model

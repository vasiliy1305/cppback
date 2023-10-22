

// #include "model.h"



// namespace model
// {

//     class LootRepr
//     {
//     public:
//         LootRepr() = default;
//         explicit LootRepr(const Loot &loot)
//             : pos_(loot.GetPos()), type_(loot.GetType()), id_(loot.GetId()), value_(loot.GetValue())
//         {
//         }

//         [[nodiscard]] Loot Restore() const
//         {
//             Loot loot(pos_, type_, id_, value_);
//             return loot;
//         }

//         template <typename Archive>
//         void serialize(Archive &ar, [[maybe_unused]] const unsigned version)
//         {
//             ar & pos_;
//             ar & type_;
//             ar & id_;
//             ar & value_;
//         }

//     private:
//         TwoDimVector pos_;
//         int type_;
//         int id_;
//         int value_;
//     };

//     class DogRepr
//     {
//     public:
//         DogRepr() = default;

//         explicit DogRepr(const Dog &dog)
//             : id_(dog.GetId()),
//               speed_(dog.GetSpeed()),
//               pos_(dog.GetPos()),
//               previous_pos_(dog.GetPreviousPos()),
//               dir_(dog.GetDirection()),
//               dir_str_(dog.GetDir()),
//               abs_speed_(dog.GetAbsSpead()),
//               score_(dog.GetScore())
//         {
//             for (auto loot : dog.GetLoots())
//             {
//                 loots_.emplace_back(loot);
//             }
//         }

//         [[nodiscard]] model::Dog Restore() const
//         {
//             model::Dog dog{id_, pos_, abs_speed_};
//             dog.SetSpeed(speed_);
//             dog.SetDir(dir_str_);
//             dog.SetScore(score_);
//             dog.SetPreviousPos(previous_pos_);
//             for (const auto &loot : loots_)
//             {
//                 dog.AddLoot(loot.Restore());
//             }
//             return dog;
//         }

//         template <typename Archive>
//         void serialize(Archive &ar, [[maybe_unused]] const unsigned version)
//         {
//             ar &*id_;
//             ar & speed_;
//             ar & pos_;
//             ar & previous_pos_;
//             ar & dir_;
//             ar & dir_str_;
//             ar & abs_speed_;
//             ar & score_;
//             ar & loots_;
//         }

//     private:
//         model::Dog::Id id_ = model::Dog::Id{0u};
//         TwoDimVector speed_;
//         TwoDimVector pos_;
//         TwoDimVector previous_pos_;
//         Direction dir_;
//         std ::string dir_str_;
//         double abs_speed_;
//         int score_ = 0;
//         std::vector<LootRepr> loots_;
//     };



//     // class GameSessionRepr
//     // {
//     // public:
//     //     GameSessionRepr() = default;
//     //     explicit GameSessionRepr(const GameSession &gem_ses)
//     //         : map_id_(*((*(gem_ses.GetMap())).GetId())), curr_loot_id_(gem_ses.GetCurrLootId())
//     //     {
//     //         for (auto dog_ptr : gem_ses.GetDogs())
//     //         {
//     //             dogs_.emplace_back(*dog_ptr);
//     //         }

//     //         for (const auto loot : gem_ses.GetLoots())
//     //         {
//     //             loots_.emplace_back(loot);
//     //         }

//     //         for (auto [id, indx] : gem_ses.GetDogIdToIndex())
//     //         {
//     //             dog_ids_.push_back(id);
//     //             indexes_.push_back(indx);
//     //         }
//     //     }

//     //     [[nodiscard]] std::shared_ptr<GameSession> Restore(std::shared_ptr<Map> map_ptr, loot_gen::LootGenerator loot_gen) const
//     //     {
//     //         std::shared_ptr<GameSession> gs_ptr = std::make_shared<GameSession>(map_ptr, loot_gen); // GameSession(std::shared_ptr<Map> map_ptr, loot_gen::LootGenerator loot_gen)
//     //         for (auto dog : dogs_)
//     //         {
//     //             gs_ptr->InsertDog(dog.Restore());
//     //         }

//     //         for (auto loot : loots_)
//     //         {
//     //             gs_ptr->AddLoot(loot.Restore());
//     //         }

//     //         for (size_t i = 0; i < dog_ids_.size(); i++)
//     //         {
//     //             gs_ptr->InsertIndexId(dog_ids_.at(i), indexes_.at(i));
//     //         }

//     //         return gs_ptr;
//     //     }

//     //     template <typename Archive>
//     //     void serialize(Archive &ar, [[maybe_unused]] const unsigned version)
//     //     {
//     //         ar & dogs_;
//     //         ar & map_ptr_;
//     //         ar & dog_id_to_index_;
//     //         ar & loots_;
//     //         ar & loot_gen_;
//     //         ar & curr_loot_id_;
//     //     }

//     //     std::string GetMapIdStr()
//     //     {
//     //         return map_id_;
//     //     }

//     // private:
//     //     using DogIdHasher = util::TaggedHasher<Dog::Id>;

//     //     std::vector<std::shared_ptr<Dog>> dogs_; //
//     //     std::shared_ptr<Map> map_ptr_;
//     //     std::unordered_map<Dog::Id, uint32_t, DogIdHasher> dog_id_to_index_;
//     //     std::vector<Loot> loots_;
//     //     loot_gen::LootGenerator loot_gen_;
//     //     int curr_loot_id_ = 0;

//     // };

//     // class PlayerRepr
//     // {
//     // public:
//     //     PlayerRepr() = default;
//     //     PlayerRepr(const Player &player)
//     //         : id_(player.GetId()), name_(player.GetName())
//     //     // , dog_(*(player.GetDog()))
//     //     // , session_(*(player.GetSession()))
//     //     {
//     //     }

//     //     template <typename Archive>
//     //     void serialize(Archive &ar, [[maybe_unused]] const unsigned version)
//     //     {
//     //         ar &*id_;
//     //         ar & name_;
//     //         // ar & dog_;
//     //         // ar & session_;
//     //     }

//     //     Player Restore(std::shared_ptr<GameSession> session, std::shared_ptr<Dog> dog)
//     //     {
//     //         Player player(session, dog, *id_, name_); //
//     //     }

//     // private:
//     //     // GameSessionRepr session_;
//     //     // DogRepr dog_;
//     //     Player::Id id_;
//     //     std::string name_;
//     // };

//     // class PlayersRepr
//     // {
//     // public:
//     //     PlayersRepr() = default;
//     //     PlayersRepr(const Players &players)
//     //     {
//     //         for (auto pl : players.GetPlayersVec())
//     //         {
//     //             players_.emplace_back(pl);
//     //         }
//     //     }

//     //     template <typename Archive>
//     //     Players serialize(Archive &ar, [[maybe_unused]] const unsigned version)
//     //     {
//     //         Players players;
//     //         for (auto pl : players_)
//     //         {
//     //             // players.Add(pl.Restore())
//     //         }
//     //     }

//     // private:
//     //     std::vector<PlayerRepr> players_;
//     // };

//     // class PlayerTokensRepr
//     // {
//     // public:
//     //     PlayerTokensRepr() = default;
//     //     PlayerTokensRepr(PlayerTokens tokens)
//     //     {
//     //     }

//     //     template <typename Archive>
//     //     void serialize(Archive &ar, [[maybe_unused]] const unsigned version)
//     //     {

//     //     }

//     // private:
//     // };

//     // class GameRepr
//     // {
//     // public:
//     //     GameRepr() = default;
//     //     GameRepr(Game game) : players_(game.GetPlayers()), curr_dog_id_(game.GetCurrDogId())
//     //     {
//     //         for (auto sess : game.GetSessions())
//     //         {
//     //             sessions_.emplace_back(*sess);
//     //         }
//     //     }

//     //     template <typename Archive>
//     //     void serialize(Archive &ar, [[maybe_unused]] const unsigned version)
//     //     {
//     //         ar & sessions_;
//     //     }

//     // private:
//     //     std::vector<GameSessionRepr> sessions_;
//     //     PlayersRepr players_;

//     //     PlayerTokens tokens_; // +
//     //     Game::MapIdToIndex map_id_to_index_;
//     //     Game::MapIdToIndex map_id_to_game_index_; // +

//     //     uint32_t curr_dog_id_ = 0;
//     // };

// } // namespace model

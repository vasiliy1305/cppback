#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include "../src/file_loader.h"
#include "../src/json_loader.h"

using namespace model;
using namespace std::literals;
namespace
{

    using InputArchive = boost::archive::text_iarchive;
    using OutputArchive = boost::archive::text_oarchive;

    struct Fixture
    {
        std::stringstream strm;
        OutputArchive output_archive{strm};
    };

} // namespace

SCENARIO_METHOD(Fixture, "Point serialization")
{
    GIVEN("A point")
    {
        const geom::Point2D p{10, 20};
        WHEN("point is serialized")
        {
            output_archive << p;

            THEN("it is equal to point after serialization")
            {
                InputArchive input_archive{strm};
                geom::Point2D restored_point;
                input_archive >> restored_point;
                CHECK(p == restored_point);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Road Serialization")
{
    GIVEN("A road")
    {
        model::Road initial(model::Road::HORIZONTAL, {1, 1}, 3);
        {
            output_archive << initial;
        }
        {

            model::Road restored;

            InputArchive input_archive{strm};
            input_archive >> restored;
        }
    }
}

SCENARIO_METHOD(Fixture, "Building Serialization")
{
    GIVEN("A Building")
    {
        model::Building initial;
        {
            output_archive << initial;
        }
        {

            model::Building restored;

            InputArchive input_archive{strm};
            input_archive >> restored;
        }
    }
}

SCENARIO_METHOD(Fixture, "Office Serialization")
{
    GIVEN("A Office")
    {
        model::Office initial;
        {
            output_archive << initial;
        }
        {

            model::Office restored;

            InputArchive input_archive{strm};
            input_archive >> restored;
        }
    }
}

SCENARIO_METHOD(Fixture, "Map Serialization")
{
    GIVEN("A Map")
    {
        model::Map initial;
        {
            output_archive << initial;
        }
        {

            model::Map restored;

            InputArchive input_archive{strm};
            input_archive >> restored;
        }
    }
}

SCENARIO_METHOD(Fixture, "Loot Serialization")
{
    GIVEN("A Loot")
    {
        model::Loot initial;
        {
            output_archive << initial;
        }
        {

            model::Loot restored;

            InputArchive input_archive{strm};
            input_archive >> restored;
        }
    }
}

SCENARIO_METHOD(Fixture, "Dog Serialization")
{
    GIVEN("A Dog")
    {
        model::Dog initial;
        {
            output_archive << initial;
        }
        {

            model::Dog restored;

            InputArchive input_archive{strm};
            input_archive >> restored;
        }
    }
}

SCENARIO_METHOD(Fixture, "GameSession Serialization")
{
    GIVEN("A GameSession")
    {
        model::GameSession initial;
        {
            output_archive << initial;
        }
        {

            model::GameSession restored;

            InputArchive input_archive{strm};
            input_archive >> restored;
        }
    }
}

SCENARIO_METHOD(Fixture, "Player Serialization")
{
    GIVEN("A Player")
    {
        model::Player initial;
        {
            output_archive << initial;
        }
        {

            model::Player restored;

            InputArchive input_archive{strm};
            input_archive >> restored;
        }
    }
}

SCENARIO_METHOD(Fixture, "PlayerTokens Serialization")
{
    GIVEN("A PlayerTokens")
    {
        model::PlayerTokens initial;
        {
            output_archive << initial;
        }
        {

            model::PlayerTokens restored;

            InputArchive input_archive{strm};
            input_archive >> restored;
        }
    }
}

SCENARIO_METHOD(Fixture, "Players Serialization")
{
    GIVEN("A Players")
    {
        model::Players initial;
        {
            output_archive << initial;
        }
        {

            model::Players restored;

            InputArchive input_archive{strm};
            input_archive >> restored;
        }
    }
}

SCENARIO_METHOD(Fixture, "GameSerialization")
{
    GIVEN("A Game")
    {
        model::Game game = json_loader::LoadGame("../../data/config.json");
        {
            output_archive << game;
        }
        {
            double probability = 0.5;
            double period = 1.0;
            loot_gen::LootGenerator::TimeInterval interval(static_cast<int64_t>(period * 1000));
            loot_gen::LootGenerator loot_gen(interval, probability); // todo - добавить генератор
            model::Game restored(loot_gen);

            InputArchive input_archive{strm};
            input_archive >> restored;
            CHECK(game.GetCurrDogId() == restored.GetCurrDogId());
            CHECK(game.GetDefaultBagCapacity() == restored.GetDefaultBagCapacity());
            CHECK(game.GetDefDogSpeed() == restored.GetDefDogSpeed());
            CHECK(game.GetDogsByToken("") == restored.GetDogsByToken(""));
            CHECK(game.GetLootsByToken("") == restored.GetLootsByToken(""));
            // CHECK(game.GetMaps() == restored.GetMaps());
            CHECK(game.GetPlayerByToken("") == restored.GetPlayerByToken(""));
            // CHECK(game.GetPlayers() == restored.GetPlayers());
            CHECK(game.GetSessions() == restored.GetSessions());

        }
    }
}

// SCENARIO_METHOD(Fixture, "Loot Serialization")
// {
//     GIVEN("a loot")
//     {
//         Loot loot({1.0, 2.0}, 1, 2, 3);
//         {
//             LootRepr repr_loot(loot);
//             output_archive << repr_loot;
//         }
//         {
//             InputArchive input_archive{strm};
//             LootRepr repr;
//             input_archive >> repr;
//             const auto restored = repr.Restore();

//             CHECK(loot.GetId() == restored.GetId());
//             CHECK(loot.GetPos() == restored.GetPos());
//             CHECK(loot.GetType() == restored.GetType());
//             CHECK(loot.GetValue() == restored.GetValue());
//             CHECK(loot.GetWidth() == restored.GetWidth());
//         }
//     }
// }

// SCENARIO_METHOD(Fixture, "Dog Serialization")
// {
//     GIVEN("a dog")
//     {
//         const auto dog = []
//         {
//             Dog dog{Dog::Id{42}, {42.2, 12.5}, 3.0};
//             dog.SetScore(42);
//             dog.AddLoot({{1.0, 2.0}, 1, 2, 3}); // Loot(TwoDimVector pos, int type, int id, int value)
//             dog.SetDir("U");
//             return dog;
//         }();

//         WHEN("dog is serialized")
//         {
//             {
//                 model::DogRepr repr{dog};
//                 output_archive << repr;
//             }

//             THEN("it can be deserialized")
//             {
//                 InputArchive input_archive{strm};
//                 model::DogRepr repr;
//                 input_archive >> repr;
//                 const auto restored = repr.Restore();

//                 CHECK(dog.GetId() == restored.GetId());
//                 CHECK(dog.GetAbsSpead() == restored.GetAbsSpead());
//                 CHECK(dog.GetDir() == restored.GetDir());
//                 CHECK(dog.GetDirection() == restored.GetDirection());
//                 CHECK(dog.GetLoots() == restored.GetLoots());
//                 CHECK(dog.GetLootSize() == restored.GetLootSize());
//                 CHECK(dog.GetPos() == restored.GetPos());
//                 CHECK(dog.GetPreviousPos() == restored.GetPreviousPos());
//                 CHECK(dog.GetScore() == restored.GetScore());
//                 CHECK(dog.GetSpeed() == restored.GetSpeed());
//                 CHECK(dog.GetWidth() == restored.GetWidth());
//             }
//         }
//     }
// }

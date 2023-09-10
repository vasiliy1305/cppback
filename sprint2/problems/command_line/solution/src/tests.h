#pragma once
#include "json_loader.h"
#include <iostream>
#include <cassert>

void TestLoadGame()
{
    const std::filesystem::path json_path("../../data/test_config.json");
    auto game = json_loader::LoadGame(json_path);
    auto maps = game.GetMaps();
    auto map = maps.at(0);
    assert(*map.GetId() == "map1");
    assert(map.GetName() == "Map 1");

    auto roads = map.GetRoads();
    assert(roads.size() == 4);

    assert(roads.at(0).GetStart().x == 0);
    assert(roads.at(0).GetStart().y == 0);
    assert(roads.at(0).GetEnd().x == 40);
    assert(roads.at(0).GetEnd().y == 0);
    assert(roads.at(0).IsHorizontal());
    assert(!roads.at(0).IsVertical());

    assert(roads.at(1).GetStart().x == 40);
    assert(roads.at(1).GetStart().y == 0);
    assert(roads.at(1).GetEnd().x == 40);
    assert(roads.at(1).GetEnd().y == 30);
    assert(!roads.at(1).IsHorizontal());
    assert(roads.at(1).IsVertical());

    assert(roads.at(2).GetStart().x == 40);
    assert(roads.at(2).GetStart().y == 30);
    assert(roads.at(2).GetEnd().x == 0);
    assert(roads.at(2).GetEnd().y == 30);
    assert(roads.at(2).IsHorizontal());
    assert(!roads.at(2).IsVertical());

    assert(roads.at(3).GetStart().x == 0);
    assert(roads.at(3).GetStart().y == 0);
    assert(roads.at(3).GetEnd().x == 0);
    assert(roads.at(3).GetEnd().y == 30);
    assert(!roads.at(3).IsHorizontal());
    assert(roads.at(3).IsVertical());

    auto buildings = map.GetBuildings();
    assert(buildings.size() == 1);
    assert(buildings.at(0).GetBounds().position.x == 5);
    assert(buildings.at(0).GetBounds().position.y == 5);
    assert(buildings.at(0).GetBounds().size.height == 20);
    assert(buildings.at(0).GetBounds().size.width == 30);

    auto offices = map.GetOffices();
    assert(offices.size() == 1);
    assert(*offices.at(0).GetId() == "o0");
    assert(offices.at(0).GetOffset().dx == 5);
    assert(offices.at(0).GetOffset().dy == 0);
    assert(offices.at(0).GetPosition().x == 40);
    assert(offices.at(0).GetPosition().y == 30);
}

void AllTests()
{
    TestLoadGame();
}

// 123

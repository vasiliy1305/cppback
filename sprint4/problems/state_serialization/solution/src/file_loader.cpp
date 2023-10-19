#include "file_loader.h"

void SaveGameToFile(const std::string &filename, const model::Game &game)
{
    std::ofstream ofs(filename);

    if (!ofs)
    {
        throw std::runtime_error("Error save game state");
    }

    boost::archive::text_oarchive oa(ofs);
    oa << game;
}

bool LoadGameFromFile(const std::string &filename, model::Game &game)
{
    std::ifstream ifs(filename);

    if (!ifs)
    {
        return false;
    }
    boost::archive::text_iarchive ia(ifs);
    ia >> game;
    return true;
}
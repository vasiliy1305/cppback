#include "file_loader.h"

bool SaveGameToFile(const std::string &filename, const model::Game &game)
{
    std::ofstream ofs(filename);

    if (!ofs)
    {
        std::cerr << "Could not open file for writing." << std::endl;
        return false;
    }

    boost::archive::text_oarchive oa(ofs);
    oa << game;
    return true;
}

bool LoadGameFromFile(const std::string &filename, model::Game &game)
{
    std::ifstream ifs(filename);

    if (!ifs)
    {
        std::cerr << "Could not open file for reading." << std::endl;
        return false;
    }

    boost::archive::text_iarchive ia(ifs);
    ia >> game;
    return true;
}
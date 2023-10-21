#include <fstream>

#include "model.h"
#include "json_loader.h"

using namespace model;
using namespace std::literals;

void SaveGameToFile(const std::string &filename, const model::Game &game);

bool LoadGameFromFile(const std::string &filename, model::Game &game);

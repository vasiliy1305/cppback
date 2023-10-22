#include "database.h"
#include <pqxx/zview.hxx>
#include <iostream>
#include "tagged_uuid.h"

using namespace std::literals;
using pqxx::operator"" _zv;

void CreateTable(pqxx::connection &connection)
{
    pqxx::work work{connection};
    try
    {
        work.exec(R"(
                    CREATE TABLE IF NOT EXISTS retired_players (
                        id UUID CONSTRAINT firstindex PRIMARY KEY,
                        name varchar(100) NOT NULL,
                        score int,
                        playTime int
                    );
                    )"_zv);
    }
    catch (...)
    {
        std::cout << "err\n";
    }
    work.exec(R"(
                    CREATE INDEX IF NOT EXISTS scores_rating ON
                    retired_players (score DESC, playtime, name)
                    ;
                )"_zv);

    work.commit();
}

void PutDogsToDb(pqxx::connection &conn, std::vector<model::DogStat> &dogs)
{
    pqxx::work work{conn};

    for (auto iter = dogs.begin(); iter != dogs.end(); iter++) // переделать
    {
        work.exec_params(
            R"(
                    INSERT INTO retired_players (id, name, score, playtime) VALUES ($1, $2, $3, $4)
                )"_zv,
            util::detail::UUIDToString(util::detail::NewUUID()), iter->name, iter->score, iter->time);

    }
    work.commit();
}

void ReadScores(pqxx::connection &conn, int offset, int limit, std::vector<model::DogStat> &dogs)
{
    pqxx::read_transaction read_trans(conn);
    auto query_text = "SELECT name, score, playtime FROM retired_players ORDER BY score DESC, playtime, name OFFSET " + read_trans.quote(offset) + " LIMIT " + read_trans.quote(limit) +
                      ";";
    for (auto [name, score, time] : read_trans.query<std::string, int, int>(query_text))
    {

        dogs.push_back({0, name, score, time}); // возможно id не нужен
    }
}

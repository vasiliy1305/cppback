#include "book_db.h"

void BookDb::CreateBookTable()
{
    w_.exec(
        "CREATE TABLE IF NOT EXISTS books (id SERIAL PRIMARY KEY, title varchar(100) NOT NULL, author varchar(100) NOT NULL ,year integer NOT NULL, ISBN char(13) UNIQUE);"_zv);
    w_.commit();
}

std::basic_string_view<char>  BookDb::AddBook(std::string title, std::string author, int year, std::string ISBN)
{

    pqxx::work add_trans(conn_);

    try
    {
        add_trans.exec_prepared(tag_add_book, title, author, year, ISBN);
        add_trans.commit();
        return result_true;
    }
    catch (...)
    {
        return result_false;
    }
}

std::basic_string_view<char>  BookDb::AddBook(std::string title, std::string author, int year)
{
    pqxx::work add_trans(conn_);

    try
    {
        add_trans.exec_prepared(tag_add_book_null_isbn, title, author, year);
        add_trans.commit();
        return result_true;
    }
    catch (...)
    {
        return result_false;
    }
}

std::string BookDb::AllBooks()
{
    pqxx::read_transaction read_trans(conn_);
    {
        auto query_text = "SELECT id, title, author, year, ISBN FROM books ORDER BY year DESC, title ASC, author ASC, ISBN ASC"_zv;

        boost::json::array arr;
        for (auto [id, title, author, year, ISBN] : read_trans.query<int, std::string, std::string, int, std::optional<std::string>>(query_text))
        {
            boost::json::object row;
            row["id"] = id;
            row["title"] = title;
            row["author"] = author;
            row["year"] = year;
            if (ISBN)
            {
                row["ISBN"] = *ISBN;
            }
            else
            {
                row["ISBN"] = nullptr;
            }

            arr.push_back(row);
        }
        boost::json::value jv(arr);
        std::string serialized_json = boost::json::serialize(jv);
        return serialized_json;
    }
}

void BookDb::PrepareTrans()
{
    conn_.prepare(tag_add_book_null_isbn, "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, NULL)"_zv);
    conn_.prepare(tag_add_book, "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, $4)"_zv);
}
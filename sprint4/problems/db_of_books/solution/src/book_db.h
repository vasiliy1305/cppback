#include <iostream>
#include <optional>
#include <pqxx/pqxx>
#include <boost/json.hpp>
#include <sstream>

using namespace std::literals;
using pqxx::operator"" _zv;

class BookDb
{
public:
    BookDb(std::string conn_url) : conn_(conn_url), w_(conn_)
    {
        // поправить именна
        conn_.prepare(tag_add_book, "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, $4)"_zv);
        conn_.prepare(tag_add_book_null_isbn, "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, NULL)"_zv);
    }

    void CreateBookTable();
    void AddBook(std::string title, std::string  author, int  year, std::string ISBN);
    void AddBook(std::string title, std::string  author, int  year);
    std::string AllBooks();

private:
    pqxx::connection conn_;
    pqxx::work w_;


    // todo поправить имена
    pqxx::zview tag_add_book = "add_book_trans"_zv;
    pqxx::zview tag_add_book_null_isbn = "add_book_null_isbn_trans"_zv;
    std::basic_string_view<char> result_ok = R"({"result":true})"sv;
    std::basic_string_view<char> result_not_ok = R"({"result":false})"sv;
};
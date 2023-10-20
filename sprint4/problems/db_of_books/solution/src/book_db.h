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
    }

    void CreateBookTable();

    void PrepareTrans();
    std::basic_string_view<char>  AddBook(std::string title, std::string author, int year, std::string ISBN);
    std::basic_string_view<char>  AddBook(std::string title, std::string author, int year);
    std::string AllBooks();

private:
    pqxx::connection conn_;
    pqxx::work w_;

    // todo поправить имена

    std::basic_string_view<char> result_ok = R"({"result":true})"sv;
    std::basic_string_view<char> result_not_ok = R"({"result":false})"sv;

    pqxx::zview tag_add_book_null_isbn;
    pqxx::zview tag_add_book;
};
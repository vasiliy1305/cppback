#include "book_db.h"

int main(int argc, const char *argv[])
{
    try
    {
        if (argc == 1)
        {
            std::cout << "Usage: connect_db <conn-string>\n"sv;
            return EXIT_SUCCESS;
        }
        else if (argc != 2)
        {
            std::cerr << "Invalid command line\n"sv;
            return EXIT_FAILURE;
        }

        BookDb book_db(argv[1]);
        book_db.CreateBookTable();
        book_db.PrepareTrans();
        std::string request;
        while (true)
        {
            std::getline(std::cin, request);
            auto value = boost::json::parse(request);
            std::string action = boost::json::value_to<std::string>(value.at("action")); // -> action
            if (action == "exit")
            {
                break;
            }
            if (action == "add_book")
            {
                auto payload = value.at("payload");
                auto title = boost::json::value_to<std::string>(payload.at("title"));
                auto author = boost::json::value_to<std::string>(payload.at("author"));
                auto year = boost::json::value_to<int>(payload.at("year"));

                if (!payload.at("ISBN").is_null())
                {
                    std::string isbn = boost::json::value_to<std::string>(payload.at("ISBN"));
                    book_db.AddBook(title, author, year, isbn);
                }
                else
                {
                    book_db.AddBook(title, author, year);
                }
            }
            if (action == "all_books")
            {
                std::cout << book_db.AllBooks() << std::endl;
            }
            request.clear();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
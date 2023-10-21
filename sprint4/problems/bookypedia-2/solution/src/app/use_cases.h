#pragma once

#include <string>
#include <vector>
#include <optional>

struct ShowBookInfo
{
    std::string title;
    std::string author_name;
    int publication_year;
    std::string book_id;
};

struct ShowSingleBook
{
    std::string title;
    std::string author_name;
    int publication_year;
    std::vector<std::string> tags;
};

namespace app
{
    class UseCases
    {
    public:
        virtual void AddAuthor(const std::string &name) = 0;
        virtual std::vector<std::pair<std::string, std::string>> ShowAuthors() = 0;
        virtual void SaveBook(std::string &title, std::string &author_id, int pub_year, std::optional<std::vector<std::string>> tags) = 0;
        virtual void SaveBook(std::string &author_name, std::string &title, std::string &author_id, int pub_year, std::optional<std::vector<std::string>> tags) = 0;
        virtual std::vector<std::pair<std::string, int>> ShowAuthorBooks(const std::string &author_id) = 0;
        virtual std::vector<ShowBookInfo> ShowBooks() = 0;
        virtual ShowSingleBook ShowBook(const std::string &name) = 0;
        virtual void DeleteAuthor(const std::string &author_id) = 0;
        virtual void DeleteBook(const std::string &book_id) = 0;
        virtual void EditAuthor(const std::string &author_id, const std::string &new_name) = 0;
        virtual void EditBook(const ShowSingleBook &book_data, const std::string &book_id) = 0;
        virtual std::string GetUUID() = 0;

    protected:
        ~UseCases() = default;
    };

} // namespace app

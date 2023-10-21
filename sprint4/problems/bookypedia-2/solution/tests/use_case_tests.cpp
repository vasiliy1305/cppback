#include <catch2/catch_test_macros.hpp>

#include "../src/app/use_cases_impl.h"
#include "../src/domain/author.h"

namespace {

struct MockAuthorRepository : domain::AuthorRepository {
    std::vector<domain::Author> saved_authors;

    void Save(const domain::Author& author) override {
        saved_authors.emplace_back(author);
    }

    std::vector<std::pair<std::string, std::string>> ShowAuthors() override {
        std::vector<std::pair<std::string, std::string>> vec = {};
        return vec;
    }

    void SaveBook(const domain::Book& book) override
    {
        ;
    }

    void SaveBook(const domain::Author& author, const domain::Book& book) override
    {
        ;
    }

    std::vector<std::pair<std::string, int>> ShowAuthorBooks(const std::string& author_id) override
    {
        std::vector<std::pair<std::string, int>> vec;
        return vec;
    }

    std::vector<ShowBookInfo> ShowBooks() override
    {
        std::vector<ShowBookInfo> vec = {};
        return vec;
    }

    ShowSingleBook ShowBook(const std::string& id)
    {
        return {};
    }

    void DeleteAuthor(const std::string& author_id) override
    {
        ;
    }

    void DeleteBook(const std::string& author_id) override
    {
        ;
    }

    void EditAuthor(const std::string& author_id, const std::string& new_name) override
    {
        ;
    }

    void EditBook(const ShowSingleBook& book_data, const std::string& book_id)
    {
        ;
    }

};

struct Fixture {
    MockAuthorRepository authors;
};

}  // namespace

SCENARIO_METHOD(Fixture, "Book Adding") {
    GIVEN("Use cases") {
        app::UseCasesImpl use_cases{authors};

        WHEN("Adding an author") {
            const auto author_name = "Joanne Rowling";
            use_cases.AddAuthor(author_name);

            THEN("author with the specified name is saved to repository") {
                REQUIRE(authors.saved_authors.size() == 1);
                CHECK(authors.saved_authors.at(0).GetName() == author_name);
                CHECK(authors.saved_authors.at(0).GetId() != domain::AuthorId{});
            }
        }
    }
}
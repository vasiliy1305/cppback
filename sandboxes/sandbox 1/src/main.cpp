#include <iostream>
#include <pqxx/pqxx>

using namespace std::literals;
// libpqxx использует zero-terminated символьные литералы вроде "abc"_zv;
using pqxx::operator"" _zv;

int main(int argc, const char* argv[]) {
    try {
        if (argc == 1) {
            std::cout << "Usage: connect_db <conn-string>\n"sv;
            return EXIT_SUCCESS;
        } else if (argc != 2) {
            std::cerr << "Invalid command line\n"sv;
            return EXIT_FAILURE;
        }

        // Подключаемся к БД, указывая её параметры в качестве аргумента
        pqxx::connection conn{argv[1]};

        // Создаём транзакцию. Это понятие будет разобрано в следующих уроках.
        // Транзакция нужна, чтобы выполнять запросы.
        pqxx::work w(conn);

        // Используя транзакцию создадим таблицу в выбранной базе данных:
        w.exec(
            "CREATE TABLE IF NOT EXISTS movies (id SERIAL PRIMARY KEY, title varchar(200) NOT NULL, year integer NOT NULL);"_zv);

        w.exec("DELETE FROM movies;"_zv);
        w.exec(
            "INSERT INTO movies (title, year) VALUES ('Trash', 2014), ('The Kid', 2000), "
            "('The Sting', 1973), ('The Terminal', 2004), ('Amarcord', 1973), ('The King''s Speech', 2010), "
            "('Det sjunde inseglet', 1957), ('Groundhog Day', 1993);"_zv);

        // Применяем все изменения
        w.commit();

        // ... Тут будем писать код чтения

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
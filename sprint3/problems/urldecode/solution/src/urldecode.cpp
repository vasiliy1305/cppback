#include "urldecode.h"

#include <charconv>
#include <stdexcept>

#include <iostream>
#include <string>
#include <sstream>
#include <cctype>

std::string UrlDecode(std::string_view str)
{
    std::ostringstream out;
    for (size_t i = 0; i < str.size(); ++i)
    {
        char ch = str[i];
        if (ch == '%')
        {
            // Проверяем, есть ли у нас достаточно символов для раскодирования
            if (i + 2 >= str.size())
            {
                throw std::invalid_argument("Некорректная %-последовательность");
            }

            // Пробуем преобразовать два следующих символа в число
            char hex[2];
            hex[0] = str[i + 1];
            hex[1] = str[i + 2];

            for (char c : hex)
            {

                if (!isxdigit(c))
                {
                    throw std::invalid_argument("Некорректная %-последовательность");
                }
            }

            int value;
            sscanf(hex, "%x", &value);
            out << static_cast<char>(value);
            i += 2;
        }
        else
        {
            out << ch;
        }
    }

    return out.str();
}

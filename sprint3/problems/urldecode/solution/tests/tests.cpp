#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(UrlDecode_tests)
{
    using namespace std::literals;

    // Позитивные тесты
    BOOST_TEST(UrlDecode(""sv) == ""s);
    BOOST_TEST(UrlDecode(R"(Hello%20World%20%21)") == "Hello World !"s);
    BOOST_TEST(UrlDecode("%21%40%23") == "!@#");
    BOOST_TEST(UrlDecode("A%2Bb%3Dc") == "A+b=c");
    BOOST_TEST(UrlDecode("%25") == "%");
    BOOST_TEST(UrlDecode("No%20spaces") == "No spaces");
    BOOST_TEST(UrlDecode("%21%23%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D") == "!#$&'()*+,/:;=?@[]");
    BOOST_TEST(UrlDecode("1234567890") == "1234567890");
}
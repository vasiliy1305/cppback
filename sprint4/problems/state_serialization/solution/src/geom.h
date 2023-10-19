#pragma once

#include <compare>

namespace geom
{

    struct Vec2D
    {
        Vec2D() = default;
        Vec2D(double x, double y)
            : x(x), y(y)
        {
        }

        Vec2D &operator*=(double scale)
        {
            x *= scale;
            y *= scale;
            return *this;
        }

        auto operator<=>(const Vec2D &) const = default;

        double x = 0;
        double y = 0;
    };

    inline Vec2D operator*(Vec2D lhs, double rhs)
    {
        return lhs *= rhs;
    }

    inline Vec2D operator*(double lhs, Vec2D rhs)
    {
        return rhs *= lhs;
    }

    struct Point2D
    {
        Point2D() = default;
        Point2D(double x, double y)
            : x(x), y(y)
        {
        }

        Point2D &operator+=(const Vec2D &rhs)
        {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }

        auto operator<=>(const Point2D &) const = default;

        double x = 0;
        double y = 0;
    };

    inline Point2D operator+(Point2D lhs, const Vec2D &rhs)
    {
        return lhs += rhs;
    }

    inline Point2D operator+(const Vec2D &lhs, Point2D rhs)
    {
        return rhs += lhs;
    }

    template <typename Archive>
    void serialize(Archive &ar, Point2D &point, [[maybe_unused]] const unsigned version)
    {
        ar & point.x;
        ar & point.y;
    }

    template <typename Archive>
    void serialize(Archive &ar, Vec2D &vec, [[maybe_unused]] const unsigned version)
    {
        ar & vec.x;
        ar & vec.y;
    }

} // namespace geom

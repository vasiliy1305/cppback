#define _USE_MATH_DEFINES

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

#include "../src/collision_detector.h"

#include <cmath>
#include <functional>
#include <sstream>

namespace Catch
{
    template <> // todo delite
    struct StringMaker<collision_detector::GatheringEvent>
    {
        static std::string convert(collision_detector::GatheringEvent const &value)
        {
            std::ostringstream resualt;
            resualt << "(" << value.gatherer_id << value.item_id << value.sq_distance << value.time << ")";
            return resualt.str();
        }
    };
} 

namespace
{
    template <typename Range, typename Predicate>
    struct RangeMatcher : Catch::Matchers::MatcherGenericBase
    {
        RangeMatcher(Range const &range, Predicate predicate)
            : range_{range}, predicate_{predicate}
        {
        }

        template <typename OtherRange>
        bool match(const OtherRange &other) const
        {
            using std::begin;
            using std::end;
            return std::equal(begin(range_), end(range_), begin(other), end(other), predicate_);
        }

        std::string describe() const override
        {
            return "Equals: " + Catch::rangeToString(range_);
        }

    private:
        const Range &range_;
        Predicate predicate_;
    };

    template <typename Range, typename Predicate>
    auto EqualsRange(const Range &range, Predicate prediate)
    {
        return RangeMatcher<Range, Predicate>{range, prediate};
    }

    class TestProvider : public collision_detector::ItemGathererProvider
    {
    public:
        TestProvider(std::vector<collision_detector::Item> items,
                                   std::vector<collision_detector::Gatherer> gatherers)
            : items_(items), gatherers_(gatherers)
        {
        }

        size_t ItemsCount() const override
        {
            return items_.size();
        }
        collision_detector::Item GetItem(size_t idx) const override
        {
            return items_.at(idx);
        }
        size_t GatherersCount() const override
        {
            return gatherers_.size();
        }
        collision_detector::Gatherer GetGatherer(size_t idx) const override
        {
            return gatherers_.at(idx);
        }

    private:
        std::vector<collision_detector::Item> items_;
        std::vector<collision_detector::Gatherer> gatherers_;
    };

    class CompareEvents
    {
    public:
        bool operator()(const collision_detector::GatheringEvent &l,
                        const collision_detector::GatheringEvent &r)
        {
            if (l.gatherer_id != r.gatherer_id || l.item_id != r.item_id)
                return false;

            static const double eps = 1e-10;

            if (std::abs(l.sq_distance - r.sq_distance) > eps)
            {
                return false;
            }

            if (std::abs(l.time - r.time) > eps)
            {
                return false;
            }
            return true;
        }
    };

}

SCENARIO("Colision identification")
{
    WHEN("empty list")
    {
        TestProvider provider{
            {}, {{{1, 2}, {4, 2}, 5.}, {{0, 0}, {10, 10}, 5.}, {{-5, 0}, {10, 5}, 5.}}};
        THEN("empty events")
        {
            auto events = collision_detector::FindGatherEvents(provider);
            CHECK(events.empty());
        }
    }
    WHEN("empty gatherers")
    {
        TestProvider provider{
            {{{1, 2}, 5.}, {{0, 0}, 5.}, {{-5, 0}, 5.}}, {}};
        THEN("empty events")
        {
            auto events = collision_detector::FindGatherEvents(provider);
            CHECK(events.empty());
        }
    }
    WHEN("Multiple items on a gatherers path")
    {
        TestProvider provider{{
                                                {{9, 0.27}, .1},
                                                {{8, 0.24}, .1},
                                                {{7, 0.21}, .1},
                                                {{6, 0.18}, .1},
                                                {{5, 0.15}, .1},
                                                {{4, 0.12}, .1},
                                                {{3, 0.09}, .1},
                                                {{2, 0.06}, .1},
                                                {{1, 0.03}, .1},
                                                {{0, 0.0}, .1},
                                                {{-1, 0}, .1},
                                            },
                                            {
                                                {{0, 0}, {10, 0}, 0.1},
                                            }};
        THEN("Items gathered in the correct order")
        {
            auto events = collision_detector::FindGatherEvents(provider);
            CHECK_THAT(
                events,
                EqualsRange(std::vector{
                                collision_detector::GatheringEvent{9, 0, 0. * 0., 0.0},
                                collision_detector::GatheringEvent{8, 0, 0.03 * 0.03, 0.1},
                                collision_detector::GatheringEvent{7, 0, 0.06 * 0.06, 0.2},
                                collision_detector::GatheringEvent{6, 0, 0.09 * 0.09, 0.3},
                                collision_detector::GatheringEvent{5, 0, 0.12 * 0.12, 0.4},
                                collision_detector::GatheringEvent{4, 0, 0.15 * 0.15, 0.5},
                                collision_detector::GatheringEvent{3, 0, 0.18 * 0.18, 0.6},
                            },
                            CompareEvents()));
        }
    }
    WHEN("Several gatherers and a single item")
    {
        TestProvider provider{{
                                                {{0, 0}, 0.},
                                            },
                                            {
                                                {{-5, 0}, {5, 0}, 1.},
                                                {{0, 1}, {0, -1}, 1.},
                                                {{-10, 10}, {101, -100}, 0.5}, // <-- that one
                                                {{-100, 100}, {10, -10}, 0.5},
                                            }};
        THEN("The item collected by the quicker gatherer")
        {
            auto events = collision_detector::FindGatherEvents(provider);
            CHECK(events.front().gatherer_id == 2);
        }
    }
    WHEN("Gatherers remain stationary")
    {
        TestProvider provider{{
                                                {{0, 0}, 10.},
                                            },
                                            {{{-5, 0}, {-5, 0}, 1.},
                                             {{0, 0}, {0, 0}, 1.},
                                             {{-10, 10}, {-10, 10}, 100}}};
        THEN("Zero events detected")
        {
            auto events = collision_detector::FindGatherEvents(provider);

            CHECK(events.empty());
        }
    }
}
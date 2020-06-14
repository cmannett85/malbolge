/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/virtual_memory.hpp"

#include "test_helpers.hpp"

using namespace malbolge;

BOOST_AUTO_TEST_SUITE(virtual_memory_suite)

BOOST_AUTO_TEST_CASE(constructor)
{
    auto f = [](const auto& v) {
        try {
            auto vmem = virtual_memory(v);
            BOOST_FAIL("Should have thrown");
        } catch (std::exception&) {}
    };

    BOOST_TEST_MESSAGE("Invalid constructors");
    test::data_set(
        f,
        {
            std::tuple{std::vector<int>{}},
            std::tuple{std::vector<int>{0}},
            std::tuple{std::vector<int>{42}},
            std::tuple{std::vector<int>(math::ternary::max+2)},
        }
    );

    // Check reversed interator arguments
    auto v = std::vector<int>{0, 3, 5, 6, 7, 1};
    try {
        auto vmem = virtual_memory(v.end(), v.begin());
        BOOST_FAIL("Should have thrown");
    } catch (std::exception&) {}

    BOOST_TEST_MESSAGE("Memory space filling");
    auto vmem = virtual_memory(v);
    for (auto i = v.size(); i < math::ternary::max; ++i) {
        const auto op = vmem[i-1].op(vmem[i-2]);
        BOOST_CHECK_EQUAL(vmem[i], op);
    }

    BOOST_TEST_MESSAGE("Move construction");
    const auto vmem2 = std::move(vmem);
    for (auto i = 0u; i < math::ternary::max; ++i) {
        if (i < v.size()) {
            BOOST_CHECK_EQUAL(vmem2[i], v[i]);
        } else {
            const auto op = vmem2[i-1].op(vmem2[i-2]);
            BOOST_CHECK_EQUAL(vmem2[i], op);
            BOOST_CHECK_EQUAL(vmem2.at(i), op);
        }
    }
}

BOOST_AUTO_TEST_CASE(constants)
{
    auto vmem = virtual_memory(std::vector<int>{0, 3, 5, 6, 7, 1});
    BOOST_CHECK(!vmem.empty());
    BOOST_CHECK_EQUAL(vmem.size(), math::ternary::max+1);
    BOOST_CHECK_EQUAL(vmem.size(), vmem.max_size());
}

BOOST_AUTO_TEST_CASE(iterator_arithmetic)
{
    auto vmem = virtual_memory(std::vector<int>{0, 3, 5, 6, 7, 1});

    BOOST_TEST_MESSAGE("In/decrement");
    auto it = vmem.begin();
    BOOST_CHECK_EQUAL(*it, vmem[0]);
    ++it;
    BOOST_CHECK_EQUAL(*it, vmem[1]);
    --it;
    BOOST_CHECK_EQUAL(*it, vmem[0]);
    it++;
    BOOST_CHECK_EQUAL(*it, vmem[1]);
    it--;
    BOOST_CHECK_EQUAL(*it, vmem[0]);
    it--;
    BOOST_CHECK_EQUAL(*it, vmem[math::ternary::max]);
    it++;
    BOOST_CHECK_EQUAL(*it, vmem[0]);

    it += 5;
    BOOST_CHECK_EQUAL(*it, vmem[5]);
    it -= 10;
    BOOST_CHECK_EQUAL(*it, vmem[math::ternary::max-4]);
    it += 5;
    BOOST_CHECK_EQUAL(*it, vmem[0]);

    {
        auto it2 = it + 5;
        BOOST_CHECK_EQUAL(*it2, vmem[5]);
        auto it3 = it2 - 10;
        BOOST_CHECK_EQUAL(*it3, vmem[math::ternary::max-4]);
        auto it4 = it3 + 5;
        BOOST_CHECK_EQUAL(*it4, vmem[0]);
    }
}

BOOST_AUTO_TEST_SUITE_END()

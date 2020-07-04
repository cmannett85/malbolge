/* Copyright Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/logging.hpp"

#define BOOST_TEST_MODULE malbolge_test Test Suite
#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>

int main(int argc, char* argv[])
{
    malbolge::logging::init_logging();

    return boost::unit_test::unit_test_main( &init_unit_test, argc, argv );
}

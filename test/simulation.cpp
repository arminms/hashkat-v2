////////////////////////////////////////////////////////////////////////////////
// This file is part of the #KAT Social Network Simulator.
//
// The #KAT Social Network Simulator is free software: you can redistribute it
// and/or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// The #KAT Social Network Simulator is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// the #KAT Social Network Simulator. If not, see http://www.gnu.org/licenses.
//
// Addendum:
//
// Under this license, derivations of the #KAT Social Network Simulator
// typically must be provided in source form. The #KAT Social Network Simulator
// and derivations thereof may be relicensed by decision of the original
// authors (Kevin Ryczko & Adam Domurad, Isaac Tamblyn), as well, in the case
// of a derivation, subsequent authors.

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <array>
#include <vector>
#include <unordered_set>
#include <random>
#include <numeric>
#include <limits>

// due to inclusion of <windows.h> by header only boost::test, we need
// the following define to prevent problem with std::numeric_limits
#   if defined(_MSC_VER)
#       define NOMINMAX
#   endif  //_MSC_VER

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>
#include <boost/test/detail/unit_test_parameters.hpp>

#include <boost/signals2.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "../include/network.h"
#include "../include/actions/twitter_add_agent.h"
#include "../include/actions/twitter_follow.h"
#include "../include/engine.h"
#include "../include/simulation.h"

using boost::test_tools::output_test_stream;
namespace butrc = boost::unit_test::runtime_config;
namespace pt = boost::property_tree;
using namespace hashkat;

typedef std::mt19937 test_rng;
typedef boost::property_tree::ptree test_config;
typedef network<dummy, test_config> test_network;
typedef engine
<
    test_network
,   dummy
,   test_config
,   test_rng
,   twitter_add_agent
,   twitter_follow
> test_engine;

typedef simulation
<
    test_network
,   dummy
,   test_config
,   test_engine
,   test_rng
> test_simulation;

struct FOLDERS
{
    FOLDERS()
#ifdef _MSC_VER
    :   cnf_folder(std::getenv("HASHKAT") ? std::getenv("HASHKAT") : "")
    {
        if (!cnf_folder.empty())
            cnf_folder += "/test/patterns/";
        else
            std::cout << "HASHKAT environment variable is not defined\n";
        ptn_folder = cnf_folder + "vc_";
#elif defined(__clang__)
    :   cnf_folder("patterns/")
    ,   ptn_folder("patterns/clang_")
    {
#else
    :   cnf_folder("patterns/")
    ,   ptn_folder("patterns/gcc_")
    {
#endif  // _MSC_VER
    }

    std::string cnf_folder;
    std::string ptn_folder;
};

BOOST_FIXTURE_TEST_CASE(Simulation_01, FOLDERS)
{
    test_config conf;
    pt::read_xml(cnf_folder + "config_01.xml", conf);
    test_simulation sim(conf);
    sim.run();
    //try
    //{
    //    sim.concurrent_run(1);
    //}
    //catch (const std::exception& e)
    //{
    //    std::cerr << "EXCEPTION: " << e.what() << std::endl;
    //}

    std::cout << "Elapsed time: " << sim.duration().count() << " ms" << std::endl;

    output_test_stream cout(
        ptn_folder + "sim_01.txt"
    ,   !butrc::save_pattern());
    cout << sim;
    BOOST_CHECK(cout.match_pattern());
}

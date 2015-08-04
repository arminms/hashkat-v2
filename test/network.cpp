///////////////////////////////////////////////////////////////////////////////
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
//
// build: g++ -std=c++11 network.cpp -o network

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <unordered_set>
#include <random>
#include <numeric>
#include <algorithm>

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>
#include <boost/test/detail/unit_test_parameters.hpp>

#include "../include/network.h"

using boost::test_tools::output_test_stream;
namespace butrc = boost::unit_test::runtime_config;

struct test_agent
{
    std::size_t id_;

    test_agent(std::size_t id = 0)
    :   id_(id)
    {}
};

struct INIT_NETWORK
{
    INIT_NETWORK()
    :   n(100)
#if (_MSC_VER >= 1800)
    ,   folder(std::getenv("HASHKAT"))
    {
        if (!folder.empty())
            folder += "/test/patterns/";
        else
            std::cout << "HASHKAT environment variable is not defined\n";
#else
    , folder("patterns/")
    {
#endif  // _WIN32
        std::default_random_engine dre;
        std::vector<std::size_t> v1(100);
        std::iota(v1.begin(), v1.end(), 0);
        //boost::iota(v1, 0);
        std::vector<std::size_t> v2(v1);
        std::shuffle(v1.begin(), v1.end(), dre);
        //boost::shuffle(v1, dre);
        std::shuffle(v2.begin(), v2.end(), dre);
        //boost::shuffle(v2, dre);

        n.initialize_bins(0, 100);
        n.grow(100);
        for (auto i = 0; i < v1.size(); ++i)
        {
            n[i].id_ = i;
            if (v1[i] != v2[i])
                n.connect(v1[i], v2[i]);
        }
    }

    network<test_agent> n;
    std::string folder;
};

BOOST_FIXTURE_TEST_CASE(Print, INIT_NETWORK)
{
    output_test_stream cout(
        folder + "network_01.txt"
    ,   !butrc::save_pattern());
    n.print(cout);
    BOOST_CHECK(cout.match_pattern());
}

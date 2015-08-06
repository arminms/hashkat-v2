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

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>
#include <boost/test/detail/unit_test_parameters.hpp>

#include "../include/network.h"

using boost::test_tools::output_test_stream;
namespace butrc = boost::unit_test::runtime_config;
using namespace hashkat;

struct test_agent
{
    std::size_t id_;

    test_agent(std::size_t id = 0)
    :   id_(id)
    {}
};

struct FOLDER
{
    FOLDER()
#ifdef _MSC_VER
    :   folder(std::getenv("HASHKAT"))
    {
        if (!folder.empty())
            folder += "/test/patterns/vc_";
        else
            std::cout << "HASHKAT environment variable is not defined\n";
#elif defined(__clang__)
    :   folder("patterns/clang_")
    {
#else
    :   folder("patterns/gcc_")
    {
#endif  // _MSC_VER
    }

    std::string folder;
};

struct INIT_NETWORK
{
    INIT_NETWORK()
    :   n(100)
#ifdef _MSC_VER
    ,   folder(std::getenv("HASHKAT"))
    {
        if (!folder.empty())
            folder += "/test/patterns/vc_";
        else
            std::cout << "HASHKAT environment variable is not defined\n";
#elif defined(__clang__)
    , folder("patterns/clang_")
    {
#else
    , folder("patterns/gcc_")
    {
#endif  // _MSC_VER
        n.initialize_bins(0, 100, 1);
        n.grow(100);

        std::mt19937 gen(333);
        std::uniform_int_distribution<int> di(0, 99);
        for (auto i = 0; i < 1000; ++i)
        {
            if (i < 100)
                n[i].id_ = i;
            auto followed = di(gen), follower = di(gen);
            if (followed != follower && !n.have_connection(followed, follower))
                n.connect(followed, follower);
        }
    }

    network<test_agent> n;
    std::string folder;
};

BOOST_FIXTURE_TEST_CASE(Range_Based_Loop, INIT_NETWORK)
{
    output_test_stream cout(
        folder + "network_01.txt"
    ,   !butrc::save_pattern());
    for (auto agent : n)
        cout << agent.id_ << ',';
    BOOST_CHECK(cout.match_pattern());
}

BOOST_AUTO_TEST_CASE(Connection)
{
    network<test_agent> n(2);
    n.initialize_bins(0, 2);
    n.grow(2);

    BOOST_CHECK(!n.have_connection(0, 1));
    BOOST_CHECK(!n.have_connection(1, 0));

    n.connect(0, 1);
    BOOST_CHECK(n.have_connection(0, 1));
    BOOST_CHECK(!n.have_connection(1, 0));

    n.connect(1, 0);
    BOOST_CHECK(n.have_connection(0, 1));
    BOOST_CHECK(n.have_connection(1, 0));

    n.disconnect(0, 1);
    BOOST_CHECK(!n.have_connection(0, 1));
    BOOST_CHECK(n.have_connection(1, 0));

    n.disconnect(1, 0);
    BOOST_CHECK(!n.have_connection(0, 1));
    BOOST_CHECK(!n.have_connection(1, 0));
}

BOOST_FIXTURE_TEST_CASE(Print, INIT_NETWORK)
{
    output_test_stream cout(
        folder + "network_02.txt"
    ,   !butrc::save_pattern());
    cout << n;
    BOOST_CHECK(cout.match_pattern());
}

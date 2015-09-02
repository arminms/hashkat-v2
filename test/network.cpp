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

#include <boost/signals2.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "../include/network_st.hpp"

using boost::test_tools::output_test_stream;
namespace butrc = boost::unit_test::runtime_config;
using namespace hashkat;

struct test_conf
{};

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
    :   folder(std::getenv("HASHKAT") ? std::getenv("HASHKAT") : "")
    {
        if (!folder.empty())
            folder += "/test/patterns/";
        else
            folder = "patterns/";
#else
    :   folder("patterns/")
    {
#endif  // _MSC_VER
    }

    std::string folder;
};

BOOST_FIXTURE_TEST_CASE(Config_Constructor, FOLDER)
{
    typedef boost::property_tree::ptree config;
    config conf;
    boost::property_tree::read_xml(folder + "network_config.xml", conf);
    network_st<test_agent, config> n(conf);

    BOOST_CHECK_EQUAL(n.max_size(), 10000);
}

BOOST_FIXTURE_TEST_CASE(Range_Based_Loop, FOLDER)
{
    output_test_stream cout(
        folder + "network_01.txt"
    ,   !butrc::save_pattern());

    network_st<test_agent, test_conf> n(10);
    n.grow(10);
    for (auto i = 0; i < n.size(); ++i)
        n[i].id_ = i;

    for (auto agent : n)
        cout << agent.id_ << ',';
    BOOST_CHECK(cout.match_pattern());
}

BOOST_AUTO_TEST_CASE(Connection)
{
    network_st<test_agent, test_conf> n(2);
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

BOOST_FIXTURE_TEST_CASE(Print, FOLDER)
{
    output_test_stream cout(
        folder + "network_02.txt"
    ,   !butrc::save_pattern());

    network_st<test_agent, test_conf> n(100);
    n.grow(100);

    std::vector<std::size_t> v1(100);
    std::iota(v1.begin(), v1.end(), 0);
    std::vector<std::size_t> v2(v1.size());
    std::rotate_copy(v1.begin(), v1.begin() + 50, v1.end(), v2.begin());

    for (auto i = 0; i < v1.size(); ++i)
        if (v1[i] != v2[i] && !n.have_connection(v1[i], v2[i]))
            n.connect(v1[i], v2[i]);

    cout << n;
    BOOST_CHECK(cout.match_pattern());
}

//BOOST_FIXTURE_TEST_CASE(Bins_10_10_1, FOLDER)
//{
//    output_test_stream cout(
//        folder + "network_03.txt"
//    ,   !butrc::save_pattern());
//
//    network_st<test_agent> n(10);
//    n.grow(10);
//
//    std::mt19937 gen(333);
//    std::uniform_int_distribution<int> di(0, 9);
//    for (auto i = 0; i < 25; ++i)
//    {
//        auto followee = di(gen), follower = di(gen);
//        if (followee != follower && !n.have_connection(followee, follower))
//            n.connect(followee, follower);
//    }
//
//    cout << n;
//    BOOST_CHECK(cout.match_pattern());
//}
//
//BOOST_FIXTURE_TEST_CASE(Bins_20_10_1, FOLDER)
//{
//    output_test_stream cout(
//        folder + "network_04.txt"
//    ,   !butrc::save_pattern());
//
//    network_st<test_agent> n(20);
//    n.grow(10);
//
//    std::mt19937 gen(333);
//    std::uniform_int_distribution<int> di(0, 9);
//    for (auto i = 0; i < 25; ++i)
//    {
//        auto followee = di(gen), follower = di(gen);
//        if (followee != follower && !n.have_connection(followee, follower))
//            n.connect(followee, follower);
//    }
//
//    cout << n;
//    BOOST_CHECK(cout.match_pattern());
//}
//
//BOOST_FIXTURE_TEST_CASE(Bins_10_10_2, FOLDER)
//{
//    output_test_stream cout(
//        folder + "network_05.txt"
//    ,   !butrc::save_pattern());
//
//    network_st<test_agent> n(10);
//    n.grow(10);
//
//    std::mt19937 gen(333);
//    std::uniform_int_distribution<int> di(0, 9);
//    for (auto i = 0; i < 25; ++i)
//    {
//        auto followee = di(gen), follower = di(gen);
//        if (followee != follower && !n.have_connection(followee, follower))
//            n.connect(followee, follower);
//    }
//
//    cout << n;
//    BOOST_CHECK(cout.match_pattern());
//}
//
//BOOST_FIXTURE_TEST_CASE(Bins_10_10_3, FOLDER)
//{
//    output_test_stream cout(
//        folder + "network_06.txt"
//    ,   !butrc::save_pattern());
//
//    network_st<test_agent> n(10);
//    n.grow(10);
//
//    std::mt19937 gen(333);
//    std::uniform_int_distribution<int> di(0, 9);
//    for (auto i = 0; i < 25; ++i)
//    {
//        auto followee = di(gen), follower = di(gen);
//        if (followee != follower && !n.have_connection(followee, follower))
//            n.connect(followee, follower);
//    }
//
//    cout << n;
//    BOOST_CHECK(cout.match_pattern());
//}
//
//BOOST_FIXTURE_TEST_CASE(Bins_21_10_3, FOLDER)
//{
//    output_test_stream cout(
//        folder + "network_07.txt"
//    ,   !butrc::save_pattern());
//
//    network_st<test_agent> n(21);
//    n.grow(10);
//
//    std::mt19937 gen(333);
//    std::uniform_int_distribution<int> di(0, 9);
//    for (auto i = 0; i < 25; ++i)
//    {
//        auto followee = di(gen), follower = di(gen);
//        if (followee != follower && !n.have_connection(followee, follower))
//            n.connect(followee, follower);
//    }
//
//    cout << n;
//    BOOST_CHECK(cout.match_pattern());
//}
//
//BOOST_FIXTURE_TEST_CASE(Bins_10_10_2_1_2, FOLDER)
//{
//    output_test_stream cout(
//        folder + "network_08.txt"
//    ,   !butrc::save_pattern());
//
//    network_st<test_agent> n(10);
//    n.grow(10);
//
//    std::mt19937 gen(333);
//    std::uniform_int_distribution<int> di(0, 9);
//    for (auto i = 0; i < 25; ++i)
//    {
//        auto followee = di(gen), follower = di(gen);
//        if (followee != follower && !n.have_connection(followee, follower))
//            n.connect(followee, follower);
//    }
//
//    cout << n;
//    BOOST_CHECK(cout.match_pattern());
//}
//
//BOOST_FIXTURE_TEST_CASE(Bins_10_10_1_2, FOLDER)
//{
//    output_test_stream cout(
//        folder + "network_09.txt"
//    ,   !butrc::save_pattern());
//
//    network_st<test_agent> n(10);
//    n.grow(10);
//
//    std::mt19937 gen(333);
//    std::uniform_int_distribution<int> di(0, 9);
//    for (auto i = 0; i < 25; ++i)
//    {
//        auto followee = di(gen), follower = di(gen);
//        if (followee != follower && !n.have_connection(followee, follower))
//            n.connect(followee, follower);
//    }
//
//    cout << n;
//    BOOST_CHECK(cout.match_pattern());
//}

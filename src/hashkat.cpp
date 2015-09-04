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

#include <boost/signals2.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "../include/network_st.hpp"
#include "../include/actions/twitter_add_agent_st.hpp"
#include "../include/actions/twitter_follow_st.hpp"
#include "../include/engine_st.hpp"
#include "../include/simulation_st.hpp"

namespace pt = boost::property_tree;
using namespace boost::program_options;
using namespace hashkat;

struct dummy
{};

typedef std::mt19937 hk_rng;
typedef boost::property_tree::ptree hk_config;
typedef network_st<dummy, hk_config> hk_network;
typedef engine_st
<
    hk_network
,   dummy
,   hk_config
,   hk_rng
,   twitter_add_agent_st
,   twitter_follow_st
> kmc_engine;

typedef simulation_st
<
    hk_network
,   dummy
,   hk_config
,   kmc_engine
,   hk_rng
> hk_simulation;

int main(int argc, char* argv[])
{
    std::string config_file = "config.xml";
    std::string output_file = "out.dat";
    bool show_duration;

    options_description visible(
        "Usage: hashkat [OPTIONS] [[-c|--config-file] CONFIG]"
        " [[-o|--output-file] OUTPUT]\n\nAllowed options");
    visible.add_options()
    ("help,h", "display this help and exit")
    ("version,v", "output version information and exit")
    ("show-duration,t", value<bool>(&show_duration)->default_value(true), "show duration");

    options_description hidden("Hidden options");
    hidden.add_options()
    ("config-file,c", value<std::string>(&config_file), "config file")
    ("output-file,o", value<std::string>(&output_file), "output file");

    positional_options_description p;
    p.add("config-file", 1);
    p.add("output-file", 1);

    options_description all("Allowed options");
    all.add(visible).add(hidden);

    variables_map vm;

    try
    {
        store(command_line_parser(argc, argv).options(all).positional(p).run(), vm);

        if (vm.count("version"))
        {
            std::cout << "hashkat version 0.1.0.0" << std::endl;
            return 0;
        }

        if (vm.count("help"))
        {
            std::cout << visible << std::endl;
            return 0;
        }

        // having notify after -v and -h options...
        notify(vm);

        hk_config conf;
        pt::read_xml(config_file, conf);
        hk_simulation sim(conf);
        sim.run();
        if (show_duration)
            std::cout << "Elapsed time: " << sim.duration().count()
                      << " ms" << std::endl;
        std::cout << "Saving output to " << output_file << " ...\n";
        std::ofstream out(output_file, std::ofstream::out);
        out << sim;
        std::cout << "Done!\n";
    }
    catch (invalid_command_line_syntax& e)
    {
        std::cout << visible << std::endl;
        std::cout << "Invalid command line syntax!" << std::endl;
        return 1;
    }
    catch (unknown_option& e)
    {
        std::cout << visible << std::endl;
        std::cout << "Unrecognized option!" << std::endl;
        return 1;
    }
    catch (too_many_positional_options_error& e)
    {
        std::cout << visible << std::endl;
        std::cout << "Too many output files!" << std::endl;
        return 1;
    }
    catch (invalid_option_value& e)
    {
        std::cout << visible << std::endl;
        std::cout << "Invalid option value!" << std::endl;
        return 1;
    }
    catch (std::exception& e)
    {
        std::cout << visible << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}

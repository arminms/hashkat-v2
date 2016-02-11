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

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <hashkat/hashkat.hpp>

#include "rss.hpp"

#define UNREFERENCED_PARAMETER(P) (P)

using namespace boost::program_options;
using namespace boost::filesystem;
using namespace hashkat;

struct dummy
{};

typedef std::mt19937 rng;
typedef network_st<dummy, configuration> network;
typedef engine_st
<
    network
,   dummy
,   configuration
,   rng
,   twitter_add_agent_st
,   twitter_follow_st
> kmc_engine;

typedef simulation_st
<
    network
,   dummy
,   configuration
,   kmc_engine
,   rng
> simulation;

int main(int argc, char* argv[])
{
    typedef std::chrono::high_resolution_clock seed_clock;
    seed_clock::time_point start = seed_clock::now();
    std::string input_file = "INFILE.xml";
    std::string output_folder = "output";
    std::string seed;
    unsigned seed_value = 1;

    options_description visible(
        "usage: hashkat [ options ]\n"
        "               [ [-i|--input-file] INFILE.xml ]\n"
        "               [ [-o|--output-folder] output ]\n\n"
        "Allowed options");
    visible.add_options()
    ("help,h", "display this help and exit")
    ("version,v", "output version information and exit")
    ("seed,r", value<std::string>(&seed),
        "seed for PRNG, =random for a true RNG")
    ("silent,s", "switch to silent mode");

    options_description hidden("Hidden options");
    hidden.add_options()
    ("input-file,i", value<std::string>(&input_file), "input file")
    ("output-folder,o", value<std::string>(&output_folder), "output folder");

    positional_options_description p;
    p.add("input-file", 1);
    p.add("output-folder", 1);

    options_description all("Allowed options");
    all.add(visible).add(hidden);

    variables_map vm;

    try
    {
        store(command_line_parser(argc, argv).options(all).positional(p).run()
        ,     vm);

        if (vm.count("version"))
        {
            std::cout << "Hashkat version 2.0" << std::endl;
            return 0;
        }

        if (vm.count("help"))
        {
            std::cout << visible << std::endl;
            return 0;
        }

        // having notify after -v and -h options...
        notify(vm);

         // creating output folder if it doesn't exist
        path p(output_folder);
        if (exists(p))
        {
            if (is_regular_file(p))
            {
                std::cout << "Error: " << output_folder << " is a file. "
                          << "Need a directory for --output-folder option."
                          << std::endl;
                return 0;
            }
        }
        else
            create_directories(p);

        // showing initial information
        if (!vm.count("silent"))
            std::cout << "Starting #k@ network simulator (version 2.0)\n"
                      << "Loading input configuration from '"
                      << input_file << "'." << std::endl;

        // acttually reading the configuration file
        configuration conf;
        config::read_xml(input_file, conf);
        conf.add("output_folder", output_folder);

        // constructing simulation object using conf
        simulation sim(conf);

        // setting the seed after reading the config and
        // making simulation object for having better true RNG
        if (vm.count("seed"))
        {
            if (boost::iequals(seed, "random"))
            {
                seed_clock::duration d = seed_clock::now() - start;
                seed_value = unsigned(d.count());
            }
            else
                seed_value = std::stoul(seed);
        }

        // showing seed information
        if (!vm.count("silent"))
            std::cout << "Starting simulation with seed '"
                      << seed_value << "'." << std::endl;
        sim.rng().seed(seed_value);

        std::cout << "Peak memory used: " 
                  << bytes2size(get_peak_rss())
                  << std::endl;

        // running simulation
        sim.run();

        // done, showing more information
        if (!vm.count("silent"))
            std::cout << "Simulation Completed: desired simulation time reached"
                      << std::endl
                      << "Elapsed time: " << sim.duration().count()
                      << " ms" << std::endl
                      << "Peak memory used: " 
                      << bytes2size(get_peak_rss()) << std::endl
                      << "Creating analysis files in: "
                      << output_folder << std::endl;

        // actually saving the output
        std::ofstream out(output_folder + "/out.dat", std::ofstream::out);
        out << sim;
        sim.dump(output_folder);

        if (!vm.count("silent"))
        {
            std::cout << "Peak memory used: " << bytes2size(get_peak_rss())
                      << std::endl;
            std::cout << "Done!" << std::endl;
        }

        sim.reset();
        std::cout << "Current memory used after reset: "
                  << bytes2size(get_current_rss())
                  << std::endl;	
    }
    catch (invalid_command_line_syntax& e)
    {
        UNREFERENCED_PARAMETER(e);
        std::cout << visible << std::endl;
        std::cout << "Invalid command line syntax!" << std::endl;
    }
    catch (unknown_option& e)
    {
        UNREFERENCED_PARAMETER(e);
        std::cout << visible << std::endl;
        std::cout << "Unrecognized option!" << std::endl;
    }
    catch (too_many_positional_options_error& e)
    {
        UNREFERENCED_PARAMETER(e);
        std::cout << visible << std::endl;
        std::cout << "Too many output files!" << std::endl;
    }
    catch (invalid_option_value& e)
    {
        UNREFERENCED_PARAMETER(e);
        std::cout << visible << std::endl;
        std::cout << "Invalid option value!" << std::endl;
    }
    catch (std::invalid_argument& e)
    {
        UNREFERENCED_PARAMETER(e);
        std::cout << visible << std::endl;
        std::cout << "Need a number as random seed!" << std::endl;
    }
    catch (filesystem_error& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

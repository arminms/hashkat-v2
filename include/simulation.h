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

#ifndef HASHKAT_SIMULATION_H_
#define HASHKAT_SIMULATION_H_

#include <chrono>
#include <thread>

#   ifdef _CONCURRENT
#       include <tbb/concurrent_queue.h>
#   endif //_CONCURRENT

namespace hashkat {

////////////////////////////////////////////////////////////////////////////////
// simulation class

template
<
    class NetworkType
,   class ContentsType
,   class ConfigType
,   class EngineType
,   class RngType
>
class simulation
{
    typedef simulation
        <NetworkType, ContentsType, ConfigType, EngineType, RngType> self_type;

public:
    simulation(const ConfigType& cnf)
    :   cnf_(cnf)
    ,   net_(cnf_)
    ,   cnt_()
    ,   eng_(net_, cnt_, cnf_, rng_)
    ,   max_time_(cnf.template get<int>("hashkat.network.max_time", 1000))
    ,   max_real_time_(cnf.template get<int>("hashkat.network.max_real_time", 1))
    {}

    bool run()
    {
        start_tp_ = std::chrono::high_resolution_clock::now();
        while (eng_.time() < max_time_ && duration() < max_real_time_)
            (*eng_())();
        return true;
    }

    bool concurrent_run(int n)
    {
#   ifdef _CONCURRENT
        if (n < 1)
            n = 1;
        start_tp_ = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> threads(n - 1);
        for (auto& thread : threads)
            thread = std::thread(&self_type::action_loop, this);
        action_loop();
        for (auto& thread : threads)
            thread.join();

        return true;
#   else
#       if BOOST_WORKAROUND(__GLIBCXX__, BOOST_TESTED_AT(20130909))
        return run();
#       else 
        BOOST_STATIC_ASSERT_MSG(false,
            "calling concurrent_run() requires _CONCURRENT to be defined :(");
#       endif //__GLIBCXX__
#   endif //_CONCURRENT
    }

    std::chrono::milliseconds duration() const
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start_tp_);
    }

    std::ostream& print(std::ostream& out) const
    {
        out << eng_;
        return out;
    }

private:
#   ifdef _CONCURRENT
    void action_loop()
    {
        try
        {
            while (eng_.time() < max_time_ && duration() < max_real_time_)
            {
                typename EngineType::action_type* action;
                if (actions_q_.try_pop(action))
                    (*action)();
                else
                    actions_q_.push(eng_());
            };
        }
        catch (const std::exception& e)
        {
            std::cerr << "THREAD-EXCEPTION (thread "
                      << std::this_thread::get_id()
                      << "): " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "THREAD-EXCEPTION (thread "
                      << std::this_thread::get_id() << ")" << std::endl;
        }
    }
#   endif //_CONCURRENT

    // member variables
    RngType      rng_;
    ConfigType   cnf_;
    NetworkType  net_;
    ContentsType cnt_;
    EngineType   eng_;
    typename EngineType::time_type max_time_;
    std::chrono::minutes max_real_time_;
    std::chrono::high_resolution_clock::time_point start_tp_;
#   ifdef _CONCURRENT
    tbb::concurrent_queue<typename EngineType::action_type*> actions_q_;
#   endif //_CONCURRENT
};

template
<
    class NetworkType
,   class ContentsType
,   class ConfigType
,   class EngineType
,   class RngType
>
std::ostream& operator<< (
    std::ostream& out
,   const simulation
        <NetworkType, ContentsType, ConfigType, EngineType, RngType>& s)
{
    return s.print(out);
}

}    // namespace hashkat

#endif  // HASHKAT_SIMULATION_H_

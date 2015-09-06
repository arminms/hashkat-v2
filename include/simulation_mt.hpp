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

#ifndef HASHKAT_SIMULATION_MT_HPP_
#define HASHKAT_SIMULATION_MT_HPP_

#include <chrono>
#include <thread>

#include <tbb/concurrent_queue.h>

namespace hashkat {

////////////////////////////////////////////////////////////////////////////////
// simulation_mt class

template
<
    class NetworkType
,   class ContentsType
,   class ConfigType
,   class EngineType
,   class RngType
>
class simulation_mt
{
    typedef simulation_mt
        <NetworkType, ContentsType, ConfigType, EngineType, RngType> self_type;

public:
    simulation_mt(const ConfigType& cnf)
    :   cnf_(cnf)
    ,   net_(cnf_)
    ,   cnt_()
    ,   eng_(net_, cnt_, cnf_, rng_)
    ,   nt_(0)
    ,   max_time_(cnf.template get<double>
            ("hashkat.network.max_time", 10))
    ,   max_real_time_(cnf.template get<unsigned>
            ("hashkat.network.max_real_time", 1))
    {}

    void reset()
    {
        net_.reset();
        //cnt_.reset();
        eng_.reset();
    }

    RngType& rng()
    {   return rng_;    }

    bool run(unsigned n = 0)
    {
        if (0 == n)
            n = std::thread::hardware_concurrency();
        nt_ = n;
        start_tp_ = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> threads(n - 1);
        for (auto& thread : threads)
            thread = std::thread(&self_type::action_loop, this);
        action_loop();
        for (auto& thread : threads)
            thread.join();

        return true;
    }

    std::chrono::milliseconds duration() const
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start_tp_);
    }

    std::ostream& print(std::ostream& out) const
    {
        out << "# Elapsed time: " << duration().count() << " ms" << std::endl;
        out << "# " << nt_ << " out of " << std::thread::hardware_concurrency()
            << " concurrent threads used\n";
        out << eng_ << net_;
        return out;
    }

private:
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

    // member variables
    RngType      rng_;
    ConfigType   cnf_;
    NetworkType  net_;
    ContentsType cnt_;
    EngineType   eng_;
    unsigned nt_;
    typename EngineType::time_type max_time_;
    std::chrono::minutes max_real_time_;
    std::chrono::high_resolution_clock::time_point start_tp_;
    tbb::concurrent_queue<typename EngineType::action_type*> actions_q_;
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
,   const simulation_mt
        <NetworkType, ContentsType, ConfigType, EngineType, RngType>& s)
{
    return s.print(out);
}

}    // namespace hashkat

#endif  // HASHKAT_SIMULATION_MT_HPP_

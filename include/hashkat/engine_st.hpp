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

#ifndef HASHKAT_ENGINE_ST_HPP_
#define HASHKAT_ENGINE_ST_HPP_

#include <vector>
#include <memory>
#include <chrono>

#ifndef HASHKAT_ACTION_ST_HPP_
#   include "action_st.hpp"
#endif // HASHKAT_ACTION_ST_HPP_

namespace hashkat {

////////////////////////////////////////////////////////////////////////////////
// action_depot class for simulations

template
<
    class Nwt   // NetworkType
,   class Ctt   // ContentsType
,   class Cft   // ConfigType
,   class Rgt   // RngType
,   class Tmt   // TimeType
,   template <class,class,class,class,class> class ...Act     // ActionType
>
struct action_depot
{
    action_depot()
    {   push_back(new Act<Nwt,Ctt,Cft,Rgt,Tmt>...);   }

    std::vector<std::unique_ptr<action_base<Nwt,Ctt,Cft,Rgt,Tmt>>> depot_;

private:
    template <typename T>
    void push_back(T* t)
    {
        depot_.push_back(
            std::unique_ptr<action_base<Nwt,Ctt,Cft,Rgt,Tmt>>(t));
    }

    template<typename First, typename ...Rest>
    void push_back(First* first, Rest* ...rest)
    {
        depot_.push_back(
            std::unique_ptr<action_base<Nwt,Ctt,Cft,Rgt,Tmt>>(first));
        push_back(rest...);
    }
};

////////////////////////////////////////////////////////////////////////////////
// engine_st class for simulations

template
<
    class Nwt   // NetworkType
,   class Ctt   // ContentsType
,   class Cft   // ConfigType
,   class Rgt   // RngType
,   template <class,class,class,class,class> class ...Act     // ActionType
>
class engine_st
{
public:
    typedef std::chrono::duration<double,std::ratio<60>> time_type;
    typedef engine_st<Nwt,Ctt,Cft,Rgt,Act...> self_type;
    typedef action_base<Nwt,Ctt,Cft,Rgt,time_type> action_type;
    typedef typename Nwt::rate_type rate_type;

    engine_st(
        Nwt& net
    ,   Ctt& cnt
    ,   Cft& cnf
    ,   Rgt& rng)
    :   net_(net)
    ,   cnt_(cnt)
    ,   cnf_(cnf)
    ,   rng_(rng)
    ,   time_(0)
    ,   n_steps_(0)
    ,   event_rate_(0)
    ,   random_time_increment_(cnf.template get<bool>
            ("analysis.use_random_time_increment", true))
    {
        for (auto& action : actions_.depot_)
        {
            action->init(net_, cnt_, cnf_, rng_, time_);
            action->happened().connect(
                boost::bind(&self_type::update_event_rate, this));
            action->finished().connect(
                boost::bind(&self_type::step_time, this));
        }
        for (auto& action : actions_.depot_)
            action->post_init();
    }

    void reset()
    {
        time_ = time_type::zero();
        n_steps_ = 0;
        event_rate_ = 0;
        for (auto& action : actions_.depot_)
            action->reset();
    }

    std::size_t steps() const
    {   return n_steps_;   }

    time_type time() const
    {   return time_;   }

    action_type* operator()()
    {
        typedef typename Nwt::type T;
        std::vector<typename action_type::weight_type> weights;
        weights.reserve(actions_.depot_.size());
        for (auto& action : actions_.depot_)
        {
            action->update_weight();
            weights.push_back(action->weight());
        }
        std::discrete_distribution<T> di(weights.begin(), weights.end());
        return actions_.depot_[di(rng_)].get();
    }

    std::ostream& print(std::ostream& out) const
    {
        out << "# Number of steps: " << n_steps_ << std::endl;
        out << "# Simulation time: " << time_.count() << " min" << std::endl;
        out << "# Event rate: " << event_rate_ << std::endl;
         for (auto& action : actions_.depot_)
            out << action.get();
        return out;
    }

    void dump(const std::string& folder) const
    {
        for (auto& action : actions_.depot_)
            action->dump(folder);
    }

private:
    void update_event_rate()
    {
        ++event_rate_;
    }

    void step_time()
    {
        ++n_steps_;

        double event_rate = 0;
        for (auto& action : actions_.depot_)
            event_rate += action->weight();

        if (random_time_increment_)
        {
            std::uniform_real_distribution<double> dr(std::nextafter(0, 1), 1);
            auto inc = time_type(-std::log(dr(rng_)) / event_rate);
            time_ += inc;
        }
        else
            time_ += time_type(1.0 / event_rate);
    }

    // member variables
    action_depot<Nwt,Ctt,Cft,Rgt,time_type,Act...> actions_;
    Nwt& net_;
    Ctt& cnt_;
    Cft& cnf_;
    Rgt& rng_;
    time_type time_;
    std::size_t n_steps_;
    std::size_t event_rate_;
    bool random_time_increment_;
};

template
<
    class Nwt
,   class Ctt
,   class Cft
,   class Rgt
,   template <class,class,class,class,class> class ...Act
>
std::ostream& operator<< (
    std::ostream& out
,   const engine_st<Nwt,Ctt,Cft,Rgt,Act...>& e)
{
    return e.print(out);
}

}    // namespace hashkat

#endif  // HASHKAT_ENGINE_ST_HPP_

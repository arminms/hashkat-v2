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

#ifndef HASHKAT_ENGINE_H_
#define HASHKAT_ENGINE_H_

#include <vector>
#include <memory>

#ifndef HASHKAT_ACTION_H_
#   include "action.h"
#endif // HASHKAT_ACTION_H_

namespace hashkat {

////////////////////////////////////////////////////////////////////////////////
// action_depot class for simulations

template
<
    class Nwt   // NetworkType
,   class Ctt   // ContentsType
,   class Cft   // ConfigType
,   class Rgt   // RngType
,   template <class,class,class,class> class ...Act     // ActionType
>
struct action_depot
{
    action_depot()
    {   push_back(new Act<Nwt,Ctt,Cft,Rgt>...);   }

    std::vector<std::unique_ptr<action_base<Nwt,Ctt,Cft,Rgt>>> actions_;

private:
    template <typename T>
    void push_back(T* t)
    {
        actions_.push_back(
            std::unique_ptr<action_base<Nwt,Ctt,Cft,Rgt>>(t));
    }

    template<typename First, typename ...Rest>
    void push_back(First* first, Rest* ...rest)
    {
        actions_.push_back(
            std::unique_ptr<action_base<Nwt,Ctt,Cft,Rgt>>(first));
        push_back(rest...);
    }
};

////////////////////////////////////////////////////////////////////////////////
// engine class for simulations

template
<
    class Nwt   // NetworkType
,   class Ctt   // ContentsType
,   class Cft   // ConfigType
,   class Rgt   // RngType
,   template <class,class,class,class> class ...Act     // ActionType
>
class engine
:   private boost::noncopyable
{
public:
    engine(
        Nwt& net
    ,   Ctt& cnt
    ,   Cft& cnf
    ,   Rgt& rng)
    :   net_(net)
    ,   cnt_(cnt)
    ,   cnf_(cnf)
    ,   rng_(rng)
    {
        for (auto action : actions_)
            action->init(net_, cnt_, cnf_, rng_);
    }

    action_base<Nwt,Ctt,Cft,Rgt>* operator()()
    {
        typedef typename Nwt::type T;
        std::vector<T> weights;
        weights.reserve(actions_.size());
        for (auto action : actions_)
            weights.push_back(action->rate());
        std::discrete_distribution<T> di(weights.begin(), weights.end());
        return *actions_[di(rng_)];
    }


private:
    static action_depot<Nwt,Ctt,Cft,Rgt,Act...> actions_;
    Nwt& net_;
    Ctt& cnt_;
    Cft& cnf_;
    Rgt& rng_;
};

template
<
    class Nwt
,   class Ctt
,   class Cft
,   class Rgt
,   template <class,class,class,class> class ...Act
>
action_depot<Nwt,Ctt,Cft,Rgt,Act...> engine<Nwt,Ctt,Cft,Rgt,Act...>::actions_;

}    // namespace hashkat

#endif  // HASHKAT_ENGINE_H_

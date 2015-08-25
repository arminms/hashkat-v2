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

#ifndef HASHKAT_ACTIONS_TWITTER_ADD_AGENT_H_
#define HASHKAT_ACTIONS_TWITTER_ADD_AGENT_H_

#ifndef HASHKAT_ACTION_H_
#   include "../action.h"
#endif // HASHKAT_ACTION_H_

namespace hashkat {

////////////////////////////////////////////////////////////////////////////////
// twitter_add_agent action class

template
<
    class NetworkType
,   class ContentsType
,   class ConfigType
,   class RngType
>
class twitter_add_agent
:   public action_base<NetworkType, ContentsType, ConfigType, RngType>
{
    typedef twitter_add_agent<NetworkType, ContentsType, ConfigType, RngType>
        self_type;
    typedef typename NetworkType::type T;
    typedef typename NetworkType::value_type V;

public:
    twitter_add_agent()
    :   action_base<NetworkType, ContentsType, ConfigType, RngType>()
    ,   net_ptr_(nullptr)
    ,   cnt_ptr_(nullptr)
    ,   cnf_ptr_(nullptr)
    ,   rng_ptr_(nullptr)
    ,   count_(0)
    {}

    twitter_add_agent(
        NetworkType& net
    ,   ContentsType& cnt
    ,   ConfigType& cnf
    ,   RngType& rng)
    :   action_base<NetworkType, ContentsType, ConfigType, RngType>()
    ,   net_ptr_(&net)
    ,   cnt_ptr_(&cnt)
    ,   cnf_ptr_(&cnf)
    ,   rng_ptr_(&rng)
    ,   count_(0)
    {}

private:
    virtual void do_init(
        NetworkType& net
    ,   ContentsType& cnt
    ,   ConfigType& cnf
    ,   RngType& rng)
    {
        net_ptr_ = &net;
        cnt_ptr_ = &cnt;
        cnf_ptr_ = &cnf;
        rng_ptr_ = &rng;

        rate_ = cnf_ptr_->template get<T>
            ("hashkat.rates.add", T(1));
    }

    virtual void do_post_init()
    {
        std::size_t ia = cnf_ptr_->template get<T>
            ("hashkat.network.initial_agents", T(0));
        for (auto i = 0; i < ia; ++i)
            (*this)();
    }

    virtual bool do_action()
    {
        if (net_ptr_->can_grow())
        {
            net_ptr_->grow();
            ++count_;
            // TODO - rate_ must be set based on network time
            return true;
        }
        else
        {
            rate_ = 0;
            return false;
        }
    }

    virtual std::ostream& do_print(std::ostream& out) const
    {
        out << "# Add count: " << count_ << std::endl;
        out << "# Add rate: " << rate_ << std::endl;
        return out;
    }

// member variables
    NetworkType* net_ptr_;
    ContentsType* cnt_ptr_;
    ConfigType* cnf_ptr_;
    RngType* rng_ptr_;
    T count_;
};

template
<
    class NetworkType
,   class ContentsType
,   class ConfigType
,   class RngType
>
std::ostream& operator<< (
    std::ostream& out
,   const twitter_add_agent<NetworkType, ContentsType, ConfigType, RngType>& aa)
{
    return aa.print(out);
}

}    // namespace hashkat

#endif  // HASHKAT_ACTIONS_TWITTER_ADD_AGENT_H_

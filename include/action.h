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

#ifndef HASHKAT_ACTION_H_
#define HASHKAT_ACTION_H_

//#include <boost/core/noncopyable.hpp>

namespace hashkat {

////////////////////////////////////////////////////////////////////////////////
// Abstract base class for actions

template
<
    class NetworkType
,   class ContentsType
,   class ConfigType
,   class RngType
>
class action_base
:   private boost::noncopyable
{
public:
    typedef typename NetworkType::rate_type rate_type;
    typedef typename NetworkType::rate_type weight_type;
    typedef boost::signals2::signal<void()> action_happened_signal_type;
    typedef boost::signals2::signal<void()> action_finished_signal_type;

    action_base()
    :   rate_(0)
    ,   weight_(0)
    {}

    void init(
        NetworkType& net
    ,   ContentsType& cnt
    ,   ConfigType& cnf
    ,   RngType& rng)
    {   do_init(net, cnt, cnf, rng);   }

    void post_init()
    {   do_post_init(); }

    rate_type rate() const
    {   return rate_;   }

    weight_type weight() const
    {   return weight_;   }

    void operator()()
    {   do_action();   }

    action_happened_signal_type& happened()
    {   return action_happened_signal_;   }

    action_finished_signal_type& finished()
    {   return action_finished_signal_;   }

    std::ostream& print(std::ostream& out) const
    {   return do_print(out); }

// Implementation
    virtual ~action_base() {};

protected:
    rate_type rate_;
    weight_type weight_;
    action_happened_signal_type action_happened_signal_;
    action_happened_signal_type action_finished_signal_;

private:
    virtual void do_init(
        NetworkType& net
    ,   ContentsType& cnt
    ,   ConfigType& cnf
    ,   RngType& rng) = 0;
    virtual void do_post_init() {};
    virtual void do_action() = 0;
    virtual std::ostream& do_print(std::ostream& out) const = 0;
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
,   const action_base<NetworkType, ContentsType, ConfigType, RngType>* ptr)
{
    return ptr->print(out);
}

}    // namespace hashkat

#endif  // HASHKAT_ACTION_H_

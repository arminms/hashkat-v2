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
public:
    twitter_add_agent(
        NetworkType& net
    ,   ContentsType& cnt
    ,   ConfigType& cnf
    ,   RngType& rng)
    :   action_base<NetworkType, ContentsType, ConfigType, RngType>()
    ,   net_(net)
    ,   cnt_(cnt)
    ,   cnf_(cnf)
    ,   rng_(rng)
    {}

private:
    NetworkType& net_;
    ContentsType& cnt_;
    ConfigType& cnf_;
    RngType& rng_;

    virtual bool do_action()
    {
        if (net_.size() == new_.max_size())
            return false;
        net_.grow();
        //rate_++;
    }
};

}    // namespace hashkat

#endif  // HASHKAT_ACTIONS_TWITTER_ADD_AGENT_H_

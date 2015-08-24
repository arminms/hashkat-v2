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
public:
    simulation(const ConfigType& conf)
    :   cnf_(conf)
    ,   net_(cnf_)
    ,   cnt_(cnf_)
    ,   eng_(net_, cnt_, cnf_, rng_)
    {}

    //bool run()
    //{
    //    while (true)
    //    {
    //        if (actions_q_.empty())
    //            actions_q_.push(eng_());
    //        else
    //        {
    //            actions_q_.front()();
    //            actions_q_.pop();
    //        }
    //    };
    //    return true;
    //}

    std::ostream& print(std::ostream& out) const
    {}

private:
    RngType      rng_;
    ConfigType   cnf_;
    NetworkType  net_;
    ContentsType cnt_;
    EngineType   eng_;
    //std::queue<decltype(eng_::operator()) actions_q_;
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

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

#include <boost/core/noncopyable.hpp>

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
    typedef typename NetworkType::type rate_type;

    action_base()
    :   rate_(0)
    {}

    rate_type rate() const
    {   return rate_;   }

    bool operator()()
    {   return do_action();   }


// Implementation
    virtual ~action_base() {};

protected:
    rate_type rate_;

private:
    virtual bool do_action() = 0;
};

}    // namespace hashkat

#endif  // HASHKAT_ACTION_H_

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

#ifndef HASHKAT_CONFIG_HPP_
#define HASHKAT_CONFIG_HPP_

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace hashkat {

// Custom translator for bool (only supports std::string)
struct bool_translator
{
    typedef std::string internal_type;
    typedef bool        external_type;

    // Converts a string to bool
    boost::optional<external_type> get_value(const internal_type& str)
    {
        if (!str.empty())
        {
            using boost::algorithm::iequals;

            if (iequals(str, "true") || iequals(str, "yes") || str == "1")
                return boost::optional<external_type>(true);
            else
                return boost::optional<external_type>(false);
        }
        else
            return boost::optional<external_type>(boost::none);
    }

    // Converts a bool to string
    boost::optional<internal_type> put_value(const external_type& b)
    {
        return boost::optional<internal_type>(b ? "true" : "false");
    }
};

typedef boost::property_tree::iptree configuration;

namespace config = boost::property_tree;

}    // namespace hashkat

namespace boost { namespace property_tree {

//template<typename Ch, typename Traits, typename Alloc> 
//struct translator_between<std::basic_string<Ch, Traits, Alloc>, bool>
template<> 
struct translator_between<std::string, bool>
{
    typedef hashkat::bool_translator type;
};

}} // namespace property_tree // namespace boost


#endif  // HASHKAT_CONFIG_HPP_

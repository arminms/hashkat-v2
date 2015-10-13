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

#ifndef HASHKAT_NETWORK_ST_HPP_
#define HASHKAT_NETWORK_ST_HPP_

#include <boost/range/adaptor/reversed.hpp>

namespace hashkat {

template
<
    class    AgentType
,   class    ConfigType
,   typename T = std::uint32_t
,   typename V = double
,   typename W = std::uint16_t
>
class network_st
{
public:
    typedef T type;
    typedef V rate_type;
    typedef V value_type;
    typedef W agent_type_index_type;
    //typedef AgentType agent_type;
    typedef ConfigType config_type;
    typedef network_st<AgentType, ConfigType, T, V, W> self_type;
    typedef boost::signals2::signal<void(T, W)> grown_signal_type;
    typedef boost::signals2::signal<void(T, T)> connection_added_signal_type;
    typedef boost::signals2::signal<void(T, T)> connection_removed_signal_type;

    network_st()
    :   agents_(nullptr)
    ,   n_agents_(0)
    ,   max_agents_(0)
    {}

    network_st(const ConfigType& conf)
    :   agents_(nullptr)
    ,   n_agents_(0)
    {
        init_agent_types(conf);
        allocate(conf.template get<T>("analysis.max_agents", 1000));
    }

    network_st(T n)
    :   agents_(nullptr)
    ,   n_agents_(0)
    ,   max_agents_(0)
    {   allocate(n); }

    ~network_st()
    {   delete[] agents_;   }

    void reset()
    {
        n_agents_ = 0;
        followers_.clear();
        followees_.clear();
        agent_type_.clear();
    }

    void allocate(T n)
    {
        if (agents_)
            delete[] agents_;
        max_agents_ = n;
        agents_ = new AgentType[max_agents_];
        agent_type_.reserve(max_agents_);
        followers_.reserve(max_agents_);
        followees_.reserve(max_agents_);
        V sum = 0;
        for (auto w : at_add_weight_)
            sum += w;
        for (unsigned i = 0; i < at_agent_ids_.size(); ++i)
            at_agent_ids_[i].reserve(
                std::size_t(max_agents_ * at_add_weight_[i] / sum + 10));
    }

    bool grow(W at = 0)
    {
        if (n_agents_ < max_agents_)
        {
            agent_type_.push_back(at);
            at_agent_ids_[at].push_back(n_agents_);
            followers_.emplace_back(std::unordered_set<T>());
            followees_.emplace_back(std::unordered_set<T>());
            ++n_agents_;
            grown_signal_(n_agents_ - 1, at);
            return true;
        }
        else 
            return false;
    }

    T grow(T n, W at = 0)
    {
        for (auto i = 0; i < n; ++i)
        {
            if (n_agents_ == max_agents_)
                return i;
            agent_type_.push_back(at);
            at_agent_ids_[at].push_back(n_agents_);
            followers_.emplace_back(std::unordered_set<T>());
            followees_.emplace_back(std::unordered_set<T>());
            ++n_agents_;
            grown_signal_(n_agents_ - 1, at);
        }
        return n;
    }

    T size() const
    {   return n_agents_;   }

    T max_size() const
    {   return max_agents_; }

    AgentType& back()
    {   return (*this)[n_agents_ - 1];  }

    AgentType* begin()
    {   return agents_; }

    AgentType* end()
    {   return agents_ + n_agents_; }

    AgentType& operator[](T idx)
    {
        BOOST_ASSERT_MSG(idx >= 0 && idx < n_agents_,
            "network out-of-bounds agent access :(");
        return agents_[idx];
    }

    W agent_type(T n) const
    {   return agent_type_[n];   }

    std::size_t count(std::size_t type) const
    {   return at_agent_ids_[type].size();   }

    std::string type_name(std::size_t type_idx) const
    {   return at_name_[type_idx];   }

    grown_signal_type& grown()
    {   return grown_signal_;   }

    connection_added_signal_type& connection_added()
    {   return connection_added_signal_;    }

    connection_removed_signal_type& connection_removed()
    {   return connection_removed_signal_;  }

    std::size_t followees_size(std::size_t id) const
    {
        return followees_[id].size();
    }

    std::size_t followers_size(std::size_t id) const
    {
        return followers_[id].size();
    }

    bool can_grow() const
    {
        return n_agents_ < max_agents_;
    }

    bool have_connection(T followee_id, T follower_id) const
    {
        return (    followers_[followee_id].find(follower_id)
               !=   followers_[followee_id].end()   );
    }

    bool connect(T followee_id, T follower_id)
    {
        BOOST_ASSERT_MSG(followee_id != follower_id,
            "agent cannot be connected to itself :(");

        if (followers_[followee_id].insert(follower_id).second
        &&  followees_[follower_id].insert(followee_id).second)
        {
            connection_added_signal_(followee_id, follower_id);
            return true;
        }
        else
            return false;
    }

    bool disconnect(T unfollowee_id, T unfollower_id)
    {
        BOOST_ASSERT_MSG(unfollowee_id != unfollower_id,
            "agent cannot be disconnected from itself :(");

        if (followers_[unfollowee_id].erase(unfollower_id))
        {
            followees_[unfollower_id].erase(unfollowee_id);
            connection_removed_signal_(unfollowee_id, unfollower_id);
            return true;
        }
        else
            return false;
    }

    std::ostream& print(std::ostream& out) const
    {
        out << "# Maximum Number of Agents: " << max_agents_ << std::endl;
        out << "# Number of Agents: " << n_agents_ << std::endl;
        out << "# Network: " << std::endl;

        for (unsigned i = 0; i < n_agents_; ++i)
        {
            out << std::setfill('0') << std::setw(8) << i << std::endl;

            out << std::setw(7) << followers_[i].size()
                << '<';
            if (followers_[i].size())
            {
                out << ' ';
                for (auto follower : followers_[i])
                    out << follower << ',';
            }
            out << std::endl;

            out << std::setw(7) << followees_[i].size()
                << '>';
            if (followees_[i].size())
            {
                out << ' ';
                for (auto following : followees_[i])
                    out << following << ',';
            }
            out << std::endl;
        }
        return out;
    }

private:
    // initialize agent types
    void init_agent_types(const ConfigType& conf)
    {
        for (auto const& v : boost::adaptors::reverse(conf))
            if (v.first == "agents")
            {
                at_agent_ids_.emplace_back(std::vector<T>());
                at_name_.emplace_back(v.second.get<std::string>("name"));
                at_add_weight_.emplace_back(v.second.get<V>("weights.add"));
            }
            else
                break;
    }

    // member variables
    AgentType* agents_;
    T n_agents_, max_agents_;
    // set of followers for the corresponding agent
    std::vector<std::unordered_set<T>> followers_;
    // set of agents that the corresponding agent acts as a followee
    std::vector<std::unordered_set<T>> followees_;
    // type of the corresponding agent
    std::vector<W> agent_type_;
    // names of agent types
    std::vector<std::string> at_name_;
    // list of agent ID's for agent types
    std::vector<std::vector<T>> at_agent_ids_;
    // add weight of agent types 
    std::vector<V> at_add_weight_;
    grown_signal_type grown_signal_;
    connection_added_signal_type connection_added_signal_;
    connection_removed_signal_type connection_removed_signal_;
};

template
<
    class    AgentType
,   class    ConfigType
,   typename T
,   typename V
,   typename W
>
std::ostream& operator<< (
    std::ostream& out
,   const network_st<AgentType, ConfigType, T, V, W>& n)
{
    return n.print(out);
}

}    // namespace hashkat

#endif  // HASHKAT_NETWORK_ST_HPP_

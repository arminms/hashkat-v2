///////////////////////////////////////////////////////////////////////////////
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

#ifndef HASHKAT_NETWORK_H_
#define HASHKAT_NETWORK_H_

namespace hashkat {

struct dummy
{};

template
<
    class    AgentType
,   class    ConfigType = dummy
,   typename T = std::size_t
,   typename ValueType = double
>
class network
{
public:
    typedef T type;
    typedef ValueType value_type;
    typedef AgentType agent_type;
    typedef ConfigType config_type;
    typedef network<AgentType, ConfigType, T, ValueType> self_type;
    typedef boost::signals2::signal<void(T)> grown_signal_type;
    typedef boost::signals2::signal<void(T, T)> connection_added_signal_type;
    typedef boost::signals2::signal<void(T, T)> connection_removed_signal_type;

private:
    AgentType* agents_;
    T n_agents_, max_agents_;
    std::vector<std::unordered_set<T>> followers_;
    std::vector<std::unordered_set<T>> following_;
    std::vector<std::unordered_set<T>> bins_;
    std::vector<ValueType> weights_;
    T denominator_;
    T kmax_;
    grown_signal_type grown_signal_;
    connection_added_signal_type connection_added_signal_;
    connection_removed_signal_type connection_removed_signal_;

public:
    network()
    :   agents_(nullptr)
    ,   n_agents_(0)
    ,   max_agents_(0)
    ,   denominator_(0)
    ,   kmax_(0)
    {}

    network(const ConfigType& conf)
    :   agents_(nullptr)
    ,   n_agents_(0)
    ,   max_agents_(conf.template get<T>("hashkat.network.max_agents", 1000))
    ,   denominator_(0)
    ,   kmax_(0)
    {}

    network(T n)
    :   agents_(nullptr)
    ,   n_agents_(0)
    ,   max_agents_(0)
    ,   denominator_(0)
    ,   kmax_(0)
    {   allocate(n); }

    ~network()
    {   delete[] agents_;   }

    void allocate(T n)
    {
        if (agents_)
            delete[] agents_;
        max_agents_ = n;
        agents_ = new AgentType[max_agents_];
        followers_.reserve(max_agents_);
        following_.reserve(max_agents_);
    }

    void grow(T n = 1)
    {
        for (auto i = 0; i < n; ++i)
        {
            followers_.emplace_back(std::unordered_set<T>());
            following_.emplace_back(std::unordered_set<T>());
            bins_[0].insert(n_agents_);
            ++denominator_;
            ++n_agents_;
        }
        grown_signal_(n);
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

    grown_signal_type& grown()
    {   return grown_signal_;   }

    connection_added_signal_type& connection_added()
    {   return connection_added_signal_;    }

    connection_removed_signal_type& connection_removed()
    {   return connection_removed_signal_;  }

    void initialize_bins(T min, T max, T inc = T(1),
        ValueType exp = ValueType(1), T spc = T(1))
    {
        for (auto i = 1; i < spc; ++i)
            inc *= inc;

        T count = (max - min) / inc;
        bins_.reserve(count + 1);
        weights_.reserve(count + 1);
        ValueType total_weight = 0;
        for (auto i = min; i <= max; i += inc)
        {
            bins_.emplace_back(std::unordered_set<T>());
            weights_.push_back(ValueType(std::pow(ValueType(i), exp)));
            total_weight += weights_.back();
        }
        if (total_weight > 0)
            for (auto i = 0; i < weights_.size(); ++i)
                weights_[i] /= total_weight;
    }

    T following_size(T id) const
    {
        return following_[id].size();
    }

    T followers_size(T id) const
    {
        return followers_[id].size();
    }

    T denominator() const
    {
        return denominator_;
    }

    T kmax() const
    {
        return kmax_;
    }

    bool can_grow() const
    {
        return n_agents_ < max_agents_;
    }

    bool have_connection(T followed_id, T follower_id) const
    {
        return (    followers_[followed_id].find(follower_id)
               !=   followers_[followed_id].end()   );
    }

    void connect(T followed_id, T follower_id)
    {
        BOOST_ASSERT_MSG(followed_id != follower_id,
            "agemt cannot connect to itself :(");
        BOOST_ASSERT_MSG(!have_connection(followed_id, follower_id),
            "already connected :(");

        auto idx = followers_size(followed_id) * bins_.size() / max_agents_;
        bins_[idx].erase(followed_id);
        followers_[followed_id].insert(follower_id);
        following_[follower_id].insert(followed_id);
        idx = followers_size(followed_id) * bins_.size() / max_agents_;
        bins_[idx].insert(followed_id);
        ++denominator_;
        if (kmax_ < idx)
            kmax_ = idx;
        connection_added_signal_(followed_id, follower_id);
    }

    void disconnect(T unfollowed_id, T unfollower_id)
    {
        BOOST_ASSERT_MSG(unfollowed_id != unfollower_id,
            "agemt cannot disconnect from itself :(");
        BOOST_ASSERT_MSG(have_connection(unfollowed_id, unfollower_id),
            "no connection to remove :(");

        auto idx = followers_size(unfollowed_id) * bins_.size() / max_agents_;
        bins_[idx].erase(unfollowed_id);
        followers_[unfollowed_id].erase(unfollower_id);
        following_[unfollower_id].erase(unfollowed_id);
        idx = followers_size(unfollowed_id) * bins_.size() / max_agents_;
        bins_[idx].insert(unfollowed_id);
        --denominator_;
        connection_removed_signal_(unfollowed_id, unfollower_id);
    }

    std::ostream& print(std::ostream& out) const
    {
        out << "# Maximum Number of Agents: " << max_agents_ << std::endl;
        out << "# Number of Agents: " << n_agents_ << std::endl;
        out << "# Number of Bins: " << bins_.size() << std::endl;
        out << "# Denominator: " << denominator_ << std::endl;
        out << "# kmax: " << kmax_ << std::endl;
        out << "# Network: " << std::endl;

        for (auto i = 0; i < n_agents_; ++i)
        {
            out << std::setfill('0') << std::setw(7) << i << std::endl;

            out << "    +---[in]" << std::endl
                << "        \\--- "
                << followers_[i].size()
                << " -> ";
            for (auto follower : followers_[i])
                out << follower << ',';
            out << std::endl;

            out << "    +---[out]" << std::endl
                << "        \\--- "
                << following_[i].size()
                << " -> ";
            for (auto following : following_[i])
                out << following << ',';
            out << std::endl;

            if (i < bins_.size())
            {
                out << "    \\---[bin] "
                    << weights_[i] << std::endl
                    << "        \\--- "
                    << bins_[i].size()
                    << " -> ";
                for (auto followed : bins_[i])
                    out << followed << ',';
                out << std::endl;
            }
        }
        return out;
    }
};

template
<
    class    AgentType
,   class    ConfigType
,   typename T
,   typename ValueType
>
std::ostream& operator<< (
    std::ostream& out
,   const network<AgentType, ConfigType, T, ValueType>& n)
{
    return n.print(out);
}

}    // namespace hashkat

#endif  // HASHKAT_NETWORK_H_

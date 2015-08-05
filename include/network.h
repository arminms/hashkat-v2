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

template
<
    class    AgentType
,   typename T = std::size_t
,   typename ValueType = double
>
class network
{
    AgentType* agents_;
    T n_agents_, max_agents_;
    std::vector<std::unordered_set<T>> followers_;
    std::vector<std::unordered_set<T>> following_;
    std::vector<std::pair<ValueType, std::unordered_set<T>>> bins_;
    T denominator_;
    T kmax_;

public:
    network()
    :   agents_(nullptr)
    ,   n_agents_(0)
    ,   max_agents_(0)
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
        //bins_.reserve(max_agents_);
    }

    void grow(T n = 1)
    {
        for (auto i = 0; i < n; ++i)
        {
            followers_.emplace_back(std::unordered_set<T>());
            following_.emplace_back(std::unordered_set<T>());
            //bins_.emplace_back(std::unordered_set<T>());
            bins_[0].second.insert(n_agents_);
            ++denominator_;
            ++n_agents_;
        }
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

    void initialize_bins(T min, T max, T inc = T(1),
        ValueType exp = ValueType(1), const std::string& spacing = "")
    {
        if ("quadratic" == spacing)
            inc *= inc;
        else if ("cubic" == spacing)
            inc *= inc * inc;

        T count = (max - min) / inc;
        bins_.reserve(count);
        ValueType total_weight = 0;
        for (auto i = min; i < max; i += inc)
        {
            bins_.emplace_back(std::make_pair(
                std::pow((double)i, exp)
            ,   std::unordered_set<T>()));
            //group.categories[j].prob = pow((double)i, exp);
            total_weight += i;
        }
        if (total_weight > 0)
            for (auto i = 0; i < bins_.size(); ++i)
                bins_[i].first /= total_weight;
    }

    T following_size(T id) const
    {
        return following_[id].size();
    }

    T followers_size(T id) const
    {
        return followers_[id].size();
    }

    const std::vector<std::unordered_set<T>>& bins() const
    {
        return bins_;
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

        bins_[followers_size(followed_id)].second.erase(followed_id);
        followers_[followed_id].insert(follower_id);
        following_[follower_id].insert(followed_id);
        bins_[followers_size(followed_id)].second.insert(followed_id);
        ++denominator_;
        if (kmax_ < followers_size(followed_id))
            kmax_ = followers_size(followed_id);
    }

    void disconnect(T unfollowed_id, T unfollower_id)
    {
        BOOST_ASSERT_MSG(unfollowed_id != unfollower_id,
            "agemt cannot disconnect from itself :(");
        BOOST_ASSERT_MSG(have_connection(unfollowed_id, unfollower_id),
            "no connection to remove :(");

        bins_[followers_size(unfollowed_id)].second.erase(unfollowed_id);
        followers_[unfollowed_id].erase(unfollower_id);
        following_[unfollower_id].erase(unfollowed_id);
        bins_[followers_size(unfollowed_id)].second.insert(unfollowed_id);
        --denominator_;
    }

    std::ostream& print(std::ostream& out) const
    {
        out << "# Number of Agents: " << n_agents_ << std::endl;
        out << "# Number of Bins: " << bins_.size() << std::endl;
        out << "# Denominator: " << denominator_ << std::endl;
        out << "# kmax: " << kmax_ << std::endl;
        out << "# Network: " << std::endl << std::endl;

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
                    << bins_[i].first << std::endl
                    << "        \\--- "
                    << bins_[i].second.size()
                    << " -> ";
                for (auto followed : bins_[i].second)
                    out << followed << ',';
            }

            out << std::endl;
        }
        return out;
    }
};

//typedef network<std::size_t> Network;
//typedef network<int> Network;

//std::ostream& operator<<(std::ostream& out, const Network& n)
//{
//    return n.print(out);
//}

}    // namespace hashkat

#endif  // HASHKAT_NETWORK_H_

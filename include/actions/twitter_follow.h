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

#ifndef HASHKAT_ACTIONS_TWITTER_FOLLOW_H_
#define HASHKAT_ACTIONS_TWITTER_FOLLOW_H_

#ifndef HASHKAT_ACTION_H_
#   include "../action.h"
#endif // HASHKAT_ACTION_H_

namespace hashkat {

////////////////////////////////////////////////////////////////////////////////
// twitter_follow action class

template
<
    class NetworkType
,   class ContentsType
,   class ConfigType
,   class RngType
>
class twitter_follow
:   public action_base<NetworkType, ContentsType, ConfigType, RngType>
{
    typedef NetworkType::type T;
    typedef NetworkType::value_type V;

public:
    twitter_follow(
        NetworkType& net
    ,   ContentsType& cnt
    ,   ConfigType& cnf
    ,   RngType& rng)
    :   action_base<NetworkType, ContentsType, ConfigType, RngType>()
    ,   net_(net)
    ,   cnt_(cnt)
    ,   cnf_(cnf)
    ,   rng_(rng)
    ,   n_connections_(0)
    ,   kmax_(0)
    {
        net_.grown().connect(boost::bind(&self_type::agent_added, this));

        T spc = cnf_.get<T>("hashkat.follow_ranks.bin_spacing", T(1));
        T min = cnf_.get<T>("hashkat.follow_ranks.min", T(1));
        T max = cnf_.get<T>("hashkat.follow_ranks.max", net_.max_size());
        T inc = cnf_.get<T>("hashkat.follow_ranks.increment", T(1));
        V exp = cnf_.get<V>("hashkat.follow_ranks.exponent", V(1.0));

        for (auto i = 1; i < spc; ++i)
            inc *= inc;

        T count = (max - min) / inc;
        bins_.reserve(count + 1);
        weights_.reserve(count + 1);
        V total_weight = 0;
        for (auto i = min; i <= max; i += inc)
        {
            bins_.emplace_back(std::unordered_set<T>());
            weights_.push_back(V(std::pow(V(i), exp)));
            total_weight += weights_.back();
        }
        if (total_weight > 0)
            for (auto i = 0; i < weights_.size(); ++i)
                weights_[i] /= total_weight;
    }

private:
    NetworkType& net_;
    ContentsType& cnt_;
    ConfigType& cnf_;
    RngType& rng_;
    T n_connections_;
    T kmax_;
    std::vector<std::unordered_set<T>> bins_;
    std::vector<ValueType> weights_;

    virtual bool do_action()
    {
        auto follower = select_follower();
        auto followee = select_followee();
        while ( follower != followee
            &&  !net_.have_connection(followee, follower) )
            followee = select_followee();

        auto idx = net_.followers_size(followee) * bins_.size()
                 / net_.max_agents();
        bins_[idx].erase(followee);

        net_.connect(followee, follower);

        idx = net_.followers_size(followee) * bins_.size() / net_.max_agents();
        bins_[idx].insert(followee);
        ++n_connections_;
        if (kmax_ < idx)
            kmax_ = idx;
        //rate_++;
        return true;
    }

    T select_follower()
    {
        std::uniform_int_distribution<T> di(0, net_.size());
        return di(rng_);
    }

    T select_followee()
    {
        std::uniform_int_distribution<T> di(0, net_.size());
        return di(rng_);
    }

    void agent_added(T idx)
    {
        bins_[0].insert(idx);
        ++n_connections_;
    }
};

}    // namespace hashkat

#endif  // HASHKAT_ACTIONS_TWITTER_FOLLOW_H_

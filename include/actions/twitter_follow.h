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

#   ifdef _MSC_VER
#       pragma warning( push )
#       pragma warning( disable: 4503 )
#   endif  // _MSC_VER

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
    typedef twitter_follow<NetworkType, ContentsType, ConfigType, RngType>
        self_type;
    typedef typename NetworkType::type T;
    typedef typename NetworkType::value_type V;

public:
    twitter_follow()
    :   action_base<NetworkType, ContentsType, ConfigType, RngType>()
    ,   net_ptr_(nullptr)
    ,   cnt_ptr_(nullptr)
    ,   cnf_ptr_(nullptr)
    ,   rng_ptr_(nullptr)
    ,   n_connections_(0)
    ,   kmax_(0)
    ,   count_(0)
    {}

    twitter_follow(
        NetworkType& net
    ,   ContentsType& cnt
    ,   ConfigType& cnf
    ,   RngType& rng)
    :   action_base<NetworkType, ContentsType, ConfigType, RngType>()
    ,   net_ptr_(&net)
    ,   cnt_ptr_(&cnt)
    ,   cnf_ptr_(&cnf)
    ,   rng_ptr_(&rng)
    ,   n_connections_(0)
    ,   kmax_(0)
    ,   count_(0)
    {
        init_slots();
        init_follow_models();
        init_bins();
    }

// Implementation
// everyhting below here is not reliable to count on
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
        init_slots();
        init_follow_models();
        init_bins();
    }

    virtual bool do_action()
    {
        BOOST_CONSTEXPR_OR_CONST auto failed = std::numeric_limits<T>::max();

        auto follower = select_follower();
        if (follower == failed)
            return false;

        auto followee = select_followee(follower);
        //while (followee == failed)
        //    followee = select_followee(follower);
        if (followee == failed || net_ptr_->have_connection(followee, follower))
            return false;

        auto idx = net_ptr_->followers_size(followee) * bins_.size()
                 / net_ptr_->max_size();
        bins_[idx].erase(followee);

        net_ptr_->connect(followee, follower);

        idx = net_ptr_->followers_size(followee) * bins_.size()
            / net_ptr_->max_size();
        bins_[idx].insert(followee);
        ++n_connections_;
        if (kmax_ < idx)
            kmax_ = idx;

        ++count_;
        // TODO - rate_ must be set based on network time
        return true;
    }

    virtual std::ostream& do_print(std::ostream& out) const
    {
        out << "# Follow count: " << count_ << std::endl;
        out << "# Follow rate: " << rate_ << std::endl;
        out << "# Number of Bins: " << bins_.size() << std::endl;
        out << "# Number of Connections: " << n_connections_ << std::endl;
        out << "# kmax: " << kmax_ << std::endl;
        out << "# Bins: " << std::endl;
        out << "#  K      W       N     Agent IDs" << std::endl;
        out << std::fixed << std::setprecision(3);
        for (auto i = 0; i < bins_.size(); ++i)
        {
            out << std::setfill('0') << std::setw(7) << i
                << ' ' << std::setw(5) << weights_[i] << " ["
                << std::setw(7) << bins_[i].size() << "] ";
            for (auto followee : bins_[i])
                out << followee << ',';
            out << std::endl;
        }
        return out;
    }

    // connect relevant slots to signals
    void init_slots()
    {
        net_ptr_->grown().connect(
            boost::bind(&self_type::agent_added, this, _1));
    }

    // initialize follow models
    void init_follow_models()
    {
        rate_ = cnf_ptr_->template get<T>
            ("hashkat.rates.follow", T(1));

        follow_models_ =
        {
            boost::bind(&self_type::random_follow_model , this , _1)
        ,   boost::bind(&self_type::twitter_suggest_follow_model , this, _1)
        ,   boost::bind(&self_type::agent_follow_model , this , _1 )
        ,   boost::bind(&self_type::preferential_agent_follow_model, this, _1)
        ,   boost::bind(&self_type::hashtag_follow_model, this, _1)
        };

        std::string follow_model = cnf_ptr_->template
            get<std::string>("hashkat.follow_model", "twitter");

        if (follow_model == "random")
            default_follow_model_ = follow_models_[0];
        else if (follow_model == "twitter_suggest")
            default_follow_model_ = follow_models_[1];
        else if (follow_model == "agent")
            default_follow_model_ = follow_models_[2];
        else if (follow_model == "preferential_agent")
            default_follow_model_ = follow_models_[3];
        else if (follow_model == "hashtag")
            default_follow_model_ = follow_models_[4];
        else if  (follow_model == "twitter")
            default_follow_model_ = 
                boost::bind(&self_type::twitter_follow_model , this , _1 );
        else
            default_follow_model_ = follow_models_[0];

        if (follow_model == "twitter")
        {
            model_weights_[0] = cnf_ptr_->template get<T>
                ("hashkat.twitter_follow_model.weights.random", T(1));
            model_weights_[1] = cnf_ptr_->template get<T>
                ("hashkat.twitter_follow_model.weights.twitter_suggest", T(1));
            model_weights_[2] = cnf_ptr_->template get<T>
                ("hashkat.twitter_follow_model.weights.agent", T(1));
            model_weights_[3] = cnf_ptr_->template get<T>
                ("hashkat.twitter_follow_model.weights.preferential_agent", T(1));
            model_weights_[4] = cnf_ptr_->template get<T>
                ("hashkat.twitter_follow_model.weights.hashtag", T(1));
         }
    }

    // initialize bins
    void init_bins()
    {
        T spc = cnf_ptr_->template get<T>
            ("hashkat.follow_ranks.bin_spacing", T(1));
        T min = cnf_ptr_->template get<T>
            ("hashkat.follow_ranks.min", T(1));
        T max = cnf_ptr_->template get<T>
            ("hashkat.follow_ranks.max", net_ptr_->max_size());
        T inc = cnf_ptr_->template get<T>
            ("hashkat.follow_ranks.increment", T(1));
        V exp = cnf_ptr_->template get<V>
            ("hashkat.follow_ranks.exponent", V(1.0));

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

    T select_follower()
    {
        std::uniform_int_distribution<T> di(0, net_ptr_->size() - 1);
        return di(*rng_ptr_);
    }

    T select_followee(T follower)
    {
        T followee = default_follow_model_(follower);
        // TODO - check for having the same language
        return followee == follower ? std::numeric_limits<T>::max() : followee;
    }

    T random_follow_model(T follower)
    {
        std::uniform_int_distribution<T> di(0, net_ptr_->size() - 1);
        return di(*rng_ptr_);
    }

    T twitter_suggest_follow_model(T follower)
    {
        //std::vector<V> weights(weights_);
        //for (auto i = 0; i < weights.size(); ++i)
        //    weights[i] *= bins_[i].size();
        //std::discrete_distribution<T> di(weights.cbegin(), weights.cend());

        std::vector<V> weights;
        weights.reserve(kmax_ + 1);
        std::transform(
            weights_.cbegin()
        ,   weights_.cbegin() + kmax_ + 1
        ,   bins_.cbegin()
        ,   std::back_inserter(weights)
        ,   [](V w, const std::unordered_set<T>& b)
        {   return w * b.size();    });
        std::discrete_distribution<T> di(weights.begin(), weights.end());

        //std::size_t i(0);
        //std::discrete_distribution<T> di(
        //    kmax_ + 1
        //,   0
        //,   double(kmax_ + 1) 
        //,   [&](double)
        //{   return weights_[i] * bins_[i++].size();    });

        auto followee = bins_[di(*rng_ptr_)].cbegin();
        return *followee;
    }

    T agent_follow_model(T follower)
    {
        // not implemented yet
        return std::numeric_limits<T>::max();
    }

    T preferential_agent_follow_model(T follower)
    {
        // not implemented yet
        return std::numeric_limits<T>::max();
    }

    T hashtag_follow_model(T follower)
    {
        // not implemented yet
        return std::numeric_limits<T>::max();
    }

    T twitter_follow_model(T follower)
    {
        std::discrete_distribution<T>
            di(model_weights_.begin(), model_weights_.end());
        return follow_models_[di(*rng_ptr_)](follower);
    }

    void agent_added(T idx)
    {
        bins_[0].insert(idx);
        ++n_connections_;
    }

// member variables
    NetworkType* net_ptr_;
    ContentsType* cnt_ptr_;
    ConfigType* cnf_ptr_;
    RngType* rng_ptr_;
    T n_connections_;
    T kmax_;
    T count_;
    std::vector<std::unordered_set<T>> bins_;
    std::vector<V> weights_;
    std::function<T(T)> default_follow_model_;
    std::array<std::function<T(T)>, 5> follow_models_;
    std::array<T, 5> model_weights_;
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
,   const twitter_follow<NetworkType, ContentsType, ConfigType, RngType>& tfa)
{
    return tfa.print(out);
}

#   ifdef _MSC_VER
#       pragma warning( pop )
#   endif  // _MSC_VER

}    // namespace hashkat

#endif  // HASHKAT_ACTIONS_TWITTER_FOLLOW_H_

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

#ifndef HASHKAT_ACTIONS_TWITTER_FOLLOW_ST_HPP_
#define HASHKAT_ACTIONS_TWITTER_FOLLOW_ST_HPP_

#include <boost/range/adaptor/reversed.hpp>

namespace hashkat {

////////////////////////////////////////////////////////////////////////////////
// twitter_follow_st action class

template
<
    class NetworkType
,   class ContentsType
,   class ConfigType
,   class RngType
,   class TimeType
>
class twitter_follow_st
:   public action_base<NetworkType, ContentsType, ConfigType, RngType, TimeType>
{
    typedef twitter_follow_st
        <NetworkType, ContentsType, ConfigType, RngType, TimeType> self_type;
    typedef action_base
        <NetworkType, ContentsType, ConfigType, RngType, TimeType> base_type;
    typedef typename base_type::weight_type weight_type;
    typedef typename NetworkType::type T;
    typedef typename NetworkType::value_type V;
    typedef typename NetworkType::agent_type_index_type W;


public:
    twitter_follow_st()
    :   action_base<NetworkType, ContentsType, ConfigType, RngType, TimeType>()
    ,   net_ptr_(nullptr)
    ,   cnt_ptr_(nullptr)
    ,   cnf_ptr_(nullptr)
    ,   rng_ptr_(nullptr)
    ,   approx_month_(30 * 24 * 60) // 30 days, 24 hours, 60 minutes
    {}

    twitter_follow_st(
        NetworkType& net
    ,   ContentsType& cnt
    ,   ConfigType& cnf
    ,   RngType& rng)
    :   action_base<NetworkType, ContentsType, ConfigType, RngType, TimeType>()
    ,   net_ptr_(&net)
    ,   cnt_ptr_(&cnt)
    ,   cnf_ptr_(&cnf)
    ,   rng_ptr_(&rng)
    ,   approx_month_(30 * 24 * 60) // 30 days, 24 hours, 60 minutes
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
    ,   RngType& rng
    ,   const TimeType& time)
    {
        net_ptr_  = &net;
        cnt_ptr_  = &cnt;
        cnf_ptr_  = &cnf;
        rng_ptr_  = &rng;
        time_ptr_ = &time;
        agent_creation_time_.reserve(cnf_ptr_->template get<T>
            ("analysis.max_agents", 1000));
        zero_add_rate_ = (0 == cnf_ptr_->template get<weight_type>
            ("rates.add.value", 1));

        init_slots();
        init_follow_models();
        init_bins();
        init_agent_types();
    }

    virtual void do_post_init()
    {
        base_type::rate_ = 0;
        base_type::weight_ = 0;
        follow_rate_ = cnf_ptr_->template
            get<typename base_type::rate_type>("agents.rates.follow", 0.0001);
        //base_type::weight_ = cnf_ptr_->template
        //    get<typename base_type::weight_type>("agents.rates.follow", 0.0001);
        n_connections_ = 0;
    }

    virtual void do_reset()
    {
        bins_.clear();
        weights_.clear();
        init_bins();
        do_post_init();
    }

    virtual void do_update_weight()
    {
        base_type::weight_ = 0;
        if (zero_add_rate_)
            for (std::size_t i = 0; i < at_monthly_weights_.size(); ++i)
                base_type::weight_ +=
                    net_ptr_->count(i) * at_monthly_weights_[i][month()];
        else
            for (std::size_t i = 0; i < at_monthly_weights_.size(); ++i)
                base_type::weight_ += at_agent_per_month_[i][month()]
                                   *  at_monthly_weights_[i][month()];
    }

    virtual void do_action()
    {
        BOOST_CONSTEXPR_OR_CONST auto failed = std::numeric_limits<T>::max();

        auto follower = select_follower();
        if (follower == failed)
        {
            base_type::action_finished_signal_();
            return;
        }

        auto followee = select_followee(follower);
        if (followee == failed)
        {
            base_type::action_finished_signal_();
            return;
        }

        if (net_ptr_->connect(followee, follower))
        {
            base_type::action_happened_signal_();
            base_type::action_finished_signal_();
        }
        else
            base_type::action_finished_signal_();
    }

    virtual std::ostream& do_print(std::ostream& out) const
    {
        out << "# Follow rate: " << base_type::rate_ << std::endl;
        out << "# Follow weight: " << base_type::weight_ << std::endl;
        out << "# Number of Bins: " << bins_.size() << std::endl;
        out << "# Number of Connections: " << n_connections_ << std::endl;
        out << "# kmax: " << kmax_ << std::endl;
        out << "# Bins: " << std::endl;
        out << "#   K        W         N     Agent IDs" << std::endl;
        out << std::scientific << std::setprecision(2);
        for (auto i = 0; i < bins_.size(); ++i)
        {
            out << std::setfill('0') << std::setw(8) << i
                << ' ' << std::setw(5) << weights_[i] << " ["
                << std::setw(8) << bins_[i].size() << "]";
            if (bins_[i].size())
            {
                out << ' ';
                for (auto followee : bins_[i])
                    out << followee << ',';
            }
            out << std::endl;
        }
        return out;
    }

    virtual void do_dump(const std::string& folder) const
    {}

    // connect relevant slots to signals
    void init_slots()
    {
        net_ptr_->grown().connect(
            boost::bind(&self_type::agent_added, this, _1, _2));
        net_ptr_->connection_added().connect(
            boost::bind(&self_type::update_bins, this, _1, _2));
    }

    // initialize follow models
    void init_follow_models()
    {
        follow_models_ =
        {
            boost::bind(&self_type::random_follow_model , this , _1)
        ,   boost::bind(&self_type::twitter_suggest_follow_model , this, _1)
        ,   boost::bind(&self_type::agent_follow_model , this , _1 )
        ,   boost::bind(&self_type::preferential_agent_follow_model, this, _1)
        ,   boost::bind(&self_type::hashtag_follow_model, this, _1)
        };

        std::string follow_model = cnf_ptr_->template
            get<std::string>("analysis.follow_model", "twitter");

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
                ("analysis.model_weights.random", T(1));
            model_weights_[1] = cnf_ptr_->template get<T>
                ("analysis.model_weights.twitter_suggest", T(1));
            model_weights_[2] = cnf_ptr_->template get<T>
                ("analysis.model_weights.weights.agent", T(1));
            model_weights_[3] = cnf_ptr_->template get<T>
                ("analysis.model_weights.preferential_agent", T(1));
            model_weights_[4] = cnf_ptr_->template get<T>
                ("analysis.model_weights.hashtag", T(1));
         }

        // init referral rate function for twitter_suggest follow model
        unsigned months = (unsigned)cnf_ptr_->template get<double>
            ("analysis.max_time", 1000) / approx_month_;
        for (unsigned i = 0; i <= months; ++i)
            monthly_referral_rate_.push_back(1.0 / double(1 + i));
    }

    // initialize bins
    void init_bins()
    {
        kmax_ = 0;

        T spc = cnf_ptr_->template get<T>
            ("follow_ranks.weights.bin_spacing", T(1));
        T min = cnf_ptr_->template get<T>
            ("follow_ranks.weights.min", T(1));
        T max = cnf_ptr_->template get<T>
            ("follow_ranks.weights.max", net_ptr_->max_size() + 1);
        T inc = cnf_ptr_->template get<T>
            ("follow_ranks.weights.increment", T(1));
        V exp = cnf_ptr_->template get<V>
            ("follow_ranks.weights.exponent", V(1.0));

        for (T i = 1; i < spc; ++i)
            inc *= inc;

        T count = (max - min) / inc;
        bins_.reserve(count + 1);
        weights_.reserve(count + 1);
        V total_weight = 0;
        for (T i = min; i <= max; i += inc)
        {
            bins_.emplace_back(std::unordered_set<T>());
            weights_.push_back(V(std::pow(V(i), exp)));
            total_weight += weights_.back();
        }
        if (total_weight > 0)
            for (T i = 0; i < weights_.size(); ++i)
                weights_[i] /= total_weight;
    }

    // initialize agent types
    void init_agent_types()
    {
        for (auto const& v : boost::adaptors::reverse(*cnf_ptr_))
        {
            if (v.first == "agents")
            {
                at_name_.emplace_back(v.second.get<std::string>("name"));
                at_add_weight_.emplace_back(v.second.get<weight_type>
                    ("weights.add", weight_type(100)));
                at_af_weight_.emplace_back(v.second.get<double>
                    ("weights.follow", 5));
                at_care_about_region_.emplace_back(v.second.get<bool>
                    ("hashtag_follow_options.care_about_region", false));
                at_care_about_ideology_.emplace_back(v.second.get<bool>
                    ("hashtag_follow_options.care_about_ideology", false));

                unsigned months = (unsigned)cnf_ptr_->template get<double>
                    ("analysis.max_time", 1000) / approx_month_;

                std::string f_type = v.second.get<std::string>
                    ("rates.follow.function", "constant");

                if (f_type == "linear" )
                {
                     weight_type y_intercept = v.second.get<weight_type>
                         ("rates.follow.y_intercept", 1);
                     weight_type slope = v.second.get<weight_type>
                         ("rates.follow.y_slope", 0.5);
                    at_monthly_weights_.emplace_back
                        (std::vector<weight_type>());
                    at_monthly_weights_.back().reserve(months + 1);
                    for (unsigned i = 0; i <= months; ++i)
                        at_monthly_weights_.back().push_back
                            (y_intercept + i * slope);
                    base_type::weight_ = at_monthly_weights_.back()[0];
                }
                else
                {
                    base_type::weight_ = v.second.get<weight_type>
                        ("rates.follow.value", 1);
                    at_monthly_weights_.emplace_back
                        (std::vector<weight_type>());
                    at_monthly_weights_.back().reserve(months + 1);
                    // TODO: this version is also possible instead of loop:
                    //at_monthly_weights_.emplace_back(std::vector<T>
                    //  (months + 1, base_type::weight_));
                    for (unsigned i = 0; i <= months; ++i)
                        at_monthly_weights_.back().push_back
                            (base_type::weight_);
                }

                at_agent_per_month_.emplace_back(std::vector<T>(months + 1, 0));
            }
            else
                break;
        }
    }

    T select_follower()
    {
        std::discrete_distribution<W> ddi(
            at_add_weight_.begin(), at_add_weight_.end());
        W at = ddi(*rng_ptr_);
        while (0 == net_ptr_->count(at))
            at = ddi(*rng_ptr_);

        if (zero_add_rate_)
        {
            std::uniform_int_distribution<T>
                udi(0, T(net_ptr_->count(at) - 1));
            return net_ptr_->agent_by_type(at, udi(*rng_ptr_));
        }
        else
        {
            weight_type sum = std::accumulate(
                at_monthly_weights_[at].begin()
            ,   at_monthly_weights_[at].end()
            ,   0.0);

            std::vector<weight_type> adjusted_add_weights;
            adjusted_add_weights.reserve(at_monthly_weights_[at].size());
            for (unsigned i = 0; i < at_monthly_weights_[at].size(); ++i)
                adjusted_add_weights.push_back(
                    at_monthly_weights_[at][i]
                *   at_add_weight_[at]
                /   sum);

            std::discrete_distribution<W> ddi(
                adjusted_add_weights.begin()
            ,   adjusted_add_weights.end());
            std::size_t month = ddi(*rng_ptr_);
            while (0 == at_agent_per_month_[at][month])
                --month;

            T start = (month
            ?   std::accumulate(
                    at_agent_per_month_[at].begin()
                ,   at_agent_per_month_[at].begin() + month
                ,   0)
            :   0);
            std::uniform_int_distribution<T>
                udi(start, start + at_agent_per_month_[at][month] - 1);
            return net_ptr_->agent_by_type(at, udi(*rng_ptr_));
        }
        //std::uniform_int_distribution<T> udi(0, net_ptr_->size() - 1);
        //return udi(*rng_ptr_);
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
        std::discrete_distribution<T> di(weights.cbegin(), weights.cend());

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

    void agent_added(T idx, W at)
    {
        agent_creation_time_.push_back(*time_ptr_);
        bins_[0].insert(idx);
        ++at_agent_per_month_[at][month()];
        ++n_connections_;
    }

    void update_bins(T followee, T follower)
    {
        std::size_t idx = net_ptr_->followers_size(followee) * bins_.size()
                 / net_ptr_->max_size();
        if (bins_[idx - 1].erase(followee))
            bins_[idx].insert(followee);
        else
            BOOST_ASSERT_MSG(false,
                "followee not found in the bins :(");

        if (kmax_ < idx)
            kmax_ = idx;

        ++base_type::rate_;
        ++n_connections_;
    }

    std::size_t month() const
    {   return std::size_t(time_ptr_->count() / approx_month_);   }

// member variables
    NetworkType* net_ptr_;
    ContentsType* cnt_ptr_;
    ConfigType* cnf_ptr_;
    RngType* rng_ptr_;
    const TimeType* time_ptr_;
    typename base_type::rate_type follow_rate_;
    std::size_t n_connections_;
    std::size_t kmax_;
    std::vector<std::unordered_set<T>> bins_;
    std::vector<V> weights_;
    std::function<T(T)> default_follow_model_;
    std::array<std::function<T(T)>, 5> follow_models_;
    std::array<T, 5> model_weights_;
    const int approx_month_;
    // referral rate function for each month, decreases over time by 1 / t
    std::vector<weight_type> monthly_referral_rate_;
    // creation time for the corresponding agent
    std::vector<TimeType> agent_creation_time_;
    // agent type name, NOTE: remove later if redundant/not used
    std::vector<std::string> at_name_;
    // agent type monthly follow weights
    std::vector<std::vector<weight_type>> at_monthly_weights_;
    // number of agents per month for each agent type
    std::vector<std::vector<T>> at_agent_per_month_;
    // agent type follow weight ONLY for 'agent' follow model
    std::vector<weight_type> at_af_weight_;
    // agent type add weight
    std::vector<weight_type> at_add_weight_;
    // agent type region care flag
    std::vector<bool> at_care_about_region_;
    // agent type ideology care flag
    std::vector<bool> at_care_about_ideology_;
    // true when add rate is zero
    bool zero_add_rate_;
};

template
<
    class NetworkType
,   class ContentsType
,   class ConfigType
,   class RngType
,   class TimeType
>
std::ostream& operator<< (
    std::ostream& out
,   const twitter_follow_st
        <NetworkType, ContentsType, ConfigType, RngType, TimeType>& tfa)
{
    return tfa.print(out);
}

}    // namespace hashkat

#endif  // HASHKAT_ACTIONS_TWITTER_FOLLOW_ST_HPP_

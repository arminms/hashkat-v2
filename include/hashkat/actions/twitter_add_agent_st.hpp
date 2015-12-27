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

#ifndef HASHKAT_ACTIONS_TWITTER_ADD_AGENT_ST_HPP_
#define HASHKAT_ACTIONS_TWITTER_ADD_AGENT_ST_HPP_

namespace hashkat {

////////////////////////////////////////////////////////////////////////////////
// twitter_add_agent_st action class

template
<
    class NetworkType
,   class ContentsType
,   class ConfigType
,   class RngType
,   class TimeType
>
class twitter_add_agent_st
:   public action_base<NetworkType, ContentsType, ConfigType, RngType, TimeType>
{
    typedef twitter_add_agent_st
        <NetworkType, ContentsType, ConfigType, RngType, TimeType> self_type;
    typedef action_base
        <NetworkType, ContentsType, ConfigType, RngType, TimeType> base_type;
    typedef typename base_type::weight_type weight_type;
    typedef typename NetworkType::type T;
    typedef typename NetworkType::value_type V;
    typedef typename NetworkType::agent_type_index_type W;

public:
    twitter_add_agent_st()
    :   action_base<NetworkType, ContentsType, ConfigType, RngType, TimeType>()
    ,   net_ptr_(nullptr)
    ,   cnt_ptr_(nullptr)
    ,   cnf_ptr_(nullptr)
    ,   rng_ptr_(nullptr)
    ,   approx_month_(30 * 24 * 60) // 30 days, 24 hours, 60 minutes
    {}

    twitter_add_agent_st(
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
    {}

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
        init_agent_types();
    }

    // initialize agent types
    void init_agent_types()
    {
        for (auto const& v : *cnf_ptr_)
            if (v.first == "agents")
            {
                //at_name_.emplace_back(v.second.get<std::string>("name"));
                at_add_weight_.push_back(v.second.template get<weight_type>
                    ("weights.add", weight_type(100)));
            }
    }

    virtual void do_post_init()
    {
        base_type::rate_ = 0;

        unsigned months = (unsigned)cnf_ptr_->template get<double>
            ("analysis.max_time", 1000) / approx_month_;
        monthly_weights_.reserve(months + 1);
        std::string f_type = cnf_ptr_->template get<std::string>
            ("rates.add.function", "constant");

        if (f_type == "linear" )
        {
             weight_type y_intercept = cnf_ptr_->template
                 get<weight_type>("rates.add.y_intercept", 0.001);
             weight_type slope = cnf_ptr_->template
                 get<weight_type>("rates.add.slope", 0.001);
            for (unsigned i = 0; i <= months; ++i)
                monthly_weights_.push_back(y_intercept + i * slope);
            base_type::weight_ = monthly_weights_[0];
        }
        else
        {
            base_type::weight_ = cnf_ptr_->template get
                <weight_type>("rates.add.value", 1);
            for (unsigned i = 0; i <= months; ++i)
                monthly_weights_.push_back(base_type::weight_);
        }

        T ia = cnf_ptr_->template get<T>
            ("analysis.initial_agents", T(0));
        for (T i = 0; i < ia; ++i)
        {
            std::discrete_distribution<W> di(
                at_add_weight_.begin(), at_add_weight_.end());
            if (net_ptr_->grow(di(*rng_ptr_)))
                ++base_type::rate_;
        }
    }

    virtual void do_reset()
    {
        do_post_init();
    }

    virtual void do_update_weight()
    {
        base_type::weight_ = monthly_weights_[month()]; 
    }

    virtual void do_action()
    {
        std::discrete_distribution<W> di(
            at_add_weight_.begin(), at_add_weight_.end());
        if (net_ptr_->grow(di(*rng_ptr_)))
        {
            ++base_type::rate_;
            base_type::action_happened_signal_();
        }
        base_type::action_finished_signal_();
    }

    virtual std::ostream& do_print(std::ostream& out) const
    {
        out << "# Add weight: " << base_type::weight_ << std::endl;
        out << "# Add rate: " << base_type::rate_ << std::endl;
        out << "# Add weight: " << base_type::weight_ << std::endl;
        return out;
    }

    virtual void do_dump(const std::string& folder) const
    {}

    std::size_t month() const
    {   return std::size_t(time_ptr_->count() / approx_month_);   }

// member variables
    NetworkType* net_ptr_;
    ContentsType* cnt_ptr_;
    ConfigType* cnf_ptr_;
    RngType* rng_ptr_;
    const TimeType* time_ptr_;
    const int approx_month_;
    std::vector<weight_type> monthly_weights_;
    // agent type add weight
    std::vector<weight_type> at_add_weight_;
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
,   const twitter_add_agent_st
        <NetworkType, ContentsType, ConfigType, RngType, TimeType>& aa)
{
    return aa.print(out);
}

}    // namespace hashkat

#endif  // HASHKAT_ACTIONS_TWITTER_ADD_AGENT_ST_HPP_

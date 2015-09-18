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

#ifndef HASHKAT_ACTIONS_TWITTER_ADD_AGENT_MT_HPP_
#define HASHKAT_ACTIONS_TWITTER_ADD_AGENT_MT_HPP_

namespace hashkat {

////////////////////////////////////////////////////////////////////////////////
// twitter_add_agent_mt action class

template
<
    class NetworkType
,   class ContentsType
,   class ConfigType
,   class RngType
,   class TimeType
>
class twitter_add_agent_mt
:   public action_base<NetworkType, ContentsType, ConfigType, RngType, TimeType>
{
    typedef twitter_add_agent_mt
        <NetworkType, ContentsType, ConfigType, RngType, TimeType> self_type;
    typedef action_base
        <NetworkType, ContentsType, ConfigType, RngType, TimeType> base_type;
    typedef typename NetworkType::type T;
    typedef typename NetworkType::value_type V;

public:
    twitter_add_agent_mt()
    :   action_base<NetworkType, ContentsType, ConfigType, RngType, TimeType>()
    ,   net_ptr_(nullptr)
    ,   cnt_ptr_(nullptr)
    ,   cnf_ptr_(nullptr)
    ,   rng_ptr_(nullptr)
    ,   approx_month_(30 * 24 * 60) // 30 days, 24 hours, 60 minutes
    {}

    twitter_add_agent_mt(
        NetworkType& net
    ,   ContentsType& cnt
    ,   ConfigType& cnf
    ,   RngType& rng)
    :   action_base<NetworkType, ContentsType, ConfigType, RngType, TimeType>()
    ,   net_ptr_(&net)
    ,   cnt_ptr_(&cnt)
    ,   cnf_ptr_(&cnf)
    ,   rng_ptr_(&rng)
    {}

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
    }

    virtual void do_post_init()
    {
        typedef typename base_type::weight_type weight_type;
        base_type::rate_.store(0);

        unsigned months = (unsigned)cnf_ptr_->template get<double>
            ("hashkat.network.max_time", 10) / approx_month_;
        monthly_weights_.reserve(months + 1);
        std::string f_type = cnf_ptr_->template get<std::string>
            ("hashkat.rates.add_function", "constant");

        if (f_type == "linear" )
        {
             weight_type y_intercept = cnf_ptr_->template
                 get<weight_type>("hashkat.rates.add_y_intercept", 1);
             weight_type slope = cnf_ptr_->template
                 get<weight_type>("hashkat.rates.add_y_slope", 0.5);
            for (unsigned i = 0; i <= months; ++i)
                monthly_weights_.push_back(y_intercept + i * slope);
            base_type::weight_ = monthly_weights_[0];
        }
        else
        {
            base_type::weight_ = cnf_ptr_->template get
                <weight_type>("hashkat.rates.add_rate", 1);
            for (unsigned i = 0; i <= months; ++i)
                monthly_weights_.push_back(base_type::weight_);
        }

        T ia = cnf_ptr_->template get<T>
            ("hashkat.network.initial_agents", T(0));
        for (T i = 0; i < ia; ++i)
            (*this)();
    }

    virtual void do_reset()
    {
        do_post_init();
    }

    virtual void do_update_weight(const TimeType& time)
    {
        base_type::weight_.store(
            monthly_weights_[std::size_t(time.count() / approx_month_)] ); 
    }

    virtual void do_action()
    {
        if (net_ptr_->grow())
        {
            ++base_type::rate_;
            base_type::action_happened_signal_();
        }
        base_type::action_finished_signal_();
    }

    virtual std::ostream& do_print(std::ostream& out) const
    {
        out << "# Add rate: " << base_type::rate_ << std::endl;
        out << "# Add weight: " << base_type::weight_ << std::endl;
        return out;
    }

// member variables
    NetworkType* net_ptr_;
    ContentsType* cnt_ptr_;
    ConfigType* cnf_ptr_;
    RngType* rng_ptr_;
    const int approx_month_;
    std::vector<typename base_type::weight_type> monthly_weights_;
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
,   const twitter_add_agent_mt
        <NetworkType, ContentsType, ConfigType, RngType, TimeType>& aa)
{
    return aa.print(out);
}

}    // namespace hashkat

#endif  // HASHKAT_ACTIONS_TWITTER_ADD_AGENT_MT_HPP_

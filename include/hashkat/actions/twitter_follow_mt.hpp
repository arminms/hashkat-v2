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

#ifndef HASHKAT_ACTIONS_TWITTER_FOLLOW_MT_HPP_
#define HASHKAT_ACTIONS_TWITTER_FOLLOW_MT_HPP_

#include <mutex>
#include <tbb/concurrent_unordered_set.h>

namespace hashkat {

////////////////////////////////////////////////////////////////////////////////
// twitter_follow_mt action class

template
<
    class NetworkType
,   class ContentsType
,   class ConfigType
,   class RngType
,   class TimeType
>
class twitter_follow_mt
:   public action_base<NetworkType, ContentsType, ConfigType, RngType, TimeType>
{
    typedef twitter_follow_mt
        <NetworkType, ContentsType, ConfigType, RngType, TimeType> self_type;
    typedef action_base
        <NetworkType, ContentsType, ConfigType, RngType, TimeType> base_type;
    typedef typename base_type::weight_type weight_type;
    typedef typename NetworkType::type T;
    typedef typename NetworkType::value_type V;
    typedef typename NetworkType::agent_type_index_type W;

public:
    twitter_follow_mt()
    :   action_base<NetworkType, ContentsType, ConfigType, RngType, TimeType>()
    ,   net_ptr_(nullptr)
    ,   cnt_ptr_(nullptr)
    ,   cnf_ptr_(nullptr)
    ,   rng_ptr_(nullptr)
    ,   approx_month_(30 * 24 * 60) // 30 days, 24 hours, 60 minutes
    {}

    twitter_follow_mt(
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

        zero_add_rate_ = (0 == cnf_ptr_->template get<weight_type>
            ("rates.add.value", 1));

        auto max_agents = cnf_ptr_->template get<T>
            ("analysis.max_agents", 1000);
        agent_creation_time_.reserve(max_agents);
        agent_as_followee_method_counts_.reserve(max_agents);
        agent_as_follower_method_counts_.reserve(max_agents);

        init_slots();
        init_follow_models();
        init_agent_types();

        n_connections_ = 0;
    }

    virtual void do_post_init()
    {
        base_type::rate_ = 0;
        base_type::weight_ = 0;
    }

    virtual void do_reset()
    {
        bins_.clear();
        weights_.clear();
        do_post_init();
    }

    virtual void do_update_weight()
    {
        auto months = month();
        if (months == at_agent_per_month_.back().size() - 1)
        {
            std::lock_guard<std::mutex> g(add_month_mutex_);
            if (months == at_agent_per_month_.back().size() - 1)
            {
                //std::cout << "month: " << months << std::endl;
                for (auto i = 0; i < at_name_.size(); ++i)
                    at_agent_per_month_[i].push_back
                        (std::unique_ptr<std::atomic<T>>(new std::atomic<T>(0)));
                save_degree_distributions(cnf_ptr_->template
                    get<std::string>("output_folder", "output"));
            }
        }

        weight_type w = 0;
        if (zero_add_rate_)
            for (std::size_t at = 0; at < at_monthly_weights_.size(); ++at)
                w += std::atomic<weight_type>(
                     net_ptr_->count(at) 
                  *  at_monthly_weights_[at][months]);
        else
            for (std::size_t at = 0; at < at_monthly_weights_.size(); ++at)
            {
                w += *(at_agent_per_month_[at].back())
                  *  at_monthly_weights_[at][0];
                for (std::size_t i = 1, j = months; i <= months; ++i, --j)
                    w += at_monthly_weights_[at][i]
                      *  at_agent_per_month_[at][j]->load();
            }
        std::lock_guard<std::mutex> g(update_weight_mutex_);
        base_type::weight_ = w;
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

        handle_follow(followee, follower);
    }

    virtual std::ostream& do_print(std::ostream& out) const
    {
        out << "# Follow rate: " << base_type::rate_ << std::endl;
        out << "# Follow weight: " << base_type::weight_ << std::endl;
        out << "# Number of Bins: " << bins_.size() << std::endl;
        out << "# Number of Connections: " << n_connections_ << std::endl;
        out << "# kmax: " << kmax_ << std::endl;
        out << "# Bins: " << std::endl;
        out << "#   K        N      Agent IDs" << std::endl;

        // building the bins
        std::vector<std::unordered_set<T>> bins(net_ptr_->size() + 1);
        for (T i = 0; i < net_ptr_->size(); ++i)
            bins[net_ptr_->followers_size(i)].insert(i);

        out << std::scientific << std::setprecision(2);
        for (T i = 0; i < bins.size(); ++i)
        {
            out << std::setfill('0') << std::setw(8) << i
                << " [" << std::setw(8) << bins[i].size() << "]";
            if (bins[i].size())
            {
                out << ' ';
                for (auto followee : bins[i])
                    out << followee << ',';
            }
            out << std::endl;
        }
        return out;
    }

    virtual void do_dump(const std::string& folder) const
    {
        if (cnf_ptr_->template get<bool>("output.main_statistics", true))
        {
            std::ofstream out(
                folder + "/main_stats.dat"
            ,   std::ofstream::app);

            out << "FOLLOWS\n"
                << "_______\n\n";
            out << "Total follows: " << base_type::rate_ << "\n";
            std::size_t sum = std::accumulate(
                follow_models_count_.begin()
            ,   follow_models_count_.end()
            ,   std::size_t(0));
            out << "Total follow attempts: " << sum << "\n";
            out << "Random: " << follow_models_count_[0]
                << "\t(" << 100 * follow_models_count_[0] / double(sum)
                << "% of total follow attempts)\n";
            out << "Twitter_Suggest: " << follow_models_count_[1]
                << "\t(" << 100 * follow_models_count_[1] / double(sum)
                << "% of total follow attempts)\n";
            out << "Agent: " << follow_models_count_[2]
                << "\t(" << 100 * follow_models_count_[2] / double(sum)
                << "% of total follows attempts)\n";
            out << "Preferential_Agent: " << follow_models_count_[3]
                << "\t(" << 100 * follow_models_count_[3] / double(sum)
                << "% of total follow attempts)\n";
            out << "Retweet: " << 0 // TODO: must be implemented 
                << "\t(" << 0 
                << "% of total follows attempts)\n";
            out << "Hashtag: " << follow_models_count_[4]
                << "\t(" << 100 * follow_models_count_[4] / double(sum)
                << "% of total follows attempts)\n";
            out << "Followbacks: " << follow_models_count_[6]
                << "\t (" << 100 * follow_models_count_[6] / double(sum)
                << "% of total follow attempts)\n";

            for (std::size_t i = 0; i < at_name_.size(); ++i)
                out << at_name_[i] << ": "
                    << at_follows_count_[i]->load() << "\t(" 
                    << 100
                     * at_follows_count_[i]->load()
                     / double(base_type::rate_)
                    << "% of total follows)\n";
        }

        if (cnf_ptr_->template get<bool>("output.categories_distro", true))
        {
            std::ofstream out(
                folder + "/Categories_Distro.dat"
            ,   std::ofstream::trunc); // TODO: change to std::ofstream::app

            out << "Following | ";
            for (auto i = 0; i < bins_.size(); ++i)
                out << bins_[i].size() << " at " << i << "|\t";
            out << std::endl;
        }

        if (cnf_ptr_->template get<bool>
            ("output.degree_distribution_by_follow_model", true))
        {
            std::size_t max_degree = 0;
            for (std::size_t i = 0; i < net_ptr_->size(); ++i)
            {
                std::size_t sum = net_ptr_->followees_size(i)
                                + net_ptr_->followers_size(i);
                if (sum > max_degree)
                    max_degree = sum + 1;
            }

            // +2 for retweeting and followback
            std::vector<std::array<double, 5+2>> follow_models;
            follow_models.reserve(max_degree);
            for (auto i = 0; i < max_degree; ++i)
                follow_models.emplace_back(std::array<double, 5+2> { {} });

            for (std::size_t i = 0; i < net_ptr_->size(); ++i)
                for (std::size_t j = 0; j < 7; ++j)
                {
                    std::size_t degree = (*agent_as_followee_method_counts_[i])[j]
                                       + (*agent_as_follower_method_counts_[i])[j];
                    ++follow_models[degree][j]; 
                }

            std::ofstream out(
                folder + "/dd_by_follow_model.dat"
            ,   std::ofstream::trunc);

            out << "This is the degree distribution by follow model. The data "
                   "order is:\n# degree\tlog_of_degree\tRandom-normalized_prob"
                   "ability\tRandom-log_of_normalized_probability\tTwitter_Sug"
                   "gest-normalized_probability\tTwitter_Suggest-log_of_normal"
                   "ized_probability\tAgent-normalized_probability\tAgent-log_"
                   "of_normalized_probability\tPreferential_Agent-normalized_p"
                   "robability\tPreferential_Agent-log_of_normalized_probabili"
                   "ty\tHashtag-normalized_probability\tHashtag-log_of_normali"
                   "zed_probability\tTwitter-normalized_probability\tTwitter-l"
                   "og_of_normalized_probability\tFollowbacks-normalized_proba"
                   "bility\tFollowbacks-log_of_normalized_probability\n\n";

            for (std::size_t i = 0; i < max_degree; ++i)
            {
                out << i << "\t" << std::log(i);
                for (auto& ent_type : follow_models[i])
                    out << "\t" << ent_type / net_ptr_->size()
                        << "\t" << std::log(ent_type / net_ptr_->size());
                out << std::endl;
            }
        }

        if (cnf_ptr_->template get<bool>("output.agent_stats", true))
        {
            for (std::size_t i = 0; i < at_name_.size(); ++i)
            {
                std::size_t max_degree = 0;
                for (T j = 0; j < net_ptr_->count(i); ++j)
                {
                    std::size_t degree
                    =   net_ptr_->followees_size(net_ptr_->agent_by_type(i, j))
                    +   net_ptr_->followers_size(net_ptr_->agent_by_type(i, j));
                    if (degree > max_degree)
                        max_degree = degree;
                }

                std::vector<T> agent_followers(max_degree + 1, 0);
                std::vector<T> agent_followees(max_degree + 1, 0);
                std::vector<T> agent_degree(max_degree + 1, 0);
                std::vector<T> who_followees(at_name_.size(), 0);
                std::vector<T> who_followers(at_name_.size(), 0);

                std::size_t followers_sum = 0, followees_sum = 0;
                for (T j = 0; j < net_ptr_->count(i); ++j)
                {
                    T id = net_ptr_->agent_by_type(i, j);
                    std::size_t in_degree = net_ptr_->followers_size(id);
                    std::size_t out_degree = net_ptr_->followees_size(id);
                    ++agent_followers[in_degree];
                    ++agent_followees[out_degree];
                    ++agent_degree[in_degree + out_degree];
                    for (auto follower : net_ptr_->follower_set(id))
                    {
                        ++who_followees[net_ptr_->agent_type(follower)];
                        ++followees_sum;
                    }
                    for (auto followee : net_ptr_->followee_set(id))
                    {
                        ++who_followers[net_ptr_->agent_type(followee)];
                        ++followers_sum;
                    }
                }

                std::ofstream out(folder
                +   '/'
                +   at_name_[i]
                +   "_info.dat"
                ,   std::ofstream::trunc);

                out << "# Agent percentages following agent type \'"
                    << at_name_[i] << "\'\n# ";
                for (std::size_t j = 0; j < at_name_.size(); ++j)
                    out << at_name_[j] << ": "
                        << who_followees[j] / double(followees_sum) * 100.0
                        << "   ";

                out << "\n# Agent percentages that agent type \'"
                    << at_name_[i] << "\' follows\n# ";
                for (std::size_t j = 0; j < at_name_.size(); ++j)
                    out << at_name_[j] << ": "
                        << who_followers[j] / double(followers_sum) * 100.0
                        << "   ";

                out << "\n# degree\tin_degree\tout_degree\tcumulative\tlog"
                       "(degree)\tlog(in_degree)\tlog(out_degree)\tlog"
                       "(cumulative)\n\n";
                for (std::size_t j = 0; j < max_degree; ++j)
                    out << j << "\t"
                        << agent_followers[j] / (double)net_ptr_->count(i)
                        << "\t"
                        << agent_followees[j] / (double)net_ptr_->count(i)
                        << "\t"
                        << agent_degree[j] / (double)net_ptr_->count(i)
                        << "\t" << std::log(j) << "\t"
                        << std::log
                            (agent_followers[j] / (double)net_ptr_->count(i))
                        << "\t"
                        << std::log
                            (agent_followees[j] / (double)net_ptr_->count(i))
                        << "\t"
                        << std::log
                            (agent_degree[j] / (double)net_ptr_->count(i))
                        << "\n";
            }
        }

        if (cnf_ptr_->template get<bool>("output.degree_distributions", true))
            save_degree_distributions(folder);
    }

    // connect relevant slots to signals
    void init_slots()
    {
        net_ptr_->grown().connect(boost::bind
            (&self_type::update_counters_when_agent_added, this, _1, _2));
        net_ptr_->connection_added().connect(boost::bind
            (&self_type::update_counters_when_connection_added, this, _1, _2));
        if (cnf_ptr_->template get<bool>("analysis.use_followback", false))
            net_ptr_->connection_added().connect(boost::bind
                (&self_type::followback_when_connection_added, this, _1, _2));
    }

    // initialize follow models
    void init_follow_models()
    {
        follow_method_ = -1;
        follow_models_count_.fill(0);

        barabasi_connections_ = cnf_ptr_->template get<T>
            ("analysis.barabasi_connections", T(1));

        follow_models_ =
        {
            boost::bind(&self_type::random_follow_model , this , _1)
        ,   boost::bind(&self_type::twitter_suggest_follow_model , this, _1)
        ,   boost::bind(&self_type::agent_follow_model , this , _1 )
        ,   boost::bind(&self_type::preferential_agent_follow_model, this, _1)
        ,   boost::bind(&self_type::hashtag_follow_model, this, _1)
        };

        // binding barabasi follow model to index 1 if necessary
        bool use_barabasi = cnf_ptr_->template
            get<bool>("analysis.use_barabasi", false);
        if (use_barabasi)
        {
            follow_models_[1] = boost::bind
                (&self_type::barabasi_follow_model, this , _1 );
            net_ptr_->grown().connect(boost::bind
                (&self_type::barabasi_follow_when_agent_added, this, _1, _2));
        }

        std::string follow_model = cnf_ptr_->template
            get<std::string>("analysis.follow_model", "twitter");

        if (follow_model == "random")
            default_follow_model_ = follow_models_[0];
        else if (follow_model == "twitter_suggest")
        {
            default_follow_model_ = follow_models_[1];
            net_ptr_->grown().connect(boost::bind
                (&self_type::update_bins_when_agent_added, this, _1, _2));
            if (use_barabasi)
                net_ptr_->connection_added().connect(boost::bind
                    (&self_type::update_barabasi_bins_when_connection_added,
                        this, _1, _2));
            else
                net_ptr_->connection_added().connect(boost::bind
                    (&self_type::update_bins_when_connection_added,
                        this, _1, _2));
            init_bins(true);
        }
        else if (follow_model == "agent")
            default_follow_model_ = follow_models_[2];
        else if (follow_model == "preferential_agent")
        {
            default_follow_model_ = follow_models_[3];
            net_ptr_->grown().connect(boost::bind
                (&self_type::update_at_bins_when_agent_added, this, _1, _2));
            net_ptr_->connection_added().connect(boost::bind
                (&self_type::update_at_bins_when_connection_added, this, _1, _2));
            init_bins(false);
        }
        else if (follow_model == "hashtag")
            default_follow_model_ = follow_models_[4];
        else if  (follow_model == "twitter")
            default_follow_model_ = 
                boost::bind(&self_type::twitter_follow_model, this , _1 );
        else
            default_follow_model_ = follow_models_[0];

        if (follow_model == "twitter")
        {
            model_weights_[0] = cnf_ptr_->template get<V>
                ("analysis.model_weights.random", V(1));
            model_weights_[1] = cnf_ptr_->template get<V>
                ("analysis.model_weights.twitter_suggest", V(1));
            model_weights_[2] = cnf_ptr_->template get<V>
                ("analysis.model_weights.agent", V(1));
            model_weights_[3] = cnf_ptr_->template get<V>
                ("analysis.model_weights.preferential_agent", V(1));
            model_weights_[4] = cnf_ptr_->template get<V>
                ("analysis.model_weights.hashtag", V(1));

            if (model_weights_[1] > 0)
            {
                net_ptr_->grown().connect(boost::bind
                    (&self_type::update_bins_when_agent_added, this, _1, _2));
                if (use_barabasi)
                    net_ptr_->connection_added().connect(boost::bind
                        (&self_type::update_barabasi_bins_when_connection_added,
                            this, _1, _2));
                else
                    net_ptr_->connection_added().connect(boost::bind
                        (&self_type::update_bins_when_connection_added,
                            this, _1, _2));
                init_bins(true);
            }

            if (model_weights_[3] > 0)
            {
                net_ptr_->grown().connect(boost::bind
                    (&self_type::update_at_bins_when_agent_added, this, _1, _2));
                net_ptr_->connection_added().connect(boost::bind
                    (&self_type::update_at_bins_when_connection_added,
                        this, _1, _2));
                if (model_weights_[1] == 0)
                    init_bins(false);
            }
        }

        // init referral rate function for twitter_suggest follow model
        unsigned months = (unsigned)cnf_ptr_->template get<double>
            ("analysis.max_time", 1000) / approx_month_;
        monthly_referral_rate_.reserve(months);
        for (unsigned i = 0; i <= months; ++i)
            monthly_referral_rate_.push_back(1.0 / double(1 + i));
    }

    // initialize bins for twitter_suggest and preferential_agent
    void init_bins(bool add_bins)
    {
        kmax_ = 0;
        V exp = cnf_ptr_->template get<V>
            ("analysis.barabasi_exponent", V(1.0));

        if (cnf_ptr_->template get<bool>("analysis.use_barabasi", false))
        {
            bins_.reserve(net_ptr_->max_size() + 1);
            weights_.reserve(net_ptr_->max_size() + 1);
            for (T i = 1; i < net_ptr_->max_size(); ++i)
            {
                bins_.emplace_back(tbb::concurrent_unordered_set<T>());
                weights_.push_back(V(std::pow(V(i), exp)));
            }
        }
        else
        {
            T spc = cnf_ptr_->template get<T>
                ("follow_ranks.weights.bin_spacing", T(1));
            T min = cnf_ptr_->template get<T>
                ("follow_ranks.weights.min", T(1));
            T max = cnf_ptr_->template get<T>
                ("follow_ranks.weights.max", net_ptr_->max_size() + 1);
            T inc = cnf_ptr_->template get<T>
                ("follow_ranks.weights.increment", T(1));

            for (T i = 1; i < spc; ++i)
                inc *= inc;

            T count = (max - min) / inc;
            if (add_bins)
                bins_.reserve(count + 1);
            weights_.reserve(count + 1);
            V total_weight = 0;
            for (T i = min; i <= max; i += inc)
            {
                if (add_bins)
                    bins_.emplace_back(tbb::concurrent_unordered_set<T>());
                weights_.push_back(V(std::pow(V(i), exp)));
                total_weight += weights_.back();
            }
            if (total_weight > 0)
                for (T i = 0; i < weights_.size(); ++i)
                    weights_[i] /= total_weight;
        }
    }

    // initialize agent types
    void init_agent_types()
    {
        for (auto const& v : *cnf_ptr_)
        {
            if (v.first == "agents")
            {
                at_name_.emplace_back(v.second.template
                    get<std::string>("name"));
                at_add_weight_.emplace_back(v.second.template
                    get<weight_type>("weights.add", weight_type(100)));
                at_af_weight_.emplace_back(v.second.template
                    get<weight_type>("weights.follow", weight_type(5)));
                at_followback_weight_.emplace_back(v.second.template
                    get<weight_type>("followback_probability", weight_type(0)));
                at_care_about_region_.emplace_back(v.second.template get<bool>
                    ("hashtag_follow_options.care_about_region", false));
                at_care_about_ideology_.emplace_back(v.second.template get<bool>
                    ("hashtag_follow_options.care_about_ideology", false));

                unsigned months = (unsigned)cnf_ptr_->template get<double>
                    ("analysis.max_time", 1000) / approx_month_;

                std::string f_type = v.second.template
                    get<std::string>("rates.follow.function", "constant");

                at_monthly_weights_.emplace_back(std::vector<weight_type>());
                at_monthly_weights_.back().reserve(months + 1);

                if (f_type == "linear" )
                {
                     weight_type y_intercept = v.second.template
                         get<weight_type>("rates.follow.y_intercept", 0.001);
                     weight_type slope = v.second.template
                         get<weight_type>("rates.follow.slope", 0.001);
                    for (unsigned i = 0; i <= months; ++i)
                        at_monthly_weights_.back().push_back
                            (y_intercept + i * slope);
                    base_type::weight_ = at_monthly_weights_.back()[0];
                }
                else if (f_type == "twitter_follow")
                {
                    // initial spike for the first 2 months
                    at_monthly_weights_.back().push_back
                        (7.44 * 0.0008298429200320164);
                    at_monthly_weights_.back().push_back
                        (7.44 * 0.0008298429200320164);
                    // gradual rate after the 2 months
                    for (unsigned i = 3; i <= months; ++i)
                        at_monthly_weights_.back().push_back
                            (7.44 * 5.5360422914604546e-05);
                }
                else if (f_type == "quarter_twitter_follow")
                {
                    // initial spike for the first 2 months
                    at_monthly_weights_.back().push_back
                        (7.44 * 0.25 * 0.0008298429200320164);
                    at_monthly_weights_.back().push_back
                        (7.44 * 0.25 * 0.0008298429200320164);
                    // gradual rate after the 2 months
                    for (unsigned i = 3; i <= months; ++i)
                        at_monthly_weights_.back().push_back
                            (7.44 * 0.25 * 5.5360422914604546e-05);
                }
                else
                {
                    base_type::weight_ = v.second.template get<weight_type>
                        ("rates.follow.value", 1);
                    // TODO: this version is also possible instead of loop:
                    //at_monthly_weights_.emplace_back(std::vector<T>
                    //  (months + 1, base_type::weight_));
                    for (unsigned i = 0; i <= months; ++i)
                        at_monthly_weights_.back().push_back
                            (base_type::weight_);
                }

                // initializing bins for preferential_agent_follow_model
                std::function<T(T)> tfm(
                    boost::bind(&self_type::twitter_follow_model , this , _1 ));

                if (default_follow_model_.template target<T(T)>()
                ==  follow_models_[3].template target<T(T)>()
                || (default_follow_model_.template target<T(T)>()
                ==  tfm.template target<T(T)>()
                &&  model_weights_[3] > 0) )
                {
                    at_kmaxes_.push_back(0);
                    update_at_bins_mutex_.emplace_back
                        (std::unique_ptr<std::mutex>(new std::mutex));

                    T spc = cnf_ptr_->template get<T>
                        ("follow_ranks.weights.bin_spacing", T(1));
                    T min = cnf_ptr_->template get<T>
                        ("follow_ranks.weights.min", T(1));
                    T max = cnf_ptr_->template get<T>
                        ("follow_ranks.weights.max", net_ptr_->max_size() + 1);
                    T inc = cnf_ptr_->template get<T>
                        ("follow_ranks.weights.increment", T(1));

                    for (T i = 1; i < spc; ++i)
                        inc *= inc;

                    T count = (max - min) / inc;
                    at_bins_.emplace_back
                        (std::vector<tbb::concurrent_unordered_set<T>>());
                    at_bins_.back().reserve(count + 1);
                    for (T i = min; i <= max; i += inc)
                        at_bins_.back().emplace_back
                            (tbb::concurrent_unordered_set<T>());
                }

                // initializing number of agent types per month
                at_agent_per_month_.emplace_back(tbb::concurrent_vector
                    <std::unique_ptr<std::atomic<T>>>());
                at_agent_per_month_.back().reserve(months + 1);
                at_agent_per_month_.back().push_back
                    (std::unique_ptr<std::atomic<T>>(new std::atomic<T>(0)));
                at_follows_count_.push_back
                    (std::unique_ptr<std::atomic<std::size_t>>
                        (new std::atomic<std::size_t>(0)));
            }
        }
    }

    T select_follower()
    {
        std::vector<weight_type> adjusted_weights;
        if (zero_add_rate_)
        {
            for (std::size_t at = 0; at < at_add_weight_.size(); ++at)
                adjusted_weights.push_back(
                    net_ptr_->count(at)
                *   at_add_weight_[at]);
            std::discrete_distribution<W> dd(
                adjusted_weights.begin(), adjusted_weights.end());
            W at = dd(*rng_ptr_);
            std::uniform_int_distribution<T>
                ud(0, T(net_ptr_->count(at) - 1));
            return net_ptr_->agent_by_type(at, ud(*rng_ptr_));
        }
        else
        {
            std::vector<std::pair<std::size_t, std::size_t>> grid;
            for (std::size_t at = 0; at < at_add_weight_.size(); ++at)
                for (std::size_t month = 0;
                     month < at_agent_per_month_[at].size();
                     ++month)
                {
                    adjusted_weights.push_back(
                        at_monthly_weights_[at][month]
                    *   at_add_weight_[at]);
                    grid.push_back(std::make_pair(at, month));
                }

                std::discrete_distribution<W> dd(
                    adjusted_weights.begin()
                ,   adjusted_weights.end());
                auto r = grid[dd(*rng_ptr_)];
                auto at = r.first;
                if (0 == net_ptr_->count(at))
                    return std::numeric_limits<T>::max();
                auto month = r.second;
                if (0 == at_agent_per_month_[at][month]->load())
                    return std::numeric_limits<T>::max();

                T sum = 0;
                for (std::size_t i = 0; i < month; ++i)
                    sum += at_agent_per_month_[at][i]->load();
                T start = (month ? sum : 0);
                std::uniform_int_distribution<T>
                    di(start, start + at_agent_per_month_[at][month]->load() - 1);
                return net_ptr_->agent_by_type(at, di(*rng_ptr_));
        }
    }

    T select_followee(T follower)
    {
        T followee = default_follow_model_(follower);
        // TODO - check for having the same language
        return followee == follower ? std::numeric_limits<T>::max() : followee;
    }

    T random_follow_model(T follower)   // 0
    {
        follow_method_ = 0;
        ++follow_models_count_[follow_method_];
        std::uniform_int_distribution<T> di(0, net_ptr_->size() - 1);
        return di(*rng_ptr_);
    }

    T barabasi_follow_model(T follower) // 1
    {
        follow_method_ = 1;
        ++follow_models_count_[follow_method_];

        std::vector<V> weights;
        std::transform(
            weights_.cbegin()
        ,   weights_.cbegin() + kmax_ + 1
        ,   bins_.cbegin()
        ,   std::back_inserter(weights)
        ,   [](V w, const tbb::concurrent_unordered_set<T>& b)
        {   return w * b.size();    });
        std::discrete_distribution<T> di(weights.cbegin(), weights.cend());

        auto idx = di(*rng_ptr_);
        if (0 == bins_[idx].size())
            return std::numeric_limits<T>::max();

        std::vector<T> v;
        v.reserve(bins_[idx].size());
        std::copy(bins_[idx].cbegin(), bins_[idx].cend(), std::back_inserter(v));
        std::uniform_int_distribution<std::size_t> udi(0, v.size() - 1);
        return v[udi(*rng_ptr_)];
    }

    T twitter_suggest_follow_model(T follower)
    {
        follow_method_ = 1;
        ++follow_models_count_[follow_method_];

        unsigned bin = unsigned(
            (time_ptr_->count() - agent_creation_time_[follower])
        /   (double)approx_month_);
        std::uniform_real_distribution<double> dr(0, 1);
        if (!(dr(*rng_ptr_) < monthly_referral_rate_[bin]))
            return std::numeric_limits<T>::max();
        std::vector<V> weights;
        auto kmax = kmax_.load();
        weights.reserve(kmax + 1);

        std::transform(
            weights_.cbegin()
        ,   weights_.cbegin() + kmax + 1
        ,   bins_.cbegin()
        ,   std::back_inserter(weights)
        ,   [](V w, const tbb::concurrent_unordered_set<T>& b)
        {   return w * b.size();    });
        std::discrete_distribution<T> di(weights.cbegin(), weights.cend());

        auto idx = di(*rng_ptr_);
        if (0 == bins_[idx].size())
            return std::numeric_limits<T>::max();
        //std::uniform_int_distribution<std::size_t> udi(0, bins_[idx].size() - 1);
        //auto followee = std::next(bins_[idx].cbegin(), udi(*rng_ptr_));
        //return *followee;
        std::vector<T> v;
        v.reserve(bins_[idx].size());
        std::copy(bins_[idx].cbegin(), bins_[idx].cend(), std::back_inserter(v));
        std::uniform_int_distribution<std::size_t> udi(0, v.size() - 1);
        return v[udi(*rng_ptr_)];
    }

    T agent_follow_model(T follower)    // 2
    {
        follow_method_ = 2;
        ++follow_models_count_[follow_method_];

        std::discrete_distribution<W> dd(
            at_af_weight_.cbegin()
        ,   at_af_weight_.cend());
        W at = dd(*rng_ptr_);
        if (0 == net_ptr_->count(at))
            return std::numeric_limits<T>::max();
        std::uniform_int_distribution<T>
            ud(0, T(net_ptr_->count(at) - 1));
        // TODO: checking for the agent language
        return net_ptr_->agent_by_type(at, ud(*rng_ptr_));
    }

    T preferential_agent_follow_model(T follower)   // 3
    {
        follow_method_ = 3;
        ++follow_models_count_[follow_method_];

        // first selecting agent type
        std::discrete_distribution<W> dd(
            at_af_weight_.cbegin()
        ,   at_af_weight_.cend());
        W at = dd(*rng_ptr_);

        // make sure we're not pulling from an empty list
        if (0 == net_ptr_->count(at))
            return std::numeric_limits<T>::max();

        // selecting bin's index
        std::vector<V> weights;
        weights.reserve(at_kmaxes_[at] + 1);
        std::transform(
            weights_.cbegin()
        ,   weights_.cbegin() + at_kmaxes_[at] + 1
        ,   at_bins_[at].cbegin()
        ,   std::back_inserter(weights)
        ,   [](V w, const tbb::concurrent_unordered_set<T>& b)
        {   return w * b.size();    });
        std::discrete_distribution<T> di(weights.cbegin(), weights.cend());
        auto idx = di(*rng_ptr_);

        // selecting agent
        if (0 == at_bins_[at][idx].size())
            return std::numeric_limits<T>::max();
        std::vector<T> bin;
        bin.reserve(at_bins_[at][idx].size());
        std::copy(at_bins_[at][idx].cbegin(),
                  at_bins_[at][idx].cend(),
                  std::back_inserter(bin));
        std::uniform_int_distribution<std::size_t> udi(0, bin.size() - 1);
        return bin[udi(*rng_ptr_)];
    }

    T hashtag_follow_model(T follower)  // 4
    {
        follow_method_ = 4;
        ++follow_models_count_[follow_method_];

        // not implemented yet
        return std::numeric_limits<T>::max();
    }

    T twitter_follow_model(T follower)
    {
        std::discrete_distribution<T>
            di(model_weights_.begin(), model_weights_.end());
        return follow_models_[di(*rng_ptr_)](follower);
    }

    // slot for network::grow() signal
    void update_counters_when_agent_added(T idx, W at)
    {
        agent_creation_time_.push_back(time_ptr_->count());
        std::unique_ptr<std::array<std::atomic<T>, 7>>
            a(new std::array<std::atomic<T>, 7> {});
        agent_as_followee_method_counts_.push_back(std::move(a));
        a = std::unique_ptr<std::array<std::atomic<T>, 7>>
            (new std::array<std::atomic<T>, 7> {});
        agent_as_follower_method_counts_.push_back(std::move(a));
        ++(*(at_agent_per_month_[at].back()));
        ++n_connections_;
    }

    // slot for network::grow() signal
    void update_bins_when_agent_added(T idx, W at)
    {
        std::lock_guard<std::mutex> g(update_bins_mutex_);
        bins_[0].insert(idx);
    }

    // slot for network::grow() signal
    void update_at_bins_when_agent_added(T idx, W at)
    {
        std::lock_guard<std::mutex> g(*update_at_bins_mutex_[at]);
        at_bins_[at][0].insert(idx);
    }

    // slot for network::grow() signal
    void barabasi_follow_when_agent_added(T follower, W at)
    {
        if (follower < 2)
            return;

        BOOST_CONSTEXPR_OR_CONST auto failed = std::numeric_limits<T>::max();
        for (T i = 0; i < barabasi_connections_; ++i)
        {
            auto followee = select_followee(follower);
            if (followee == failed)
            {
                base_type::action_finished_signal_();
                return;
            }

            handle_follow(followee, follower);
        }
    }

    // slot for network::connection_added() signal
    void update_counters_when_connection_added(T followee, T follower)
    {
        ++base_type::rate_;
        ++n_connections_;
    }

    // slot for network::connection_added() signal
    void update_bins_when_connection_added(T followee, T follower)
    {
        std::size_t idx;
        {
            std::lock_guard<std::mutex> g(update_bins_mutex_);
            idx = net_ptr_->followers_size(followee)
                * bins_.size()
                / net_ptr_->max_size();
            if (bins_[idx - 1].unsafe_erase(followee))
                bins_[idx].insert(followee);
            else
            {
                while (bins_[idx].find(followee) == bins_[idx].end() && idx > 0)
                    --idx;
                bins_[idx].unsafe_erase(followee);
                bins_[++idx].insert(followee);
            }
        }
        if (kmax_ < idx)
            kmax_.store(idx);
    }


    // slot for network::connection_added() signal
    void update_barabasi_bins_when_connection_added(T followee, T follower)
    {
        std::size_t idx;
        {
            std::lock_guard<std::mutex> g(update_bins_mutex_);
            idx = net_ptr_->followers_size(followee);
            if (bins_[idx - 1].unsafe_erase(followee))
                bins_[idx].insert(followee);
            else
            {
                while (bins_[idx].find(followee) == bins_[idx].end() && idx > 0)
                    --idx;
                bins_[idx].unsafe_erase(followee);
                bins_[++idx].insert(followee);
            }
        }

        if (kmax_ < idx)
            kmax_.store(idx);
    }

    // slot for network::connection_added() signal
    void update_at_bins_when_connection_added(T followee, T follower)
    {
        auto at = net_ptr_->agent_type(followee);
        std::size_t idx;
        {
            std::lock_guard<std::mutex> g(*update_at_bins_mutex_[at]);
            idx = net_ptr_->followers_size(followee)
                * at_bins_[at].size()
                / net_ptr_->max_size();
            if (at_bins_[at][idx - 1].unsafe_erase(followee))
                at_bins_[at][idx].insert(followee);
            else
            {
                while (bins_[idx].find(followee) == bins_[idx].end() && idx > 0)
                    --idx;
                bins_[idx].unsafe_erase(followee);
                bins_[++idx].insert(followee);
            }
        }

        if (at_kmaxes_[at] < idx)
            at_kmaxes_[at] = idx;
    }

    // slot for network::connection_added() signal
    void followback_when_connection_added(T followee, T follower)
    {
        auto fbw = at_followback_weight_[net_ptr_->agent_type(followee)];
        std::discrete_distribution<> dd({1.0 - fbw, fbw});
        if (dd(*rng_ptr_))
        {
            follow_method_ = 6;
            ++follow_models_count_[follow_method_];
            handle_follow(follower, followee);
        }
    }

    void handle_follow(T followee, T follower)
    {
        if (net_ptr_->connect(followee, follower))
        {
            ++(*at_follows_count_[net_ptr_->agent_type(follower)]);
            ++((*agent_as_followee_method_counts_[followee])[follow_method_]);
            ++((*agent_as_follower_method_counts_[follower])[follow_method_]);
            base_type::action_happened_signal_();
            base_type::action_finished_signal_();
        }
        else
            base_type::action_finished_signal_();
    }

    std::size_t month() const
    {   return std::size_t(time_ptr_->count() / approx_month_);   }

    void save_degree_distributions(const std::string& folder) const
    {
        std::size_t max_followees = 0, max_followers = 0;
        for (T i = 0; i < net_ptr_->size(); ++i)
        {
            if (net_ptr_->followees_size(i) >= max_followees)
                max_followees = net_ptr_->followees_size(i) + 1;
            if (net_ptr_->followers_size(i) >= max_followers)
                max_followers = net_ptr_->followers_size(i) + 1;
        }
        std::size_t max_degree = max_followees + max_followers;

        std::vector<T> od_distro(max_followees, 0);
        std::vector<T> id_distro(max_followers, 0);
        std::vector<T> cd_distro(max_degree, 0);
        for (T i = 0; i < net_ptr_->size(); ++i)
        {
            std::size_t out, in;
            ++od_distro[out = net_ptr_->followees_size(i)];
            ++id_distro[in  = net_ptr_->followers_size(i)];
            ++cd_distro[out + in];
        }

        std::size_t max = 0;
        for (auto count : cd_distro)
            if (count > max)
                max = count;

        std::string base("-degree_distribution_month_");
        std::string rest("degree distribution. The data order is:\n# degree, "
            "normalized probability, log of degree, log of normalized"
            " probability\n\n");

        std::ostringstream fname(folder, std::ios_base::ate);
        fname << "/out" << base << std::setfill('0')
              << std::setw(3) << month() << ".dat";
        std::ofstream out(fname.str(), std::ofstream::trunc);
        out << "# This is the out-" << rest;
        for (std::size_t i = 0; i < max_followees; ++i)
            out << i << "\t"
                << od_distro[i] / (double)net_ptr_->size()
                << "\t" << std::log(i) << "\t"
                << std::log(od_distro[i] / (double)net_ptr_->size())
                << std::endl;
        out.close();

        fname.str(folder);
        fname << "/in" << base << std::setfill('0')
              << std::setw(3) << month() << ".dat";
        out.open(fname.str(), std::ofstream::trunc);
        out << "# This is the in-" << rest;
        for (std::size_t i = 0; i < max_followers; ++i)
            out << i << "\t"
                << id_distro[i] / (double)net_ptr_->size()
                << "\t" << std::log(i) << "\t"
                << std::log(id_distro[i] / (double)net_ptr_->size())
                << std::endl;
        out.close();

        fname.str(folder);
        fname << "/cumulative" << base << std::setfill('0')
              << std::setw(3) << month() << ".dat";
        out.open(fname.str(), std::ofstream::trunc);
        out << "# This is the cumulative " << rest;
        for (std::size_t i = 0; i < max_degree; ++i)
            out << i << "\t"
                << cd_distro[i] / (double)net_ptr_->size()
                << "\t" << std::log(i) << "\t"
                << std::log(cd_distro[i] / (double)net_ptr_->size())
                << std::endl;
    }

// member variables
    NetworkType* net_ptr_;
    ContentsType* cnt_ptr_;
    ConfigType* cnf_ptr_;
    RngType* rng_ptr_;
    const TimeType* time_ptr_;
    std::atomic<std::size_t> n_connections_;
    std::vector<tbb::concurrent_unordered_set<T>> bins_;
    std::vector<V> weights_;
    std::atomic<std::size_t> kmax_;
    std::function<T(T)> default_follow_model_;
    std::array<std::function<T(T)>, 5> follow_models_;
    std::array<std::size_t, 7> follow_models_count_;
    std::array<V, 5> model_weights_;
    int follow_method_;
    const int approx_month_;
    // referral rate function for each month, decreases over time by 1 / t
    std::vector<weight_type> monthly_referral_rate_;
    // creation time for the corresponding agent
    std::vector<double> agent_creation_time_;
    // counts of follow models for the corresponding agent as a followee
    tbb::concurrent_vector<std::unique_ptr<std::array<std::atomic<T>, 7>>>
        agent_as_followee_method_counts_;
    // counts of follow models for the corresponding agent as a follower
    tbb::concurrent_vector<std::unique_ptr<std::array<std::atomic<T>, 7>>>
        agent_as_follower_method_counts_;
    // agent type name, NOTE: remove later if redundant/not used
    std::vector<std::string> at_name_;
    // agent type monthly follow weights
    std::vector<std::vector<weight_type>> at_monthly_weights_;
    // number of agents per month for each agent type
    std::vector<tbb::concurrent_vector<std::unique_ptr<std::atomic<T>>>>
        at_agent_per_month_;
    // number of follows for each agent type
    std::vector<std::unique_ptr<std::atomic<std::size_t>>> at_follows_count_;
    // agent type follow weight ONLY for 'agent' follow model
    std::vector<weight_type> at_af_weight_;
    // agent type add weight
    std::vector<weight_type> at_add_weight_;
    // agent type followback weight
    std::vector<weight_type> at_followback_weight_;
    // agent type region care flag
    std::vector<bool> at_care_about_region_;
    // agent type ideology care flag
    std::vector<bool> at_care_about_ideology_;
    // bins for each agent type used in preferential_agent_follow_model
    std::vector<std::vector<tbb::concurrent_unordered_set<T>>> at_bins_;
    // max degrees for each agent type used in preferential_agent_follow_model
    std::vector<std::size_t> at_kmaxes_;
    // true when add rate is zero
    bool zero_add_rate_;
    // number of connections for barabasi model
    T barabasi_connections_;

    // mutex for updating base_type::weight_
    std::mutex update_weight_mutex_;
    // mutex for updating bins
    std::mutex update_bins_mutex_;
    // mutex for adding new month
    std::mutex add_month_mutex_;
    // mutexes for updating bins for each agent type
    std::vector<std::unique_ptr<std::mutex>> update_at_bins_mutex_;
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
,   const twitter_follow_mt
        <NetworkType, ContentsType, ConfigType, RngType, TimeType>& tfa)
{
    return tfa.print(out);
}

}    // namespace hashkat

#endif  // HASHKAT_ACTIONS_TWITTER_FOLLOW_MT_HPP_

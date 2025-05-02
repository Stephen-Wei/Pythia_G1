#include <iostream>
#include <vector>
#include <assert.h>
#include <strings.h>
#include <numeric>
#include "util.h"
#include "learning_engine_featurewise.h"
#include "scooby.h"
#include "stats.h"                // 新增：全局统计接口（get_llc_miss_rate(), get_bw_usage(), get_mispred_rate()）

#if 0
#define LOCKED(...)     \
	{                   \
		fflush(stdout); \
		__VA_ARGS__;    \
		fflush(stdout); \
	}
#define LOGID() fprintf(stdout, "[%25s@%3u] ", \
						__FUNCTION__, __LINE__);
#define MYLOG(...) LOCKED(LOGID(); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n");)
#else
#define MYLOG(...) \
	{              \
	}
#endif

namespace knob
{
	extern vector<int32_t> le_featurewise_active_features;
	extern vector<int32_t> le_featurewise_num_tilings;
	extern vector<int32_t> le_featurewise_num_tiles;
	extern vector<int32_t> le_featurewise_hash_types;
	extern vector<int32_t> le_featurewise_enable_tiling_offset;
	extern float le_featurewise_max_q_thresh;
	extern bool le_featurewise_enable_action_fallback;
	extern vector<float> le_featurewise_feature_weights;
	extern bool le_featurewise_enable_dynamic_weight;
	extern float le_featurewise_weight_gradient; /* hyperparameter */
	extern bool le_featurewise_disable_adjust_weight_all_features_align;
	extern bool le_featurewise_selective_update;
	extern uint32_t le_featurewise_pooling_type;
	extern bool le_featurewise_enable_dyn_action_fallback;
	extern uint32_t le_featurewise_bw_acc_check_level;
	extern uint32_t le_featurewise_acc_thresh;
	extern bool le_featurewise_enable_trace;
	extern std::string le_featurewise_trace_file_name;
	extern bool le_featurewise_enable_score_plot;
	extern vector<int32_t> le_featurewise_plot_actions;
	extern std::string le_featurewise_plot_file_name;
	extern bool le_featurewise_remove_plot_script;
}

void LearningEngineFeaturewise::init_knobs()
{
	assert(knob::le_featurewise_active_features.size() == knob::le_featurewise_num_tilings.size());
	assert(knob::le_featurewise_active_features.size() == knob::le_featurewise_num_tiles.size());
	assert(knob::le_featurewise_active_features.size() == knob::le_featurewise_enable_tiling_offset.size());
	assert(knob::le_featurewise_active_features.size() == knob::le_featurewise_feature_weights.size());
	if (knob::le_featurewise_enable_dyn_action_fallback)
	{
		assert(knob::le_featurewise_enable_action_fallback);
	}
}

void LearningEngineFeaturewise::init_stats()
{
}

LearningEngineFeaturewise::LearningEngineFeaturewise(Prefetcher *parent, float alpha, float gamma, float epsilon, uint32_t actions, uint64_t seed, std::string policy, std::string type, bool zero_init)
	: LearningEngineBase(parent, alpha, gamma, epsilon, actions, 0 /*dummy state value*/, seed, policy, type)
{
	/* init each feature engine */
	for (uint32_t index = 0; index < NumFeatureTypes; ++index)
	{
		m_feature_knowledges[index] = NULL;
	}
	for (uint32_t index = 0; index < knob::le_featurewise_active_features.size(); ++index)
	{
		assert(knob::le_featurewise_active_features[index] < NumFeatureTypes);
		m_feature_knowledges[knob::le_featurewise_active_features[index]] = new FeatureKnowledge((FeatureType)knob::le_featurewise_active_features[index],
																								 alpha,
																								 gamma,
																								 actions,
																								 knob::le_featurewise_feature_weights[index],
																								 knob::le_featurewise_weight_gradient,
																								 knob::le_featurewise_num_tilings[index],
																								 knob::le_featurewise_num_tiles[index],
																								 zero_init,
																								 knob::le_featurewise_hash_types[index],
																								 knob::le_featurewise_enable_tiling_offset[index]);
		assert(m_feature_knowledges[knob::le_featurewise_active_features[index]]);
	}

	m_max_q_value = (float)1 / (1 - gamma) * std::accumulate(knob::le_featurewise_num_tilings.begin(), knob::le_featurewise_num_tilings.end(), 0);
	/* init Q-value buckets */
	m_q_value_buckets.push_back((-1) * 0.50 * m_max_q_value);
	m_q_value_buckets.push_back((-1) * 0.25 * m_max_q_value);
	m_q_value_buckets.push_back((-1) * 0.00 * m_max_q_value);
	m_q_value_buckets.push_back((+1) * 0.25 * m_max_q_value);
	m_q_value_buckets.push_back((+1) * 0.50 * m_max_q_value);
	m_q_value_buckets.push_back((+1) * 1.00 * m_max_q_value);
	m_q_value_buckets.push_back((+1) * 2.00 * m_max_q_value);
	/* init histogram */
	m_q_value_histogram.resize(m_q_value_buckets.size() + 1, 0);

	/* init random generators */
	m_generator.seed(m_seed);
	m_explore = new std::bernoulli_distribution(epsilon);
	m_actiongen = new std::uniform_int_distribution<int>(0, m_actions - 1);

	/* init stats */
	bzero(&stats, sizeof(stats));
}

LearningEngineFeaturewise::~LearningEngineFeaturewise()
{
	for (uint32_t index = 0; index < NumFeatureTypes; ++index)
	{
		if (m_feature_knowledges[index])
			delete m_feature_knowledges[index];
	}
}

// uint32_t LearningEngineFeaturewise::chooseAction(State *state, float &max_to_avg_q_ratio, vector<bool> &consensus_vec)
// {
// 	stats.action.called++;
// 	uint32_t action = 0;
// 	max_to_avg_q_ratio = 0.0;
// 	consensus_vec.resize(NumFeatureTypes, false);

// 	if (m_type == LearningType::SARSA && m_policy == Policy::EGreedy)
// 	{
// 		if ((*m_explore)(m_generator))
// 		{
// 			action = (*m_actiongen)(m_generator); // take random action
// 			stats.action.explore++;
// 			stats.action.dist[action][0]++;
// 			MYLOG("action taken %u explore, state %s, scores %s", action, state->to_string().c_str(), getStringQ(state).c_str());
// 		}
// 		else
// 		{
// 			float max_q = 0.0;
// 			action = getMaxAction(state, max_q, max_to_avg_q_ratio, consensus_vec);
// 			stats.action.exploit++;
// 			stats.action.dist[action][1]++;
// 			gather_stats(max_q, max_to_avg_q_ratio); /* for only stats collection's sake */
// 			MYLOG("action taken %u exploit, state %s, scores %s", action, state->to_string().c_str(), getStringQ(state).c_str());
// 		}
// 	}
// 	else
// 	{
// 		printf("learning_type %s policy %s not supported!\n", MapLearningTypeString(m_type), MapPolicyString(m_policy));
// 		assert(false);
// 	}

// 	return action;
// }

uint32_t LearningEngineFeaturewise::chooseAction(State *state, float &max_to_avg_q_ratio, std::vector<bool> &consensus_vec)
{
    // ================================
    // 【新增】生成当前上下文向量
    // 利用全局统计数据采集新指标：LLC miss rate、带宽利用率、分支预测失误率，
    // 并映射到 0~255 范围内；后面3个槽位保留原有 state 参数（bw_level、acc_level、pc的低8位）
    ContextVector ctx;
    {
        // 新指标1：LLC miss rate（假设 get_llc_miss_rate() 返回 0~1 之间的比例）
        double llc_miss = get_llc_miss_rate();  
        ctx.features[0] = static_cast<uint8_t>(std::min(255.0, llc_miss * 255.0));

        // 新指标2：带宽利用率（单位：GB/s），假设最大带宽为50GB/s，则缩放因子约为5.1
        double bw_usage = get_bw_usage();
        ctx.features[1] = static_cast<uint8_t>(std::min(255.0, bw_usage * 5.1));

        // 新指标3：分支预测失误率（0~1之间），直接映射到 0~255
        double mispred = get_mispred_rate();
        ctx.features[2] = static_cast<uint8_t>(std::min(255.0, mispred * 255.0));

        // 其他3个特征：使用 State 中已有的参数（你可以根据需要调整映射方式）
        ctx.features[3] = static_cast<uint8_t>(std::min(state->bw_level, (uint8_t)255));
        ctx.features[4] = static_cast<uint8_t>(std::min(state->acc_level, (uint32_t)255));
        ctx.features[5] = static_cast<uint8_t>(state->pc & 0xFF);
    }
	

    // ================================
    // 根据当前上下文更新每个 FeatureKnowledge 对象的权重集
    // 每个特征知识库内部会调用 load_weight_set_by_context()，
    // 根据欧几里得距离匹配最合适的权重集，从而影响后续 Q 值计算
    for (uint32_t i = 0; i < NumFeatureTypes; ++i)
    {
        if (m_feature_knowledges[i])
        {
            m_feature_knowledges[i]->load_weight_set_by_context(ctx);
        }
    }
    // ================================

    // 原有的 chooseAction 逻辑
    stats.action.called++;
    uint32_t action = 0;
    max_to_avg_q_ratio = 0.0;
    consensus_vec.resize(NumFeatureTypes, false);

    if (m_type == LearningType::SARSA && m_policy == Policy::EGreedy)
    {
        if ((*m_explore)(m_generator))
        {
            action = (*m_actiongen)(m_generator); // 随机探索
            stats.action.explore++;
            stats.action.dist[action][0]++;
            MYLOG("action taken %u explore, state %s, scores %s", action, state->to_string().c_str(), getStringQ(state).c_str());
        }
        else
        {
            float max_q = 0.0;
            action = getMaxAction(state, max_q, max_to_avg_q_ratio, consensus_vec);
            stats.action.exploit++;
            stats.action.dist[action][1]++;
            gather_stats(max_q, max_to_avg_q_ratio); // 仅用于统计收集
            MYLOG("action taken %u exploit, state %s, scores %s", action, state->to_string().c_str(), getStringQ(state).c_str());
        }
    }
    else
    {
        printf("learning_type %s policy %s not supported!\n", MapLearningTypeString(m_type), MapPolicyString(m_policy));
        assert(false);
    }

    return action;
}


// void LearningEngineFeaturewise::learn(State *state1, uint32_t action1, int32_t reward, State *state2, uint32_t action2, vector<bool> consensus_vec, RewardType reward_type)
// {
// 	stats.learn.called++;
// 	if(m_type == LearningType::SARSA && m_policy == Policy::EGreedy)
// 	{
// 		for(uint32_t index = 0; index < NumFeatureTypes; ++index)
// 		{
// 			if(m_feature_knowledges[index])
// 			{
// 				if(!knob::le_featurewise_selective_update || consensus_vec[index])
// 				{
// 					m_feature_knowledges[index]->updateQ(state1, action1, reward, state2, action2);
// 				}
// 				else if(knob::le_featurewise_selective_update && !consensus_vec[index])
// 				{
// 					stats.learn.su_skip[index]++;
// 				}
// 			}
// 		}

// 		if(knob::le_featurewise_enable_dynamic_weight)
// 		{
// 			adjust_feature_weights(consensus_vec, reward_type);
// 		}
// 	}
// 	else
// 	{
// 		printf("learning_type %s policy %s not supported!\n", MapLearningTypeString(m_type), MapPolicyString(m_policy));
// 		assert(false);
// 	}
// }

void LearningEngineFeaturewise::learn(State *state1, uint32_t action1, int32_t reward,
									  State *state2, uint32_t action2,
									  vector<bool> consensus_vec, RewardType reward_type)
{
	stats.learn.called++;
	if (m_type == LearningType::SARSA && m_policy == Policy::EGreedy)
	{
		for (uint32_t index = 0; index < NumFeatureTypes; ++index)
		{
			if (m_feature_knowledges[index])
			{
				if (!knob::le_featurewise_selective_update || consensus_vec[index])
				{
					// 原有：更新 Q 值
					m_feature_knowledges[index]->updateQ(state1, action1, reward, state2, action2);

					// 【新增】调用 update_weight() 根据 reward 动态调整该特征的权重
					m_feature_knowledges[index]->update_weight(reward);
				}
				else if (knob::le_featurewise_selective_update && !consensus_vec[index])
				{
					stats.learn.su_skip[index]++;
				}
			}
		}

		if (knob::le_featurewise_enable_dynamic_weight)
		{
			adjust_feature_weights(consensus_vec, reward_type);
		}
	}
	else
	{
		printf("learning_type %s policy %s not supported!\n",
			   MapLearningTypeString(m_type), MapPolicyString(m_policy));
		assert(false);
	}
}

uint32_t LearningEngineFeaturewise::getMaxAction(State *state, float &max_q, float &max_to_avg_q_ratio, vector<bool> &consensus_vec)
{
	float max_q_value = 0.0, q_value = 0.0, total_q_value = 0.0;
	uint32_t selected_action = 0, init_index = 0;

	bool fallback = do_fallback(state);

	if (!fallback)
	{
		max_q_value = consultQ(state, 0);
		total_q_value += max_q_value;
		init_index = 1;
	}
	for (uint32_t action = init_index; action < m_actions; ++action)
	{
		q_value = consultQ(state, action);
		total_q_value += q_value;
		if (q_value > max_q_value)
		{
			max_q_value = q_value;
			selected_action = action;
		}
	}
	if (fallback && max_q_value == 0.0)
	{
		stats.action.fallback++;
	}

	/* max to avg ratio calculation */
	float avg_q_value = total_q_value / m_actions;
	if ((max_q_value > 0 && avg_q_value > 0) || (max_q_value < 0 && avg_q_value < 0))
	{
		max_to_avg_q_ratio = abs(max_q_value) / abs(avg_q_value) - 1;
	}
	else
	{
		max_to_avg_q_ratio = (max_q_value - avg_q_value) / abs(avg_q_value);
	}
	if (max_q_value < knob::le_featurewise_max_q_thresh * m_max_q_value)
	{
		max_to_avg_q_ratio = 0.0;
	}
	max_q = max_q_value;

	action_selection_consensus(state, selected_action, consensus_vec);

	return selected_action;
}

// float LearningEngineFeaturewise::consultQ(State *state, uint32_t action)
// {
// 	assert(action < m_actions);
// 	float q_value = 0.0;
// 	float max = -1000000000.0;

// 	/* pool Q-value accross all feature tables */
// 	for (uint32_t index = 0; index < NumFeatureTypes; ++index)
// 	{
// 		if (m_feature_knowledges[index])
// 		{
// 			if (knob::le_featurewise_pooling_type == 1) /* sum pooling */
// 			{
// 				q_value += m_feature_knowledges[index]->retrieveQ(state, action);
// 			}
// 			else if (knob::le_featurewise_pooling_type == 2) /* max pooling */
// 			{
// 				float tmp = m_feature_knowledges[index]->retrieveQ(state, action);
// 				if (tmp >= max)
// 				{
// 					max = tmp;
// 					q_value = tmp;
// 				}
// 			}
// 			else
// 			{
// 				assert(false);
// 			}
// 		}
// 	}
	
// 	return q_value;
// }
float LearningEngineFeaturewise::consultQ(State *state, uint32_t action)
{
    assert(action < m_actions);
    float q_value = 0.0;
    float max = -1000000000.0;
    

    // 遍历所有特征知识库
    for (uint32_t index = 0; index < NumFeatureTypes; ++index)
    {
        if (m_feature_knowledges[index])
        {
            float current_q = m_feature_knowledges[index]->retrieveQ(state, action);
            if (knob::le_featurewise_pooling_type == 1) /* sum pooling */
            {
                q_value += current_q;
            }
            else if (knob::le_featurewise_pooling_type == 2) /* max pooling */
            {
                if (current_q >= max)
                {
                    max = current_q;
                    q_value = current_q;
                }
            }
            else
            {
                assert(false);
            }

        }
    }
    
    return q_value;
}

void LearningEngineFeaturewise::dump_stats()
{
	Scooby *scooby = (Scooby *)m_parent;
	fprintf(stdout, "learning_engine_featurewise.action.called %lu\n", stats.action.called);
	fprintf(stdout, "learning_engine_featurewise.action.explore %lu\n", stats.action.explore);
	fprintf(stdout, "learning_engine_featurewise.action.exploit %lu\n", stats.action.exploit);
	fprintf(stdout, "learning_engine_featurewise.action.fallback %lu\n", stats.action.fallback);
	fprintf(stdout, "learning_engine_featurewise.action.dyn_fallback_saved_bw %lu\n", stats.action.dyn_fallback_saved_bw);
	fprintf(stdout, "learning_engine_featurewise.action.dyn_fallback_saved_bw_acc %lu\n", stats.action.dyn_fallback_saved_bw_acc);
	for (uint32_t action = 0; action < m_actions; ++action)
	{
		fprintf(stdout, "learning_engine_featurewise.action.index_%d_explored %lu\n", scooby->getAction(action), stats.action.dist[action][0]);
		fprintf(stdout, "learning_engine_featurewise.action.index_%d_exploited %lu\n", scooby->getAction(action), stats.action.dist[action][1]);
	}
	fprintf(stdout, "learning_engine_featurewise.learn.called %lu\n", stats.learn.called);
	for (uint32_t index = 0; index < NumFeatureTypes; ++index)
	{
		if (m_feature_knowledges[index])
		{
			fprintf(stdout, "learning_engine_featurewise.learn.su_skip_%s %lu\n", FeatureKnowledge::getFeatureString((FeatureType)index).c_str(), stats.learn.su_skip[index]);
		}
	}
	fprintf(stdout, "\n");

	/* plot histogram */
	for (uint32_t index = 0; index < m_q_value_histogram.size(); ++index)
	{
		fprintf(stdout, "learning_engine_featurewise.q_value_histogram.bucket_%u %lu\n", index, m_q_value_histogram[index]);
	}
	fprintf(stdout, "\n");

	/* consensus stats */
	fprintf(stdout, "learning_engine_featurewise.consensus.total %lu\n", stats.consensus.total);
	for (uint32_t index = 0; index < NumFeatureTypes; ++index)
	{
		if (m_feature_knowledges[index])
		{
			fprintf(stdout, "learning_engine_featurewise.consensus.feature_align_%s %lu\n", FeatureKnowledge::getFeatureString((FeatureType)index).c_str(), stats.consensus.feature_align_dist[index]);
			fprintf(stdout, "learning_engine_featurewise.consensus.feature_align_%s_ratio %0.2f\n", FeatureKnowledge::getFeatureString((FeatureType)index).c_str(), (float)stats.consensus.feature_align_dist[index] / stats.consensus.total);
		}
	}
	fprintf(stdout, "learning_engine_featurewise.consensus.feature_align_all %lu\n", stats.consensus.feature_align_all);
	fprintf(stdout, "learning_engine_featurewise.consensus.feature_align_all_ratio %0.2f\n", (float)stats.consensus.feature_align_all / stats.consensus.total);
	fprintf(stdout, "\n");

	/* weight stats */
	for (uint32_t index = 0; index < NumFeatureTypes; ++index)
	{
		if (m_feature_knowledges[index])
		{
			fprintf(stdout, "learning_engine_featurewise.feature_%s_min_weight %0.8f\n", FeatureKnowledge::getFeatureString((FeatureType)index).c_str(), m_feature_knowledges[index]->get_min_weight());
			fprintf(stdout, "learning_engine_featurewise.feature_%s_max_weight %0.8f\n", FeatureKnowledge::getFeatureString((FeatureType)index).c_str(), m_feature_knowledges[index]->get_max_weight());
		}
	}

	/* score plotting */
	if (knob::le_featurewise_enable_trace && knob::le_featurewise_enable_score_plot)
	{
		plot_scores();
	}
}

void LearningEngineFeaturewise::gather_stats(float max_q, float max_to_avg_q_ratio)
{
	float high = 0.0, low = 0.0;
	for (uint32_t index = 0; index < m_q_value_buckets.size(); ++index)
	{
		low = index ? m_q_value_buckets[index - 1] : -1000000000;
		high = (index < m_q_value_buckets.size() - 1) ? m_q_value_buckets[index + 1] : +1000000000;
		if (max_q >= low && max_q < high)
		{
			m_q_value_histogram[index]++;
			break;
		}
	}
}

/* consensus stats: whether each feature's maxAction decision aligns with the final selected action */
void LearningEngineFeaturewise::action_selection_consensus(State *state, uint32_t selected_action, vector<bool> &consensus_vec)
{
	stats.consensus.total++;
	bool all_features_align = true;
	for (uint32_t index = 0; index < NumFeatureTypes; ++index)
	{
		if (m_feature_knowledges[index])
		{
			if (m_feature_knowledges[index]->getMaxAction(state) == selected_action)
			{
				stats.consensus.feature_align_dist[index]++;
				consensus_vec[index] = true;
			}
			else
			{
				all_features_align = false;
			}
		}
	}
	if (all_features_align)
	{
		stats.consensus.feature_align_all++;
	}
}

void LearningEngineFeaturewise::adjust_feature_weights(vector<bool> consensus_vec, RewardType reward_type)
{
	assert(consensus_vec.size() == NumFeatureTypes);

	if (knob::le_featurewise_disable_adjust_weight_all_features_align && (uint32_t)std::accumulate(consensus_vec.begin(), consensus_vec.end(), 0) == knob::le_featurewise_active_features.size())
	{
		return;
	}

	for (uint32_t index = 0; index < NumFeatureTypes; ++index)
	{
		if (consensus_vec[index]) /* the feature algined with the overall decision */
		{
			assert(m_feature_knowledges[index]);

			/* if the prefetch decision is indeed proven to be correct
			 * increase the weight of the feature, else decrease it */
			if (isRewardCorrect(reward_type))
			{
				m_feature_knowledges[index]->increase_weight();
			}
			else if (isRewardIncorrect(reward_type))
			{
				m_feature_knowledges[index]->decrease_weight();
			}
		}
	}
}

bool LearningEngineFeaturewise::do_fallback(State *state)
{
	/* set fallback to whatever the knob value is, if dynamic fallback is turned off */
	if (!knob::le_featurewise_enable_dyn_action_fallback)
	{
		return knob::le_featurewise_enable_action_fallback;
	}

	if (state->is_high_bw)
	{
		stats.action.dyn_fallback_saved_bw++;
		return false;
	}
	else if (state->bw_level >= knob::le_featurewise_bw_acc_check_level && state->acc_level <= knob::le_featurewise_acc_thresh)
	{
		stats.action.dyn_fallback_saved_bw_acc++;
		return false;
	}

	return true;
}

void LearningEngineFeaturewise::plot_scores()
{
	Scooby *scooby = (Scooby *)m_parent;

	char *script_file = (char *)malloc(16 * sizeof(char));
	assert(script_file);
	gen_random(script_file, 16);
	FILE *script = fopen(script_file, "w");
	assert(script);

	/* define line styles */
	stringstream ss;
	ss << "set style line 1 lt 1 lc rgb \"#A00000\" lw 1.4\n"
	   << "set style line 2 lt 1 lc rgb \"#00A000\" lw 1.4\n"
	   << "set style line 3 lt 1 lc rgb \"#5060D0\" lw 1.4\n"
	   << "set style line 4 lt 1 lc rgb \"#0000A0\" lw 1.4\n"
	   << "set style line 5 lt 1 lc rgb \"#D0D000\" lw 1.4\n"
	   << "set style line 6 lt 1 lc rgb \"#00D0D0\" lw 1.4\n"
	   << "set style line 7 lt 1 lc rgb \"#B200B2\" lw 1.4";
	string line_styles = ss.str();

	fprintf(script, "%s\n\n", line_styles.c_str());
	fprintf(script, "set term pdf size 5,4 enhanced color font 'SVBasicManual, 20' lw 2\n");
	fprintf(script, "set datafile sep ','\n");
	fprintf(script, "set output '%s'\n", knob::le_featurewise_plot_file_name.c_str());
	fprintf(script, "set border linewidth 1.2\n");
	fprintf(script, "set xlabel \"time\"\n");
	fprintf(script, "set ylabel \"q-value\"\n");
	fprintf(script, "set format x '%%.0s%%c'\n");
	fprintf(script, "set grid xtics\n");
	fprintf(script, "set grid ytics\n");
	// fprintf(script, "set key right bottom Left box 3\n");
	// fprintf(script, "set key outside center bottom horizontal box\n");
	fprintf(script, "set key outside center top horizontal font \",18\"\n");
	fprintf(script, "plot ");
	for (uint32_t index = 0; index < knob::le_featurewise_plot_actions.size(); ++index)
	{
		if (index)
			fprintf(script, ", ");
		int action = scooby->getAction(knob::le_featurewise_plot_actions[index]);
		stringstream ss;
		if (action > 0)
			ss << "+" << action;
		else
			ss << "-" << action;
		fprintf(script, "'%s' using 1:%u with lines ls %u title \"(%s)\"",
				knob::le_featurewise_trace_file_name.c_str(),
				(knob::le_featurewise_plot_actions[index] + 2),
				(index + 1),
				ss.str().c_str());
	}
	fprintf(script, "\n");
	fclose(script);

	std::string cmd = "gnuplot " + std::string(script_file);
	system(cmd.c_str());

	if (knob::le_featurewise_remove_plot_script)
	{
		std::string cmd2 = "rm " + std::string(script_file);
		system(cmd2.c_str());
	}
}

void LearningEngineFeaturewise::handlePhaseChange(PhaseLevel level)
{
    switch (level)
    {
    case PhaseLevel::MEDIUM:
        // 1) 如果是MEDIUM相位变化，仅重置所有权重，不重置上下文
        for (uint32_t index = 0; index < NumFeatureTypes; ++index) {
            if (m_feature_knowledges[index]) {
                // 重置该特征知识库的10组权重为1.0
                m_feature_knowledges[index]->resetAllWeightSets(1.0f);
            }
        }
        printf("[DEBUG] handlePhaseChange -> MEDIUM reset (weights only)\n");
        break;

    case PhaseLevel::MAJOR:
        // 2) 如果是MAJOR，就权重和上下文都一起重置
        for (uint32_t index = 0; index < NumFeatureTypes; ++index) {
            if (m_feature_knowledges[index]) {
                m_feature_knowledges[index]->resetAllWeightSets(1.0f);
                m_feature_knowledges[index]->resetAllContexts(128);
            }
        }
        printf("[DEBUG] handlePhaseChange -> MAJOR reset (weights & contexts)\n");
        break;

    case PhaseLevel::NONE:
        // 3) 正常不会进到这里, 因为Scooby那边检测到 != NONE 才会调用
        // 可以放个保护或调试输出
        printf("[DEBUG] handlePhaseChange -> NO CHANGE\n");
        break;
    }

    // 如果您还想检查带宽(比如 is_high_bw() )，可在这里写额外逻辑:
    /*
    if (is_high_bw()) {
        // do something else ...
    }
    */

    // 4) 调试计数器 (可选)
    static int debug_phase_counter = 0;
    if (debug_phase_counter < 100) {
        printf("[DEBUG] handlePhaseChange() invoked with level = %d\n", (int)level);
        debug_phase_counter++;
    }
}
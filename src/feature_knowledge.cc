#include <stdio.h>
#include <assert.h>
#include "feature_knowledge.h"
#include "feature_knowledge_helper.h"
#include <cmath>

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
	extern bool le_featurewise_enable_action_fallback;
	extern bool le_featurewise_enable_trace;
	extern uint32_t le_featurewise_trace_feature_type;
	extern string le_featurewise_trace_feature;
	extern uint32_t le_featurewise_trace_interval;
	extern uint32_t le_featurewise_trace_record_count;
	extern std::string le_featurewise_trace_file_name;
}

const char *MapFeatureTypeString[] = {"PC", "Offset", "Delta", "Address", "PC_Offset", "PC_Address", "PC_Page", "PC_Path", "Delta_Path", "Offset_Path", "PC_Delta", "PC_Offset_Delta", "Page", "PC_Path_Offset", "PC_Path_Offset_Path", "PC_Path_Delta", "PC_Path_Delta_Path", "PC_Path_Offset_Path_Delta_Path", "Offset_Path_PC", "Delta_Path_PC"};

string FeatureKnowledge::getFeatureString(FeatureType feature)
{
	assert(feature < FeatureType::NumFeatureTypes);
	return MapFeatureTypeString[(uint32_t)feature];
}

// FeatureKnowledge::FeatureKnowledge(FeatureType feature_type, float alpha, float gamma, uint32_t actions, float weight, float weight_gradient, uint32_t num_tilings, uint32_t num_tiles, bool zero_init, uint32_t hash_type, int32_t enable_tiling_offset)
// 	: m_feature_type(feature_type), m_alpha(alpha), m_gamma(gamma), m_actions(actions), m_weight(weight), m_weight_gradient(weight_gradient), m_hash_type(hash_type), m_num_tilings(num_tilings), m_num_tiles(num_tiles), m_enable_tiling_offset(enable_tiling_offset ? true : false)
// {
// 	assert(m_num_tilings <= FK_MAX_TILINGS);
// 	assert(m_num_tilings == 1 || m_enable_tiling_offset); /* enforce the use of tiling offsets in case of multiple tilings */

// 	/* create Q-table */
// 	m_qtable = (float***)calloc(m_num_tilings, sizeof(float**));
// 	assert(m_qtable);
// 	for(uint32_t tiling = 0; tiling < m_num_tilings; ++tiling)
// 	{
// 		m_qtable[tiling] = (float**)calloc(m_num_tiles, sizeof(float*));
// 		assert(m_qtable[tiling]);
// 		for(uint32_t tile = 0; tile < m_num_tiles; ++tile)
// 		{
// 			m_qtable[tiling][tile] = (float*)calloc(m_actions, sizeof(float));
// 			assert(m_qtable[tiling][tile]);
// 		}
// 	}

// 	/* init Q-table */
// 	if(zero_init)
// 	{
// 		m_init_value = 0;
// 	}
// 	else
// 	{
// 		m_init_value = (float)1ul/(1-gamma);
// 	}
// 	for(uint32_t tiling = 0; tiling < m_num_tilings; ++tiling)
// 	{
// 		for(uint32_t tile = 0; tile < m_num_tiles; ++tile)
// 		{
// 			for(uint32_t action = 0; action < m_actions; ++action)
// 			{
// 				m_qtable[tiling][tile][action] = m_init_value;
// 			}
// 		}
// 	}

// 	min_weight = 1000000;
// 	max_weight = 0;

// 	/* reward tracing */
// 	if(knob::le_featurewise_enable_trace)
// 	{
// 		trace_interval = 0;
// 		trace_timestamp = 0;
// 		trace_record_count = 0;
// 		trace = fopen(knob::le_featurewise_trace_file_name.c_str(), "w");
// 		assert(trace);
// 	}
// }
FeatureKnowledge::FeatureKnowledge(FeatureType feature_type, float alpha, float gamma, uint32_t actions,
								   float init_weight, float weight_gradient, uint32_t num_tilings, uint32_t num_tiles,
								   bool zero_init, uint32_t hash_type, int32_t enable_tiling_offset)
	: m_feature_type(feature_type), m_alpha(alpha), m_gamma(gamma), m_actions(actions),
	  /* 原来的 m_weight(weight) 已删除，改用多组权重存储 */
	  m_weight_gradient(weight_gradient), m_hash_type(hash_type),
	  m_num_tilings(num_tilings), m_num_tiles(num_tiles), m_enable_tiling_offset(enable_tiling_offset ? true : false)
{
	assert(m_num_tilings <= FK_MAX_TILINGS);
	assert(m_num_tilings == 1 || m_enable_tiling_offset); /* enforce the use of tiling offsets in case of multiple tilings */

	/* create Q-table */
	m_qtable = (float ***)calloc(m_num_tilings, sizeof(float **));
	assert(m_qtable);
	for (uint32_t tiling = 0; tiling < m_num_tilings; ++tiling)
	{
		m_qtable[tiling] = (float **)calloc(m_num_tiles, sizeof(float *));
		assert(m_qtable[tiling]);
		for (uint32_t tile = 0; tile < m_num_tiles; ++tile)
		{
			m_qtable[tiling][tile] = (float *)calloc(m_actions, sizeof(float));
			assert(m_qtable[tiling][tile]);
		}
	}

	/* init Q-table */
	if (zero_init)
	{
		m_init_value = 0;
	}
	else
	{
		m_init_value = (float)1ul / (1 - gamma);
	}
	for (uint32_t tiling = 0; tiling < m_num_tilings; ++tiling)
	{
		for (uint32_t tile = 0; tile < m_num_tiles; ++tile)
		{
			for (uint32_t action = 0; action < m_actions; ++action)
			{
				m_qtable[tiling][tile][action] = m_init_value;
			}
		}
	}

	min_weight = 1000000;
	max_weight = 0;

	/* reward tracing */
	if (knob::le_featurewise_enable_trace)
	{
		trace_interval = 0;
		trace_timestamp = 0;
		trace_record_count = 0;
		trace = fopen(knob::le_featurewise_trace_file_name.c_str(), "w");
		assert(trace);
	}

	// =============================
	// 【新增】初始化权重集和上下文向量
	// =============================
	// 初始化多组权重数组，每组权重初始值为传入的 init_weight
	for (uint32_t i = 0; i < NUM_WEIGHT_SETS; ++i)
	{
		m_weights[i] = init_weight;
	}
	// 初始化上下文向量，简单设置为中性值（全部设为128）
	for (uint32_t i = 0; i < NUM_WEIGHT_SETS; ++i)
	{
		for (uint32_t j = 0; j < 6; ++j)
		{
			m_contexts[i].features[j] = 128;
		}
	}
	// 初始时选中第0组权重
	m_current_weight_set = 0;
}

FeatureKnowledge::~FeatureKnowledge()
{

}

float FeatureKnowledge::getQ(uint32_t tiling, uint32_t tile_index, uint32_t action)
{
	assert(tiling < m_num_tilings);
	assert(tile_index < m_num_tiles);
	assert(action < m_actions);
	return m_qtable[tiling][tile_index][action];
}

void FeatureKnowledge::setQ(uint32_t tiling, uint32_t tile_index, uint32_t action, float value)
{
	assert(tiling < m_num_tilings);
	assert(tile_index < m_num_tiles);
	assert(action < m_actions);
	m_qtable[tiling][tile_index][action] = value;
}

// void FeatureKnowledge::update_weight(int32_t reward)
// {
// 	static int debug_counter = 0; // 限制调试输出次数
// 	float learning_rate = 0.001f; // 固定学习率（可根据需要改为 knobs 参数）
// 	float old_weight = m_weight;
// 	m_weight += learning_rate * reward;

// 	// 限制 m_weight 的范围在 [0, 10]
// 	if (m_weight < 0.0f)
// 		m_weight = 0.0f;
// 	if (m_weight > 10.0f)
// 		m_weight = 10.0f;

// 	if (debug_counter < 100)
// 	{
// 		printf("[DEBUG] update_weight: reward = %d, old weight = %.4f, new weight = %.4f\n", reward, old_weight, m_weight);
// 		debug_counter++;
// 	}
// }
void FeatureKnowledge::update_weight(int32_t reward)
{
    // 这里的学习率设为固定值，可以改为 knobs 参数
    float learning_rate = 0.001f;
    float old_weight = m_weights[m_current_weight_set];
    m_weights[m_current_weight_set] = old_weight + learning_rate * (reward - old_weight);

    // 限制权重范围，例如 [0, 10]
    if(m_weights[m_current_weight_set] < 0.0f)
        m_weights[m_current_weight_set] = 0.0f;
    if(m_weights[m_current_weight_set] > 10.0f)
        m_weights[m_current_weight_set] = 10.0f;

    
}


// float FeatureKnowledge::retrieveQ(State *state, uint32_t action)
// {
// 	uint32_t tile_index = 0;
// 	float q_value = 0.0;

// 	for (uint32_t tiling = 0; tiling < m_num_tilings; ++tiling)
// 	{
// 		tile_index = get_tile_index(tiling, state);
// 		q_value += getQ(tiling, tile_index, action);
// 	}

// 	return m_weight * q_value;
// }
float FeatureKnowledge::retrieveQ(State *state, uint32_t action)
{
    uint32_t tile_index = 0;
    float q_value = 0.0;

    for (uint32_t tiling = 0; tiling < m_num_tilings; ++tiling)
    {
        tile_index = get_tile_index(tiling, state);
        q_value += getQ(tiling, tile_index, action);
    }
    // 使用当前选中的权重集
    return m_weights[m_current_weight_set] * q_value;
}

void FeatureKnowledge::updateQ(State *state1, uint32_t action1, int32_t reward, State *state2, uint32_t action2)
{
	uint32_t tile_index1 = 0, tile_index2 = 0;
	float Qsa1, Qsa2, Qsa1_old;

	float QSa1_old_overall = retrieveQ(state1, action1);
	float QSa2_old_overall = retrieveQ(state2, action2);

	for (uint32_t tiling = 0; tiling < m_num_tilings; ++tiling)
	{
		tile_index1 = get_tile_index(tiling, state1);
		tile_index2 = get_tile_index(tiling, state2);
		Qsa1 = getQ(tiling, tile_index1, action1);
		Qsa2 = getQ(tiling, tile_index2, action2);
		Qsa1_old = Qsa1;
		/* SARSA */
		Qsa1 = Qsa1 + m_alpha * ((float)reward + m_gamma * Qsa2 - Qsa1);
		setQ(tiling, tile_index1, action1, Qsa1);
		MYLOG("<tiling %u> Q(%s,%u) = %0.2f, R = %d, Q(%s,%u) = %0.2f, Q(%s,%u) = %0.2f", tiling, state1->to_string().c_str(), action1, Qsa1_old, reward, state2->to_string().c_str(), action2, Qsa2, state1->to_string().c_str(), action1, Qsa1);
	}

	float QSa1_new_overall = retrieveQ(state1, action1);
	MYLOG("<feature %s> Q(%s,%u) = %0.2f, R = %d, Q(%s,%u) = %0.2f, Q(%s,%u) = %0.2f", getFeatureString(m_feature_type).c_str(), state1->to_string().c_str(), action1, QSa1_old_overall, reward, state2->to_string().c_str(), action2, QSa2_old_overall, state1->to_string().c_str(), action1, QSa1_new_overall);

	/* tracing Q-values */
	if (knob::le_featurewise_enable_trace && knob::le_featurewise_trace_feature_type == m_feature_type && !knob::le_featurewise_trace_feature.compare(get_feature_string(state1)) && trace_interval++ == knob::le_featurewise_trace_interval && trace_record_count < knob::le_featurewise_trace_record_count)
	{
		dump_feature_trace(state1);
		trace_interval = 0;
		trace_record_count++;
	}
}

uint32_t FeatureKnowledge::get_tile_index(uint32_t tiling, State *state)
{
	uint64_t pc = state->pc;
	uint64_t page = state->page;
	uint64_t address = state->address;
	uint32_t offset = state->offset;
	int32_t delta = state->delta;
	uint32_t delta_path = state->local_delta_sig2;
	uint32_t pc_path = state->local_pc_sig;
	uint32_t offset_path = state->local_offset_sig;

	switch (m_feature_type)
	{
	case F_PC:
		return process_PC(tiling, pc);
	case F_Offset:
		return process_offset(tiling, offset);
	case F_Delta:
		return process_delta(tiling, delta);
	case F_Address:
		return process_address(tiling, address);
	case F_PC_Offset:
		return process_PC_offset(tiling, pc, offset);
	case F_PC_Address:
		return process_PC_address(tiling, pc, address);
	case F_PC_Page:
		return process_PC_page(tiling, pc, page);
	case F_PC_Path:
		return process_PC_path(tiling, pc_path);
	case F_Delta_Path:
		return process_delta_path(tiling, delta_path);
	case F_Offset_Path:
		return process_offset_path(tiling, offset_path);
	case F_PC_Delta:
		return process_PC_delta(tiling, pc, delta);
	case F_PC_Offset_Delta:
		return process_PC_offset_delta(tiling, pc, offset, delta);
	case F_Page:
		return process_Page(tiling, page);
	case F_PC_Path_Offset:
		return process_PC_Path_Offset(tiling, pc_path, offset);
	case F_PC_Path_Offset_Path:
		return process_PC_Path_Offset_Path(tiling, pc_path, offset_path);
	case F_PC_Path_Delta:
		return process_PC_Path_Delta(tiling, pc_path, delta);
	case F_PC_Path_Delta_Path:
		return process_PC_Path_Delta_Path(tiling, pc_path, delta_path);
	case F_PC_Path_Offset_Path_Delta_Path:
		return process_PC_Path_Offset_Path_Delta_Path(tiling, pc_path, offset_path, delta_path);
	case F_Offset_Path_PC:
		return process_Offset_Path_PC(tiling, offset_path, pc);
	case F_Delta_Path_PC:
		return process_Delta_Path_PC(tiling, delta_path, pc);
	default:
		assert(false);
		return 0;
	}
}

// uint32_t FeatureKnowledge::getMaxAction(State *state)
// {
// 	float max_q_value = 0.0, q_value = 0.0;
// 	uint32_t selected_action = 0, init_index = 0;

// 	if(!knob::le_featurewise_enable_action_fallback)
// 	{
// 		max_q_value = retrieveQ(state, 0);
// 		init_index = 1;
// 	}

// 	for(uint32_t action = init_index; action < m_actions; ++action)
// 	{
// 		q_value = retrieveQ(state, action);
// 		if(q_value > max_q_value)
// 		{
// 			max_q_value = q_value;
// 			selected_action = action;
// 		}
// 	}
// 	return selected_action;
// }

uint32_t FeatureKnowledge::getMaxAction(State *state)
{
	float max_q_value = 0.0, q_value = 0.0;
	uint32_t selected_action = 0, init_index = 0;

	if (!knob::le_featurewise_enable_action_fallback)
	{
		max_q_value = retrieveQ(state, 0);
		init_index = 1;
	}

	for (uint32_t action = init_index; action < m_actions; ++action)
	{
		q_value = retrieveQ(state, action);
		if (q_value > max_q_value)
		{
			max_q_value = q_value;
			selected_action = action;
		}
		
	}
	return selected_action;
}

string FeatureKnowledge::get_feature_string(State *state)
{
	uint64_t pc = state->pc;
	// uint64_t page = state->page;
	// uint64_t address = state->address;
	// uint32_t offset = state->offset;
	int32_t delta = state->delta;
	// uint32_t delta_path = state->local_delta_sig2;
	// uint32_t pc_path = state->local_pc_sig;
	// uint32_t offset_path = state->local_offset_sig;

	std::stringstream ss;
	switch (m_feature_type)
	{
	case F_PC:
		ss << std::hex << pc << std::dec;
		break;
	case F_PC_Delta:
		ss << std::hex << pc << std::dec << "|" << delta;
		break;
	default:
		/* @RBERA TODO: define the rest */
		assert(false);
	}
	return ss.str();
}

void FeatureKnowledge::dump_feature_trace(State *state)
{
	trace_timestamp++;
	fprintf(trace, "%lu,", trace_timestamp);
	for (uint32_t action = 0; action < m_actions; ++action)
	{
		fprintf(trace, "%.2f,", retrieveQ(state, action));
	}
	fprintf(trace, "\n");
	fflush(trace);
}

// 根据当前上下文向量，加载最匹配的权重集
// 该函数计算当前上下文（current_context）与内部存储的 10 组上下文向量（m_contexts）的欧几里得距离
// 如果找到距离小于或等于阈值（300.0）的上下文，则认为匹配成功，并将 m_current_weight_set 设置为该权重集的索引
// 否则，调用 initialize_neutral_weight_set() 将当前权重集初始化为中性值
// void FeatureKnowledge::load_weight_set_by_context(const ContextVector &current_context)
// {
//     uint32_t best_index = 0;          // 用于存储最佳匹配的权重集索引
//     double best_distance = 1e9;       // 初始化最佳距离为一个很大的数

//     // 遍历所有权重集（共有 NUM_WEIGHT_SETS 组）
//     for (uint32_t i = 0; i < NUM_WEIGHT_SETS; ++i)
//     {
//         double distance = 0.0;        // 初始化当前权重集与当前上下文的距离为0

//         // 遍历上下文向量中的每个特征（共6个特征）
//         for (uint32_t j = 0; j < 6; ++j)
//         {
//             // 计算当前权重集的第 j 个特征与当前上下文对应特征的差值
//             int diff = (int)m_contexts[i].features[j] - (int)current_context.features[j];
//             // 将差值的平方累加
//             distance += diff * diff;
//         }
//         // 计算欧几里得距离：开平方得到真实距离
//         distance = sqrt(distance);

//         // 如果当前计算的距离小于已知最佳距离，则更新最佳距离和最佳匹配索引
//         if (distance < best_distance)
//         {
//             best_distance = distance;
//             best_index = i;
//         }
//     }
//     // 如果找到的最佳距离小于或等于阈值 300.0，则认为匹配成功
//     if (best_distance <= 300.0)
//     {
//         m_current_weight_set = best_index;  // 选择该权重集作为当前权重集
//         //printf("[DEBUG] load_weight_set_by_context: matched weight set %u with distance %.2f\n", best_index, best_distance);
//     }
//     else
//     {
//         // ===========================
//         // 【本次修改】若所有都 >300，调用 storeNewContext()
//         // 而不是仅 initialize_neutral_weight_set()
//         // ===========================
//         uint32_t new_idx = storeNewContext(current_context); // <-- 新增调用
//         m_current_weight_set = new_idx;

//         // 如果仍想保留“中性权重”逻辑，可在 storeNewContext() 内部，
//         // 已将 m_weights[new_idx] 设为1.0f。
//         // 这样下次就能匹配到它了。

//         // 如果您还想调用 initialize_neutral_weight_set()
//         // （例如给 m_current_weight_set 再来一次 1.0），也可以保留：
//         // initialize_neutral_weight_set();
//         // 但由于 storeNewContext 里通常也会赋值1.0，就可省略。
//     }
// }

// 当上下文匹配不成功时，初始化当前权重集为中性值（例如1.0）
// 这里直接将当前选中权重集 m_weights[m_current_weight_set] 设置为 1.0f
void FeatureKnowledge::initialize_neutral_weight_set()
{
    m_weights[m_current_weight_set] = 1.0f;
    //printf("[DEBUG] initialize_neutral_weight_set: set current weight to neutral value 1.0\n");
}

//新增
void FeatureKnowledge::resetAllWeightSets(float default_val)
{
    for (uint32_t i = 0; i < NUM_WEIGHT_SETS; i++) {
        m_weights[i] = default_val;
    }
	//printf("[DEBUG]）
    // 也可以把 m_current_weight_set = 0;
    // 取决于您是否想强制回到第0组
}

//新增
void FeatureKnowledge::resetAllContexts(uint8_t default_feature_val)
{
    for (uint32_t i = 0; i < NUM_WEIGHT_SETS; i++) {
        for (uint32_t j = 0; j < 6; j++) {
            m_contexts[i].features[j] = default_feature_val;
        }
    }
	//printf("[DEBUG]）
}

//新增
void FeatureKnowledge::load_weight_set_by_context(const ContextVector &current_context)
{
    uint32_t best_index = 0;
    double best_distance = 1e9;

    // 遍历现有 10 组上下文，找最小距离
    for (uint32_t i = 0; i < NUM_WEIGHT_SETS; ++i)
    {
        double distance = 0.0;
        for (uint32_t j = 0; j < 6; ++j)
        {
            int diff = (int)m_contexts[i].features[j] - (int)current_context.features[j];
            distance += diff * diff;
        }
        distance = sqrt(distance);

        if (distance < best_distance)
        {
            best_distance = distance;
            best_index = i;
        }
    }

    // --- 2) 阈值分档 (示例：300 & 500)
    if (best_distance <= 300.0)
    {
        // 正常匹配
        m_current_weight_set = best_index;
        // printf("[DEBUG] matched weight set %u dist=%.2f\n", best_index, best_distance);
    }
    else if (best_distance < 500.0)
    {
        // 中间区间 [300, 500) -> 仅用中性权重，不插入新上下文
        initialize_neutral_weight_set();
        // printf("[DEBUG] distance=%.2f <500 => use neutral mode\n", best_distance);
    }
    else
    {
        // 距离≥500 => 认为真的是一个全新上下文
        uint32_t new_idx = storeNewContext(current_context);
        m_current_weight_set = new_idx;
        // printf("[DEBUG] distance=%.2f >=500 => storeNewContext -> m_current_weight_set=%u\n", best_distance, new_idx);
    }
	//printf("[DEBUG]）
}

uint32_t FeatureKnowledge::storeNewContext(const ContextVector &ctx)
{
    // (A) 先尝试找一个“空闲”的槽位
    //     这里“空闲”定义：所有特征都 ==128
    int free_index = -1;
    for (uint32_t i = 0; i < NUM_WEIGHT_SETS; i++)
    {
        bool all_128 = true;
        for (uint32_t j = 0; j < 6; j++)
        {
            if (m_contexts[i].features[j] != 128)
            {
                all_128 = false;
                break;
            }
        }
        if (all_128)
        {
            free_index = (int)i;
            break;
        }
    }

    uint32_t chosen_index = 0;
    if (free_index >= 0)
    {
        // 有空闲槽 => 使用它
        chosen_index = (uint32_t)free_index;
    }
    else
    {
        // (B) 没有空闲槽 => 这里简单写死 chosen_index=0
        //     也可随机/按LRU等更复杂策略
        chosen_index = 0;
    }

    // (C) 把新的上下文写入 chosen_index
    m_contexts[chosen_index] = ctx;

    // (D) 初始化对应的权重，让它从中性值(1.0f)开始学习
    m_weights[chosen_index] = 1.0f;

    // 返回该索引，以便在 load_weight_set_by_context() 设置 m_current_weight_set
    return chosen_index;
}

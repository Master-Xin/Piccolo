#include "runtime/function/framework/component/animation/animation_component.h"

#include "runtime/function/animation/animation_system.h"
#include "runtime/function/framework/object/object.h"
#include <runtime/engine.h>

namespace Pilot
{
    void AnimationComponent::postLoadResource(std::weak_ptr<GObject> parent_object)
    {
        m_parent_object = parent_object;

        auto skeleton_res = AnimationManager::tryLoadSkeleton(m_animation_res.m_skeleton_file_path);

        m_skeleton.buildSkeleton(*skeleton_res);
    }

    void AnimationComponent::blend1D(float desired_ratio, BlendSpace1D* blend_state)
    {
        // 只有一个状态，不需要进行插值
        if (blend_state->m_values.size() < 2)
        {
            // no need to interpolate
            return;
        }
        // 根据参与混合的 状态 ，从信号中找到其迭代器
        const auto& key_it    = m_signal.find(blend_state->m_key);
        double      key_value = 0;

        // 如果从信号中找到需要混合的状态
        if (key_it != m_signal.end())
        {
            // 如果第二个参数是权重系数
            if (key_it->second.is_number())
            {
                key_value = key_it->second.number_value();
            }
        }
        int max_smaller = -1;
        for (auto value : blend_state->m_values)
        {
            if (value <= key_value)
            {
                max_smaller++;
            }
            else
            {
                break;
            }
        }

        for (auto& weight : blend_state->m_blend_weight)
        {
            weight = 0;
        }
        if (max_smaller == -1)
        {
            blend_state->m_blend_weight[0] = 1.0f;
        }
        else if (max_smaller == blend_state->m_values.size() - 1)
        {
            blend_state->m_blend_weight[max_smaller] = 1.0f;
        }
        else
        {
            auto l = blend_state->m_values[max_smaller];
            auto r = blend_state->m_values[max_smaller + 1];

            if (l == r)
            {
                // some error
            }

            float weight = (key_value - l) / (r - l);

            blend_state->m_blend_weight[max_smaller + 1] = weight;
            blend_state->m_blend_weight[max_smaller]     = 1 - weight;
        }
        blend(desired_ratio, blend_state);
    }
    void AnimationComponent::tick(float delta_time)
    {
        if ((m_tick_in_editor_mode == false) && g_is_editor_mode)
            return;

        // 获取当前状态机的状态
        std::string name = m_animation_fsm.getCurrentClipBaseName();

        for (auto blend_state : m_animation_res.m_clips)
        {
            // 循环查找到与当前 state 对应的 clips 资源
            if (blend_state->m_name == name)
            {
                float length        = blend_state->getLength();     // clip 的长度
                float delta_ratio   = delta_time / length;          // delta_time 占 clip 总时间的比率（ratio）
                float desired_ratio = delta_ratio + m_ratio;        // 期望进行插值操作的 ratio 点，即加上当前的 ratio 点

                // 由于 clip 是循环的，ratio 超出 1 时，需要自适应地减去整数部分
                if (desired_ratio >= 1.f)
                {
                    desired_ratio = desired_ratio - floor(desired_ratio);
                    updateSignal("clip_finish", true);      // 完成插值比率点的确认，得到要传给状态机的信号
                }
                else
                {
                    updateSignal("clip_finish", false);     // 完成插值比率点的确认，得到要传给状态机的信号
                }

                // 使用信号对状态机的状态进行更新
                // 返回的 restart 代表状态是否发生改变/重启
                bool restart = m_animation_fsm.update(m_signal);
                if (!restart)
                {
                    m_ratio = desired_ratio;
                }
                else
                {
                    m_ratio = 0;
                }
                break;
            }
        }

        // 经过前面的操作，状态机的状态已经更新，需要再读取获得新的 clip/状态 名称
        name = m_animation_fsm.getCurrentClipBaseName();
        for (auto clip : m_animation_res.m_clips)
        {
            // 循环查找到和 当前状态机状态 对应的 clip
            if (clip->m_name == name)
            {
                // 一维的混合插值操作
                if (clip.getTypeName() == "BlendSpace1D")
                {
                    auto blend_state_1d_pre = static_cast<BlendSpace1D*>(clip);
                    blend1D(m_ratio, blend_state_1d_pre);
                }
                else if (clip.getTypeName() == "BlendState")
                {
                    auto blend_state = static_cast<BlendState*>(clip);
                    blend(m_ratio, blend_state);
                }
                else if (clip.getTypeName() == "BasicClip")
                {
                    auto basic_clip = static_cast<BasicClip*>(clip);
                    animateBasicClip(m_ratio, basic_clip);
                }
                break;
            }
        }
    }

    const AnimationResult& AnimationComponent::getResult() const { return m_animation_result; }

    void AnimationComponent::animateBasicClip(float desired_ratio, BasicClip* basic_clip)
    {
        auto                       clip_data = AnimationManager::getClipData(*basic_clip);

            AnimationPose pose(clip_data.m_clip,
                               desired_ratio,
                               clip_data.m_anim_skel_map);
            m_skeleton.resetSkeleton();
            m_skeleton.applyAdditivePose(pose);
            m_skeleton.extractPose(pose);
        

        m_skeleton.applyPose(pose);
        m_animation_result = m_skeleton.outputAnimationResult();
    }
    void AnimationComponent::blend(float desired_ratio, BlendState* blend_state)
    {
        // 将每个参与混合的的比率点都设为期望混合的比率点
        for (auto& ratio : blend_state->m_blend_ratio)
        {
            ratio = desired_ratio;
        }

        // 读取所有参与渲染的状态的 clip 数据
        auto                       blendStateData = AnimationManager::getBlendStateWithClipData(*blend_state);

        // 存放每个 clip 的目标准确帧的 pose 数据
        std::vector<AnimationPose> poses;

        // 对每个 clip
        for (int i = 0; i < blendStateData.m_clip_count; i++)
        {
            // 对一个 clip，构造目标比率点处的的 pose，即位置和三维姿态的矩阵表示（需要对前后的关键帧进行插值）
            AnimationPose pose(blendStateData.m_blend_clip[i],
                               blendStateData.m_blend_weight[i],
                               blendStateData.m_blend_ratio[i],
                               blendStateData.m_blend_anim_skel_map[i]);
            // 重置骨骼的 pose
            m_skeleton.resetSkeleton();
            // 应用构造的 pose 变换矩阵，中间需要将 pose 解构成 local 坐标系下的相对变换
            m_skeleton.applyAdditivePose(pose);
            // 应用变换后，读取当前的骨骼节点的位姿表示
            m_skeleton.extractPose(pose);
            // 存入当前 clip 的数据
            poses.push_back(pose);
        }

        // 对每个 clip
        for (int i = 1; i < blendStateData.m_clip_count; i++)
        {
            // 对第 i 个 clip 的目标帧的每个骨骼的权重
            for (auto& pose : poses[i].m_weight.m_blend_weight)
            {
                // 第 i 个 clip 的每个骨骼的混合权重，都是一致的
                pose = blend_state->m_blend_weight[i];
            }

            // 循环进行混合
            poses[0].blend(poses[i]);
        }

        m_skeleton.applyPose(poses[0]);
        m_animation_result = m_skeleton.outputAnimationResult();
    }
} // namespace Pilot

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
        // ֻ��һ��״̬������Ҫ���в�ֵ
        if (blend_state->m_values.size() < 2)
        {
            // no need to interpolate
            return;
        }
        // ���ݲ����ϵ� ״̬ �����ź����ҵ��������
        const auto& key_it    = m_signal.find(blend_state->m_key);
        double      key_value = 0;

        // ������ź����ҵ���Ҫ��ϵ�״̬
        if (key_it != m_signal.end())
        {
            // ����ڶ���������Ȩ��ϵ��
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

        // ��ȡ��ǰ״̬����״̬
        std::string name = m_animation_fsm.getCurrentClipBaseName();

        for (auto blend_state : m_animation_res.m_clips)
        {
            // ѭ�����ҵ��뵱ǰ state ��Ӧ�� clips ��Դ
            if (blend_state->m_name == name)
            {
                float length        = blend_state->getLength();     // clip �ĳ���
                float delta_ratio   = delta_time / length;          // delta_time ռ clip ��ʱ��ı��ʣ�ratio��
                float desired_ratio = delta_ratio + m_ratio;        // �������в�ֵ������ ratio �㣬�����ϵ�ǰ�� ratio ��

                // ���� clip ��ѭ���ģ�ratio ���� 1 ʱ����Ҫ����Ӧ�ؼ�ȥ��������
                if (desired_ratio >= 1.f)
                {
                    desired_ratio = desired_ratio - floor(desired_ratio);
                    updateSignal("clip_finish", true);      // ��ɲ�ֵ���ʵ��ȷ�ϣ��õ�Ҫ����״̬�����ź�
                }
                else
                {
                    updateSignal("clip_finish", false);     // ��ɲ�ֵ���ʵ��ȷ�ϣ��õ�Ҫ����״̬�����ź�
                }

                // ʹ���źŶ�״̬����״̬���и���
                // ���ص� restart ����״̬�Ƿ����ı�/����
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

        // ����ǰ��Ĳ�����״̬����״̬�Ѿ����£���Ҫ�ٶ�ȡ����µ� clip/״̬ ����
        name = m_animation_fsm.getCurrentClipBaseName();
        for (auto clip : m_animation_res.m_clips)
        {
            // ѭ�����ҵ��� ��ǰ״̬��״̬ ��Ӧ�� clip
            if (clip->m_name == name)
            {
                // һά�Ļ�ϲ�ֵ����
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
        // ��ÿ�������ϵĵı��ʵ㶼��Ϊ������ϵı��ʵ�
        for (auto& ratio : blend_state->m_blend_ratio)
        {
            ratio = desired_ratio;
        }

        // ��ȡ���в�����Ⱦ��״̬�� clip ����
        auto                       blendStateData = AnimationManager::getBlendStateWithClipData(*blend_state);

        // ���ÿ�� clip ��Ŀ��׼ȷ֡�� pose ����
        std::vector<AnimationPose> poses;

        // ��ÿ�� clip
        for (int i = 0; i < blendStateData.m_clip_count; i++)
        {
            // ��һ�� clip������Ŀ����ʵ㴦�ĵ� pose����λ�ú���ά��̬�ľ����ʾ����Ҫ��ǰ��Ĺؼ�֡���в�ֵ��
            AnimationPose pose(blendStateData.m_blend_clip[i],
                               blendStateData.m_blend_weight[i],
                               blendStateData.m_blend_ratio[i],
                               blendStateData.m_blend_anim_skel_map[i]);
            // ���ù����� pose
            m_skeleton.resetSkeleton();
            // Ӧ�ù���� pose �任�����м���Ҫ�� pose �⹹�� local ����ϵ�µ���Ա任
            m_skeleton.applyAdditivePose(pose);
            // Ӧ�ñ任�󣬶�ȡ��ǰ�Ĺ����ڵ��λ�˱�ʾ
            m_skeleton.extractPose(pose);
            // ���뵱ǰ clip ������
            poses.push_back(pose);
        }

        // ��ÿ�� clip
        for (int i = 1; i < blendStateData.m_clip_count; i++)
        {
            // �Ե� i �� clip ��Ŀ��֡��ÿ��������Ȩ��
            for (auto& pose : poses[i].m_weight.m_blend_weight)
            {
                // �� i �� clip ��ÿ�������Ļ��Ȩ�أ�����һ�µ�
                pose = blend_state->m_blend_weight[i];
            }

            // ѭ�����л��
            poses[0].blend(poses[i]);
        }

        m_skeleton.applyPose(poses[0]);
        m_animation_result = m_skeleton.outputAnimationResult();
    }
} // namespace Pilot

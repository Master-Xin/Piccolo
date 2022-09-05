#include "runtime/function/animation/pose.h"

using namespace Pilot;

AnimationPose::AnimationPose() { m_reorder = false; }

AnimationPose::AnimationPose(const AnimationClip& clip, float ratio, const AnimSkelMap& animSkelMap)
{
    m_bone_indexs = animSkelMap.convert;
    m_reorder     = true;
    extractFromClip(m_bone_poses, clip, ratio);
    m_weight.m_blend_weight.resize(m_bone_poses.size());
    for (auto& weight : m_weight.m_blend_weight)
    {
        weight = 1.f;
    }

}
AnimationPose::AnimationPose(const AnimationClip& clip, const BoneBlendWeight& weight, float ratio)
{
    m_weight  = weight;
    m_reorder = false;
    extractFromClip(m_bone_poses, clip, ratio);
}
AnimationPose::AnimationPose(const AnimationClip&   clip,
                             const BoneBlendWeight& weight,
                             float                  ratio,
                             const AnimSkelMap&     animSkelMap)
{
    m_weight      = weight;
    m_bone_indexs = animSkelMap.convert;
    m_reorder     = true;
    extractFromClip(m_bone_poses, clip, ratio);
}

void AnimationPose::extractFromClip(std::vector<Transform>& bones, const AnimationClip& clip, float ratio)
{
    // �������ؽڵ��λ�����������Ĵ�С��Ϊ�� clip �ڵ�����һ��
    bones.resize(clip.node_count);

    // �����׼���ǵ� x.x ֡
    float exact_frame        = ratio * (clip.total_frame - 1);
    // ��׼֡����һ����֡����һ����֡
    int   current_frame_low  = floor(exact_frame); 
    int   current_frame_high = ceil(exact_frame);
    // �ڲ�֮ͬ֡���ֵ�ı���
    float lerp_ratio         = exact_frame - current_frame_low;

    // �Ե�ǰ clip ��ÿ�������ڵ���м���
    for (int i = 0; i < clip.node_count; i++)
    {
        // �� i �������ڵ�ĸ���֡������
        const AnimationChannel& channel = clip.node_channels[i];

        // �ֱ��λ�á���ת������ϵ����ǰ��֡�Ĳ�ֵ���õ���ǰ��׼֡�ĵ� i ��������λ��
        bones[i].m_position = Vector3::lerp(
            channel.position_keys[current_frame_low], channel.position_keys[current_frame_high], lerp_ratio);
        bones[i].m_scale    = Vector3::lerp(
            channel.scaling_keys[current_frame_low], channel.scaling_keys[current_frame_high], lerp_ratio);
        bones[i].m_rotation = Quaternion::nLerp(
            lerp_ratio, channel.rotation_keys[current_frame_low], channel.rotation_keys[current_frame_high], true);
    }
}


void AnimationPose::blend(const AnimationPose& pose)
{
    // ������ pose �ж�Ӧ�� bone �ֱ���� blend
    for (int i = 0; i < m_bone_poses.size(); i++)
    {
        auto&       bone_trans_one = m_bone_poses[i];
        const auto& bone_trans_two = pose.m_bone_poses[i];

        // �����ϵ����� bone �ĸ���Ȩ��
        float bone_weight_one = m_weight.m_blend_weight[i];
        float bone_weight_two = pose.m_weight.m_blend_weight[i];

        // ���� bone ��Ȩ�غ�
        float sum_weight = bone_weight_one + bone_weight_two;

        if (sum_weight != 0)
        {
            float cur_weight           = bone_weight_one / sum_weight;  // bone 1 �Ĺ�һ����Ȩ��
            m_weight.m_blend_weight[i] = sum_weight;                    // ������ѭ����ϣ�ÿ�λ�Ϻ���Ҫ������ϵĶ����Ȩ�ؽ��и��£��Բ�����һ�� blend
            bone_trans_one.m_position = Vector3::lerp(bone_trans_two.m_position, bone_trans_one.m_position, cur_weight);
            bone_trans_one.m_scale    = Vector3::lerp(bone_trans_two.m_scale, bone_trans_one.m_scale, cur_weight);
            bone_trans_one.m_rotation = Quaternion::sLerp(cur_weight, bone_trans_two.m_rotation, bone_trans_one.m_rotation, true);
        }
    }
}




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
    // 将骨骼关节点的位姿数据容器的大小改为和 clip 节点数量一致
    bones.resize(clip.node_count);

    // 求出精准的是第 x.x 帧
    float exact_frame        = ratio * (clip.total_frame - 1);
    // 精准帧的上一整数帧和下一整数帧
    int   current_frame_low  = floor(exact_frame); 
    int   current_frame_high = ceil(exact_frame);
    // 在不同帧之间插值的比例
    float lerp_ratio         = exact_frame - current_frame_low;

    // 对当前 clip 的每个骨骼节点进行计算
    for (int i = 0; i < clip.node_count; i++)
    {
        // 第 i 个骨骼节点的各个帧的数据
        const AnimationChannel& channel = clip.node_channels[i];

        // 分别对位置、旋转、缩放系数做前后帧的插值，得到当前精准帧的第 i 个骨骼的位姿
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
    // 对两个 pose 中对应的 bone 分别进行 blend
    for (int i = 0; i < m_bone_poses.size(); i++)
    {
        auto&       bone_trans_one = m_bone_poses[i];
        const auto& bone_trans_two = pose.m_bone_poses[i];

        // 参与混合的两个 bone 的各自权重
        float bone_weight_one = m_weight.m_blend_weight[i];
        float bone_weight_two = pose.m_weight.m_blend_weight[i];

        // 两个 bone 的权重和
        float sum_weight = bone_weight_one + bone_weight_two;

        if (sum_weight != 0)
        {
            float cur_weight           = bone_weight_one / sum_weight;  // bone 1 的归一化的权重
            m_weight.m_blend_weight[i] = sum_weight;                    // 由于是循环混合，每次混合后，需要将被混合的对象的权重进行更新，以参与下一次 blend
            bone_trans_one.m_position = Vector3::lerp(bone_trans_two.m_position, bone_trans_one.m_position, cur_weight);
            bone_trans_one.m_scale    = Vector3::lerp(bone_trans_two.m_scale, bone_trans_one.m_scale, cur_weight);
            bone_trans_one.m_rotation = Quaternion::sLerp(cur_weight, bone_trans_two.m_rotation, bone_trans_one.m_rotation, true);
        }
    }
}




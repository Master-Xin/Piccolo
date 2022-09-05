#pragma once
#include "runtime/core/math/transform.h"
#include "runtime/core/meta/reflection/reflection.h"
#include <string>
#include <vector>
namespace Pilot
{

    REFLECTION_TYPE(AnimNodeMap)
    CLASS(AnimNodeMap, Fields)
    {
        REFLECTION_BODY(AnimNodeMap);

    public:
        std::vector<std::string> convert;
    };

    REFLECTION_TYPE(AnimationChannel)
    CLASS(AnimationChannel, Fields)
    {
        REFLECTION_BODY(AnimationChannel);

    public:
        std::string             name;               // 频段名称
        std::vector<Vector3>    position_keys;      // 该骨骼的所有关键帧的位置（容器索引代表是第几帧的数据）
        std::vector<Quaternion> rotation_keys;      // 该骨骼的所有关键帧的旋转（容器索引代表是第几帧的数据）
        std::vector<Vector3>    scaling_keys;       // 该骨骼的所有关键帧的缩放系数（容器索引代表是第几帧的数据）
    };

    REFLECTION_TYPE(AnimationClip)
    CLASS(AnimationClip, Fields)
    {
        REFLECTION_BODY(AnimationClip);

    public:
        int                           total_frame {0};      // 
        int                           node_count {0};       // 骨骼数量
        std::vector<AnimationChannel> node_channels;        // clip 的各个骨骼的数据，一个骨骼就是一个channel，每个 channel 的横坐标是帧
    };

    REFLECTION_TYPE(AnimationAsset)
    CLASS(AnimationAsset, Fields)
    {
        REFLECTION_BODY(AnimationAsset);

    public:
        AnimNodeMap   node_map;
        AnimationClip clip_data;
        std::string   skeleton_file_path;
    };

} // namespace Pilot
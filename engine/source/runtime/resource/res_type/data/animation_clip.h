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
        std::string             name;               // Ƶ������
        std::vector<Vector3>    position_keys;      // �ù��������йؼ�֡��λ�ã��������������ǵڼ�֡�����ݣ�
        std::vector<Quaternion> rotation_keys;      // �ù��������йؼ�֡����ת���������������ǵڼ�֡�����ݣ�
        std::vector<Vector3>    scaling_keys;       // �ù��������йؼ�֡������ϵ�����������������ǵڼ�֡�����ݣ�
    };

    REFLECTION_TYPE(AnimationClip)
    CLASS(AnimationClip, Fields)
    {
        REFLECTION_BODY(AnimationClip);

    public:
        int                           total_frame {0};      // 
        int                           node_count {0};       // ��������
        std::vector<AnimationChannel> node_channels;        // clip �ĸ������������ݣ�һ����������һ��channel��ÿ�� channel �ĺ�������֡
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
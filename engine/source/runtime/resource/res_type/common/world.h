#pragma once
#include "runtime/core/meta/reflection/reflection.h"
#include <string>
#include <vector>
namespace Pilot
{
    REFLECTION_TYPE(WorldRes)
    CLASS(WorldRes, Fields)
    {
        REFLECTION_BODY(WorldRes);

    public:
        // world name
        std::string              m_name;        // 世界名称

        // all level urls for this world
        std::vector<std::string> m_level_urls;  // 世界中所有等级的 url "asset/level/1-1.level.json"

        // the default level for this world, which should be first loading level
        std::string m_default_level_url;        // 默认的 level 的url "asset/level/1-1.level.json"
    };
} // namespace Pilot

#pragma once

#include "runtime/function/framework/object/object_id_allocator.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace Pilot
{
    class Character;
    class GObject;
    class ObjectInstanceRes;
    class PhysicsScene;

    using LevelObjectsMap = std::unordered_map<GObjectID, std::shared_ptr<GObject>>;

    /// The main class to manage all game objects
    class Level
    {
    public:
        virtual ~Level();

        bool load(const std::string& level_res_url);
        void unload();

        bool save();

        void tick(float delta_time);

        const std::string& getLevelResUrl() const { return m_level_res_url; }

        const LevelObjectsMap& getAllGObjects() const { return m_gobjects; }

        std::weak_ptr<GObject>   getGObjectByID(GObjectID go_id) const;
        std::weak_ptr<Character> getCurrentActiveCharacter() const { return m_current_active_character; }

        GObjectID createObject(const ObjectInstanceRes& object_instance_res);
        void      deleteGObjectByID(GObjectID go_id);

        std::weak_ptr<PhysicsScene> getPhysicsScene() const { return m_physics_scene; }

    protected:
        void clear();

        bool        m_is_loaded {false};
        std::string m_level_res_url;        // level 资源的 url，用于加载运算组件 "asset/level/1-1.level.json"

        // all game objects in this level, key: object id, value: object instance
        LevelObjectsMap m_gobjects;         // level 中的所有 GO 

        std::shared_ptr<Character> m_current_active_character;      // 当前活动的角色

        std::weak_ptr<PhysicsScene> m_physics_scene;
    };
} // namespace Pilot

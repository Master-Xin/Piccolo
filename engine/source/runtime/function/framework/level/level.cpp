#include "runtime/function/framework/level/level.h"

#include "runtime/core/base/macro.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/res_type/common/level.h"

#include "runtime/engine.h"
#include "runtime/function/character/character.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/physics/physics_manager.h"
#include "runtime/function/physics/physics_scene.h"

#include <limits>

namespace Pilot
{
    Level::~Level() { clear(); }

    void Level::clear()
    {
        m_current_active_character.reset();
        m_gobjects.clear();

        ASSERT(g_runtime_global_context.m_physics_manager);
        g_runtime_global_context.m_physics_manager->deletePhysicsScene(m_physics_scene);
    }

    GObjectID Level::createObject(const ObjectInstanceRes& object_instance_res)
    {
        // 创建一个 GO 对象，得到对象的下一个 GO 的 ID
        GObjectID object_id = ObjectIDAllocator::alloc();

        // 断定 ID 没有超出非法 ID
        ASSERT(object_id != k_invalid_gobject_id);

        // 创建新的 GO，为其分配 ID（ ID 的管理由 GObjectID 类来完成）
        std::shared_ptr<GObject> gobject;
        try
        {
            gobject = std::make_shared<GObject>(object_id);
        }
        catch (const std::bad_alloc&)
        {
            LOG_FATAL("cannot allocate memory for new gobject");
        }

        // 为 GO 加载对象实例资源，也就是加载运算组件
        bool is_loaded = gobject->load(object_instance_res);
        if (is_loaded)
        {
            // 将 id 和 GO 存到 level 对象当中
            m_gobjects.emplace(object_id, gobject);
        }
        // 加载运算组件失败，打印 GO 的名称，返回非法 ID
        else
        {
            LOG_ERROR("loading object " + object_instance_res.m_name + " failed");
            return k_invalid_gobject_id;
        }

        // 加载运算组件成功后，返回对象的 ID
        return object_id;
    }

    bool Level::load(const std::string& level_res_url)  // 输入 "asset/level/1-1.level.json"
    {
        LOG_INFO("loading level: {}", level_res_url);

        // 设置 level 对象内各种资源的 url
        m_level_res_url = level_res_url;

        // 根据资源定位符，把资源数据加载出来，也就是运算的组件
        LevelRes   level_res;
        const bool is_load_success = g_runtime_global_context.m_asset_manager->loadAsset(level_res_url, level_res);
        if (is_load_success == false)
        {
            return false;
        }

        // 断定物理系统管理器是已经创建过的
        ASSERT(g_runtime_global_context.m_physics_manager);

        // 根据对象资源的重力数据，创建物理计算的场景
        m_physics_scene = g_runtime_global_context.m_physics_manager->createPhysicsScene(level_res.m_gravity);

        // 对 level 中的每个对象资源实例，创建与之对应的 GO 对象并放入 level 对象当中保存起来
        for (const ObjectInstanceRes& object_instance_res : level_res.m_objects)
        {
            createObject(object_instance_res);
        }

        // create active character
        // 对 level 中保存的各个 GO
        for (const auto& object_pair : m_gobjects)
        {
            std::shared_ptr<GObject> object = object_pair.second;
            if (object == nullptr)
                continue;

            // 找到 level_res 对应的 GO，如果是
            if (level_res.m_character_name == object->getName())
            {
                m_current_active_character = std::make_shared<Character>(object);
                break;
            }
        }

        m_is_loaded = true;

        LOG_INFO("level load succeed");

        return true;
    }

    void Level::unload()
    {
        clear();
        LOG_INFO("unload level: {}", m_level_res_url);
    }

    bool Level::save()
    {
        LOG_INFO("saving level: {}", m_level_res_url);
        LevelRes output_level_res;

        const size_t                    object_cout    = m_gobjects.size();
        std::vector<ObjectInstanceRes>& output_objects = output_level_res.m_objects;
        output_objects.resize(object_cout);

        size_t object_index = 0;
        for (const auto& id_object_pair : m_gobjects)
        {
            if (id_object_pair.second)
            {
                id_object_pair.second->save(output_objects[object_index]);
                ++object_index;
            }
        }

        const bool is_save_success =
            g_runtime_global_context.m_asset_manager->saveAsset(output_level_res, m_level_res_url);

        if (is_save_success == false)
        {
            LOG_ERROR("failed to save {}", m_level_res_url);
        }
        else
        {
            LOG_INFO("level save succeed");
        }

        return is_save_success;
    }

    void Level::tick(float delta_time)
    {
        if (!m_is_loaded)
        {
            return;
        }

        //  对每个 GO，进行 tick 循环计算
        for (const auto& id_object_pair : m_gobjects)
        {
            assert(id_object_pair.second);
            if (id_object_pair.second)
            {
                // 每个 GO 的 tick 循环
                id_object_pair.second->tick(delta_time);
            }
        }
        if (m_current_active_character && g_is_editor_mode == false)
        {
            // 如果角色是可控的状态，根据输入更新位置和角度

            m_current_active_character->tick(delta_time);
        }

        // 计算物理参数 ？

        std::shared_ptr<PhysicsScene> physics_scene = m_physics_scene.lock();
        if (physics_scene)
        {
            physics_scene->tick(delta_time);
        }
    }

    std::weak_ptr<GObject> Level::getGObjectByID(GObjectID go_id) const
    {
        auto iter = m_gobjects.find(go_id);
        if (iter != m_gobjects.end())
        {
            return iter->second;
        }

        return std::weak_ptr<GObject>();
    }

    void Level::deleteGObjectByID(GObjectID go_id)
    {
        auto iter = m_gobjects.find(go_id);
        if (iter != m_gobjects.end())
        {
            std::shared_ptr<GObject> object = iter->second;
            if (object)
            {
                if (m_current_active_character && m_current_active_character->getObjectID() == object->getID())
                {
                    m_current_active_character->setObject(nullptr);
                }
            }
        }

        m_gobjects.erase(go_id);
    }

} // namespace Pilot

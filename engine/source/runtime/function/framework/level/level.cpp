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
        // ����һ�� GO ���󣬵õ��������һ�� GO �� ID
        GObjectID object_id = ObjectIDAllocator::alloc();

        // �϶� ID û�г����Ƿ� ID
        ASSERT(object_id != k_invalid_gobject_id);

        // �����µ� GO��Ϊ����� ID�� ID �Ĺ����� GObjectID ������ɣ�
        std::shared_ptr<GObject> gobject;
        try
        {
            gobject = std::make_shared<GObject>(object_id);
        }
        catch (const std::bad_alloc&)
        {
            LOG_FATAL("cannot allocate memory for new gobject");
        }

        // Ϊ GO ���ض���ʵ����Դ��Ҳ���Ǽ����������
        bool is_loaded = gobject->load(object_instance_res);
        if (is_loaded)
        {
            // �� id �� GO �浽 level ������
            m_gobjects.emplace(object_id, gobject);
        }
        // �����������ʧ�ܣ���ӡ GO �����ƣ����طǷ� ID
        else
        {
            LOG_ERROR("loading object " + object_instance_res.m_name + " failed");
            return k_invalid_gobject_id;
        }

        // ������������ɹ��󣬷��ض���� ID
        return object_id;
    }

    bool Level::load(const std::string& level_res_url)  // ���� "asset/level/1-1.level.json"
    {
        LOG_INFO("loading level: {}", level_res_url);

        // ���� level �����ڸ�����Դ�� url
        m_level_res_url = level_res_url;

        // ������Դ��λ��������Դ���ݼ��س�����Ҳ������������
        LevelRes   level_res;
        const bool is_load_success = g_runtime_global_context.m_asset_manager->loadAsset(level_res_url, level_res);
        if (is_load_success == false)
        {
            return false;
        }

        // �϶�����ϵͳ���������Ѿ���������
        ASSERT(g_runtime_global_context.m_physics_manager);

        // ���ݶ�����Դ���������ݣ������������ĳ���
        m_physics_scene = g_runtime_global_context.m_physics_manager->createPhysicsScene(level_res.m_gravity);

        // �� level �е�ÿ��������Դʵ����������֮��Ӧ�� GO ���󲢷��� level �����б�������
        for (const ObjectInstanceRes& object_instance_res : level_res.m_objects)
        {
            createObject(object_instance_res);
        }

        // create active character
        // �� level �б���ĸ��� GO
        for (const auto& object_pair : m_gobjects)
        {
            std::shared_ptr<GObject> object = object_pair.second;
            if (object == nullptr)
                continue;

            // �ҵ� level_res ��Ӧ�� GO�������
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

        //  ��ÿ�� GO������ tick ѭ������
        for (const auto& id_object_pair : m_gobjects)
        {
            assert(id_object_pair.second);
            if (id_object_pair.second)
            {
                // ÿ�� GO �� tick ѭ��
                id_object_pair.second->tick(delta_time);
            }
        }
        if (m_current_active_character && g_is_editor_mode == false)
        {
            // �����ɫ�ǿɿص�״̬�������������λ�úͽǶ�

            m_current_active_character->tick(delta_time);
        }

        // ����������� ��

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

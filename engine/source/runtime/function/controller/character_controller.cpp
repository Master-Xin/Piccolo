#include "runtime/function/controller/character_controller.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/framework/component/motor/motor_component.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/physics/physics_scene.h"

namespace Pilot
{
    CharacterController::CharacterController(const Capsule& capsule) : m_capsule(capsule)
    {
        m_rigidbody_shape                                    = RigidBodyShape();
        m_rigidbody_shape.m_geometry                         = PILOT_REFLECTION_NEW(Capsule);
        *static_cast<Capsule*>(m_rigidbody_shape.m_geometry) = m_capsule;

        m_rigidbody_shape.m_type = RigidBodyShapeType::capsule;

        Quaternion orientation;
        orientation.fromAngleAxis(Radian(Degree(90.f)), Vector3::UNIT_X);

        m_rigidbody_shape.m_local_transform =
            Transform(
                Vector3(0, 0, capsule.m_half_height + capsule.m_radius),
                orientation,
                Vector3::UNIT_SCALE);
    }

    Vector3 CharacterController::move(const Vector3& current_position, const Vector3& displacement)
    {
        std::shared_ptr<PhysicsScene> physics_scene =
            g_runtime_global_context.m_world_manager->getCurrentActivePhysicsScene().lock();
        ASSERT(physics_scene);

        std::vector<PhysicsHitInfo> hits;

        Transform world_transform = Transform(
            current_position + 0.1f * Vector3::UNIT_Z,
            Quaternion::IDENTITY,
            Vector3::UNIT_SCALE);

        Vector3 vertical_displacement   = displacement.z * Vector3::UNIT_Z;
        Vector3 horizontal_displacement = Vector3(displacement.x, displacement.y, 0.f);

        Vector3 vertical_direction   = vertical_displacement.normalisedCopy();
        Vector3 horizontal_direction = horizontal_displacement.normalisedCopy();

        Vector3 final_position = current_position;

        m_is_touch_ground = physics_scene->sweep(
            m_rigidbody_shape, world_transform.getMatrix(), Vector3::NEGATIVE_UNIT_Z, 0.105f, hits);

        hits.clear();

        world_transform.m_position -= 0.1f * Vector3::UNIT_Z;

        // vertical pass
        if (physics_scene->sweep(
            m_rigidbody_shape,
            world_transform.getMatrix(),
            vertical_direction,
            vertical_displacement.length(),
            hits))
        {
            final_position += hits[0].hit_distance * vertical_direction;
        }
        else
        {
            final_position += vertical_displacement;
        }

        hits.clear();


        // side pass
        // 水平方向有障碍物
        if (physics_scene->sweep(
            m_rigidbody_shape,
            world_transform.getMatrix(),
            horizontal_direction,
            horizontal_displacement.length(),
            hits))
        {
            // 如果是高障碍物阻挡
            world_transform.m_position += 0.4f * Vector3::UNIT_Z;
            if (physics_scene->sweep(
                m_rigidbody_shape,
                world_transform.getMatrix(),
                horizontal_direction,
                horizontal_displacement.length(),
                hits))
            {
                // 修正运动方向，沿着斜面走
                float cosTheta = 0.f - Math::cos(hits[0].hit_normal.angleBetween(horizontal_direction));
                final_position +=
                    (horizontal_displacement + cosTheta * horizontal_displacement.length() * hits[0].hit_normal);
            }
            // 如果是低障碍物阻挡
            else
            {
                // 水平方向正常移动
                final_position += horizontal_displacement;

                // 竖直方向进行位置修正，即将角色高度与台阶高度同步
                if (physics_scene->raycast(
                        final_position + 0.1f * Vector3::UNIT_Z, Vector3::NEGATIVE_UNIT_Z, 0.995f, hits))
                {
                    final_position = hits[0].hit_position;
                }
            }
            hits.clear();
        }
        else
        {
            final_position += horizontal_displacement; 
        }

        hits.clear();

        return final_position;
    }

} // namespace Pilot

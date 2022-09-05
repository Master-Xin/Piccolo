#include "runtime/function/framework/component/motor/motor_component.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/character/character.h"
#include "runtime/function/controller/character_controller.h"
#include "runtime/function/framework/component/animation/animation_component.h"
#include "runtime/function/framework/component/camera/camera_component.h"
#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/level/level.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/input/input_system.h"
#include "runtime/function/physics/physics_scene.h"

namespace Pilot
{
    void MotorComponent::postLoadResource(std::weak_ptr<GObject> parent_object)
    {
        // ？这个父对象都会跳转到 phisics actor，应该是有错误
        m_parent_object = parent_object;


        if (m_motor_res.m_controller_config.getTypeName() == "PhysicsControllerConfig")
        {
            m_controller_type = ControllerType::physics;
            PhysicsControllerConfig* controller_config =
                static_cast<PhysicsControllerConfig*>(m_motor_res.m_controller_config);
            m_controller = new CharacterController(controller_config->m_capsule_shape);
        }
        else if (m_motor_res.m_controller_config != nullptr)
        {
            m_controller_type = ControllerType::invalid;
            LOG_ERROR("invalid controller type, not able to move");
        }

        const TransformComponent* transform_component = parent_object.lock()->tryGetComponentConst(TransformComponent);

        m_target_position = transform_component->getPosition();
    }

    MotorComponent::~MotorComponent()
    {
        if (m_controller_type == ControllerType::physics)
        {
            delete m_controller;
            m_controller = nullptr;
        }
    }

    void MotorComponent::tick(float delta_time)
    {
        tickPlayerMotor(delta_time);
    }

    void MotorComponent::tickPlayerMotor(float delta_time)
    {
        // 获取父对象的锁，成功就可以继续执行。这保证多线程安全
        if (!m_parent_object.lock())
            return;

        // 获取存储着 GO 对象的 level 对象和其中的活跃的 character
        std::shared_ptr<Level>     current_level     = g_runtime_global_context.m_world_manager->getCurrentActiveLevel().lock();
        std::shared_ptr<Character> current_character = current_level->getCurrentActiveCharacter().lock();
        if (current_character == nullptr)
            return;

        // 获取当前活跃角色的 ID
        if (current_character->getObjectID() != m_parent_object.lock()->getID())
            return;

        // 获取 transform 的运算组件数据
        TransformComponent* transform_component =
            m_parent_object.lock()->tryGetComponent<TransformComponent>("TransformComponent");

        //============================================================
        CameraComponent* camera_component =
            m_parent_object.lock()->tryGetComponent<CameraComponent>("CameraComponent");
        //============================================================

        // 根据光标计算视角的偏转角
        Radian turn_angle_yaw = g_runtime_global_context.m_input_system->m_cursor_delta_yaw;

        // 获得输入的指令数据
        unsigned int command = g_runtime_global_context.m_input_system->getGameCommand();

        if (command >= (unsigned int)GameCommand::invalid)
            return;

        // 根据 输入指令、delta_time，计算期望的水平移动速度
        calculatedDesiredHorizontalMoveSpeed(command, delta_time);

        // 根据 输出指令、delta_time，计算期望的垂直移动速度
        calculatedDesiredVerticalMoveSpeed(command, delta_time);

        //// 根据 输出指令、当前角色的世界坐标下的朝向，计算移动方向
        //calculatedDesiredMoveDirection(command, transform_component->getRotation());

        //==================== 新改动的 ============================
        calculatedDesiredMoveDirectionNew(command, camera_component->getCameraForwardDirection());

        // 计算逻辑期望位移
        calculateDesiredDisplacement(delta_time);

        // 计算最终移动位置
        calculateTargetPosition(transform_component->getPosition());
        
        // 
        transform_component->setPosition(m_target_position);

        AnimationComponent* animation_component =
            m_parent_object.lock()->tryGetComponent<AnimationComponent>("AnimationComponent");
        if (animation_component != nullptr)
        {
            animation_component->updateSignal("speed", m_target_position.distance(transform_component->getPosition()) / delta_time);
            animation_component->updateSignal("jumping", m_jump_state != JumpState::idle);
        }
    }

    void MotorComponent::calculatedDesiredHorizontalMoveSpeed(unsigned int command, float delta_time)
    {
        // 用来判断是否水平移动
        bool has_move_command = ((unsigned int)GameCommand::forward | (unsigned int)GameCommand::backward |
                                 (unsigned int)GameCommand::left | (unsigned int)GameCommand::right) &
                                command;
        // 用来判断是否冲刺
        bool has_sprint_command = (unsigned int)GameCommand::sprint & command;

        // 是否加速标志
        bool  is_acceleration    = false;

        // 加速度初始化为的电机驱动的移动状态下的加速度
        float final_acceleration = m_motor_res.m_move_acceleration;

        // 初始化最大和最小的速度比率
        float min_speed_ratio    = 0.f;
        float max_speed_ratio    = 0.f;

        // 如果移动了
        if (has_move_command)
        {
            // 打开加速开关
            is_acceleration = true;
            // 读取 能够驱动到的 最大移动速度比率
            max_speed_ratio = m_motor_res.m_max_move_speed_ratio;

            // 如果 当前移动速度比率 大于等于 驱动的最大移动速度比率（意味着可能要加速到最大冲刺速度，否则就是减速）
            if (m_move_speed_ratio >= m_motor_res.m_max_move_speed_ratio)
            {
                // 实际最终的加速度 设为 驱动的冲刺加速度
                final_acceleration = m_motor_res.m_sprint_acceleration;
                // 读取实际是否在冲刺
                is_acceleration    = has_sprint_command;
                // 移动速度已经超出能够驱动到的最大速度，将此次速度的变化范围的最小值设为电机能够驱动到的最大移动速度
                min_speed_ratio    = m_motor_res.m_max_move_speed_ratio;
                // 此次速度的变化范围的最大值设为电机能够驱动到的最大冲刺速度
                max_speed_ratio    = m_motor_res.m_max_sprint_speed_ratio;
            }
        }
        // 如果没有移动
        else
        {
            // 不会加速
            is_acceleration = false;
            // 将要减速到 0
            min_speed_ratio = 0.f;
            // 最大速度比率是 电机能够驱动到的最大冲刺速度
            max_speed_ratio = m_motor_res.m_max_sprint_speed_ratio;
        }

        // 计算当前的移动速度比率，不是加速就是减速
        m_move_speed_ratio += (is_acceleration ? 1.0f : -1.0f) * final_acceleration * delta_time;
        // 钳制住速度比率，也就是限幅
        m_move_speed_ratio = std::clamp(m_move_speed_ratio, min_speed_ratio, max_speed_ratio);
    }

    void MotorComponent::calculatedDesiredVerticalMoveSpeed(unsigned int command, float delta_time)
    {
        // 读取物理场景的一些参数
        std::shared_ptr<PhysicsScene> physics_scene =
            g_runtime_global_context.m_world_manager->getCurrentActivePhysicsScene().lock();
        ASSERT(physics_scene);

        // 如果 驱动器 支持的跳跃高度是 0，直接返回，不对垂直方向速度进行改变
        if (m_motor_res.m_jump_height == 0.f)
            return;

        // 读取的是重力向量的长度，也就是重力的大小
        const float gravity = physics_scene->getGravity().length();

        // JumpState 中有三种状态：静止、下降、上升

        // 如果当前跳跃状态不是静止，或者没有触碰到地面
        if (m_jump_state == JumpState::idle && m_controller->isTouchGround() == false)
        {
            // 将跳跃状态改为下降
            m_jump_state = JumpState::falling;
        }

        // 如果跳跃状态是静止
        if (m_jump_state == JumpState::idle)
        {
            // 如果指令中跳跃被按下
            if ((unsigned int)GameCommand::jump & command)
            {
                // 跳跃状态切换为上升
                m_jump_state                    = JumpState::rising;
                // 垂直方向的初速度，v = 根号下(2gh)
                m_vertical_move_speed           = Math::sqrt(m_motor_res.m_jump_height * 2 * gravity);
                // 跳跃时水平方向的移动速度
                m_jump_horizontal_speed_ratio   = m_move_speed_ratio;
            }
            else
            {
                // 跳跃键没有被按下，垂直方向速度就是 0 
                m_vertical_move_speed = 0.f;
            }
        }
        // 如果跳跃状态是在上升或者下降
        else if (m_jump_state == JumpState::rising || m_jump_state == JumpState::falling)
        {
            // 垂直移动速度更新，如果是正，代表速度朝上；如果是负，则代表速度朝下
            m_vertical_move_speed -= gravity * delta_time;
            if (m_vertical_move_speed <= 0.f)
            {
                m_jump_state = JumpState::falling;
            }
        }
    }

    void MotorComponent::calculatedDesiredMoveDirection(unsigned int command, const Quaternion& object_rotation)
    {

        // 如果不在跳跃状态
        if (m_jump_state == JumpState::idle)
        {
            // 前面的方向向量（模型坐标系下的 -y 方向）
            Vector3 forward_dir = object_rotation * Vector3::NEGATIVE_UNIT_Y;
            // 朝左的方向向量（模型坐标系下的 x 方向）
            Vector3 left_dir    = object_rotation * Vector3::UNIT_X;


            // 有指令被传输
            if (command > 0)
            {
                m_desired_horizontal_move_direction = Vector3::ZERO;
            }

            // 朝前
            if ((unsigned int)GameCommand::forward & command)
            {
                m_desired_horizontal_move_direction += forward_dir;
            }

            // 朝后
            if ((unsigned int)GameCommand::backward & command)
            {
                m_desired_horizontal_move_direction -= forward_dir;
            }
            // 朝左
            if ((unsigned int)GameCommand::left & command)
            {
                m_desired_horizontal_move_direction += left_dir;
            }
            // 朝右
            if ((unsigned int)GameCommand::right & command)
            {
                m_desired_horizontal_move_direction -= left_dir;
            }
            // 方向向量归一化
            m_desired_horizontal_move_direction.normalise();
        }
    }

    void MotorComponent::calculatedDesiredMoveDirectionNew(unsigned int command, const Vector3& camera_direction)
    {

        // 如果不在跳跃状态
        if (m_jump_state == JumpState::idle)
        {
            // 前面的方向向量
            Vector3 forward_dir = camera_direction;
            forward_dir.z       = 0.f;            // 忽略 z 轴的方向信息
            forward_dir.normalise();              // 忽略 z 轴信息后，需要归一化

            // 朝左的方向向量（模型坐标系下的 x 方向）
            Vector3 left_dir {0.f - forward_dir.y, forward_dir.x, forward_dir.z};

            if (command > 0)
            {
                m_desired_horizontal_move_direction = Vector3::ZERO;
            }
            if ((unsigned int)GameCommand::forward & command)
            {
                m_desired_horizontal_move_direction += forward_dir;
            }
            if ((unsigned int)GameCommand::backward & command)
            {
                m_desired_horizontal_move_direction -= forward_dir;
            }
            if ((unsigned int)GameCommand::left & command)
            {
                m_desired_horizontal_move_direction += left_dir;
            }
            if ((unsigned int)GameCommand::right & command)
            {
                m_desired_horizontal_move_direction -= left_dir;
            }
            m_desired_horizontal_move_direction.normalise();
        }
    }

    void MotorComponent::calculateDesiredDisplacement(float delta_time)
    {
        float horizontal_speed_ratio =
            m_jump_state == JumpState::idle ? m_move_speed_ratio : m_jump_horizontal_speed_ratio;
        m_desired_displacement =
            m_desired_horizontal_move_direction * m_motor_res.m_move_speed * horizontal_speed_ratio * delta_time +
            Vector3::UNIT_Z * m_vertical_move_speed * delta_time;
    }

    void MotorComponent::calculateTargetPosition(const Vector3&& current_position)
    {
        Vector3 final_position;

        switch (m_controller_type)
        {
            case ControllerType::none:
                final_position = current_position + m_desired_displacement;
                break;
            case ControllerType::physics:
                final_position = m_controller->move(current_position, m_desired_displacement);
                break;
            default:
                final_position = current_position;
                break;
        }

        if (m_jump_state == JumpState::falling && m_controller->isTouchGround())
        {
            m_jump_state = JumpState::idle;
        }

        m_is_moving       = (final_position - current_position).squaredLength() > 0.f;
        m_target_position = final_position;
    }

} // namespace Pilot

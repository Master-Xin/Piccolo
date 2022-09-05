#include "runtime/engine.h"

#include "runtime/core/base/macro.h"
#include "runtime/core/meta/reflection/reflection_register.h"

#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/input/input_system.h"
#include "runtime/function/physics/physics_manager.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/window_system.h"

namespace Pilot
{
    bool                            g_is_editor_mode {false};
    std::unordered_set<std::string> g_editor_tick_component_types {};

    void PilotEngine::startEngine(const EngineInitParams& param)
    {
        m_init_params = param;

        // 构建出所有需要元数据的类型的元数据
        Reflection::TypeMetaRegister::Register();

        // 进行引擎中物理、渲染、窗口、配置等系统的初始化
        g_runtime_global_context.startSystems(param);

        LOG_INFO("engine start");
    }

    void PilotEngine::shutdownEngine()
    {
        LOG_INFO("engine shutdown");

        g_runtime_global_context.shutdownSystems();

        Reflection::TypeMetaRegister::Unregister();
    }

    void PilotEngine::initialize() {}
    void PilotEngine::clear() {}

    void PilotEngine::run()
    {
        std::shared_ptr<WindowSystem> window_system = g_runtime_global_context.m_window_system;
        ASSERT(window_system);

        while (!window_system->shouldClose())
        {
            const float delta_time = calculateDeltaTime();
            tickOneFrame(delta_time);
        }
    }

    float PilotEngine::calculateDeltaTime()
    {
        float delta_time;
        {
            using namespace std::chrono;

            // 获得当前 tick 计算开始的时间点
            steady_clock::time_point tick_time_point = steady_clock::now();

            // 减去上一次 tick 计算开始的时间点，获得时长
            duration<float> time_span = duration_cast<duration<float>>(tick_time_point - m_last_tick_time_point);
            delta_time                = time_span.count();

            // 将这次 tick 开始的时间点记录下来
            m_last_tick_time_point = tick_time_point;
        }

        // 返回上一次 tick 的运算时长
        return delta_time;
    }

    bool PilotEngine::tickOneFrame(float delta_time)
    {
        // 动画、物理计算，读取输入数据
        logicalTick(delta_time);

        calculateFPS(delta_time);

        // single thread
        // exchange data between logic and render contexts
        g_runtime_global_context.m_render_system->swapLogicRenderData();

        rendererTick();

#ifdef ENABLE_PHYSICS_DEBUG_RENDERER
        g_runtime_global_context.m_physics_manager->renderPhysicsWorld(delta_time);
#endif

        g_runtime_global_context.m_window_system->pollEvents();


        g_runtime_global_context.m_window_system->setTile(
            std::string("Pilot - " + std::to_string(getFPS()) + " FPS").c_str());

        const bool should_window_close = g_runtime_global_context.m_window_system->shouldClose();
        return !should_window_close;
    }

    void PilotEngine::logicalTick(float delta_time)
    {
        // 先是动画、物理的计算，对一些模块内的状态数据进行更新
        g_runtime_global_context.m_world_manager->tick(delta_time);

        // 后是对输入数据进行读取保存，放在输入系统中的 m_command 当中
        g_runtime_global_context.m_input_system->tick();
    }

    bool PilotEngine::rendererTick()
    {
        g_runtime_global_context.m_render_system->tick();
        return true;
    }

    const float PilotEngine::k_fps_alpha = 1.f / 100;
    void        PilotEngine::calculateFPS(float delta_time)
    {
        m_frame_count++;

        if (m_frame_count == 1)
        {
            m_average_duration = delta_time;
        }
        else
        {
            m_average_duration = m_average_duration * (1 - k_fps_alpha) + delta_time * k_fps_alpha;
        }

        m_fps = static_cast<int>(1.f / m_average_duration);
    }
} // namespace Pilot

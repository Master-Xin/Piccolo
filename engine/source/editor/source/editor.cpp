#include "editor//include/editor.h"

#include "runtime/engine.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_system.h"

#include "editor/include/editor_global_context.h"
#include "editor/include/editor_input_manager.h"
#include "editor/include/editor_scene_manager.h"
#include "editor/include/editor_ui.h"

namespace Pilot
{
    void registerEdtorTickComponent(std::string component_type_name)
    {
        g_editor_tick_component_types.insert(component_type_name);
    }

    PilotEditor::PilotEditor()
    {
        registerEdtorTickComponent("TransformComponent");
        registerEdtorTickComponent("MeshComponent");
    }

    PilotEditor::~PilotEditor() {}

    void PilotEditor::initialize(PilotEngine* engine_runtime)
    {
        assert(engine_runtime);

        g_is_editor_mode = true;
        m_engine_runtime = engine_runtime;

        EditorGlobalContextInitInfo init_info = {g_runtime_global_context.m_window_system.get(),
                                                 g_runtime_global_context.m_render_system.get(),
                                                 engine_runtime};
        g_editor_global_context.initialize(init_info);
        g_editor_global_context.m_scene_manager->setEditorCamera(
            g_runtime_global_context.m_render_system->getRenderCamera());
        g_editor_global_context.m_scene_manager->uploadAxisResource();

        m_editor_ui                   = std::make_shared<EditorUI>();
        WindowUIInitInfo ui_init_info = {g_runtime_global_context.m_window_system,
                                         g_runtime_global_context.m_render_system};
        m_editor_ui->initialize(ui_init_info);
    }

    void PilotEditor::clear() { g_editor_global_context.clear(); }

    void PilotEditor::run()
    {
        assert(m_engine_runtime);   // 断定当前的 engine 对象 已经创建
        assert(m_editor_ui);        // 断定当前的 编辑器窗口对象 已经创建
        float delta_time;

        // 主循环来了，只有窗口被关闭时，循环才终止
        while (true)
        {
            // 计算出上一次的 tick 的运行间隔时长
            delta_time = m_engine_runtime->calculateDeltaTime();

            // 编辑器模式下的场景管理器循环
            g_editor_global_context.m_scene_manager->tick(delta_time);

            // 编辑器模式下的输入管理器循环
            g_editor_global_context.m_input_manager->tick(delta_time);

            // 运行时引擎的功能，也就是游戏模式，里面就是 逻辑 + 动画 + 物理 + 渲染
            if (!m_engine_runtime->tickOneFrame(delta_time))
                return;
        }
    }
} // namespace Pilot

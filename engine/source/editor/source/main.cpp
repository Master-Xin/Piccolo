#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>

#include "runtime/engine.h"

#include "editor/include/editor.h"

// https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html
#define PILOT_XSTR(s) PILOT_STR(s)
#define PILOT_STR(s) #s

int main(int argc, char** argv)
{
    // 读取引擎文件的根目录，其中 PILOT_ROOT_DIR 宏在预处理器当中，是构建 VS 工程的时候生成的
    std::filesystem::path pilot_root_folder = std::filesystem::path(PILOT_XSTR(PILOT_ROOT_DIR));

    // 设置引擎初始化参数，包括引擎的根目录文件夹、配置文件夹
    // 其中 std::filesystem::path 的 / 符号是经过重载的，专门来进行路径的拼接
    Pilot::EngineInitParams params;
    params.m_root_folder      = pilot_root_folder;
    params.m_config_file_path = pilot_root_folder / "PilotEditor.ini";

    // 实例化一个引擎类，是所有引擎子功能模块的集合
    Pilot::PilotEngine* engine = new Pilot::PilotEngine();

    // 进行引擎各个子系统的初始化操作
    engine->startEngine(params);
    engine->initialize();

    // 将编辑器初始化，需要一些引擎子系统的上下文信息（context）
    Pilot::PilotEditor* editor = new Pilot::PilotEditor();
    editor->initialize(engine);

    // 编辑器开始运行
    editor->run();

    // 运行完毕后，清理编辑器占用的内存
    editor->clear();

    // 清理引擎对象占用的内存
    engine->clear();
    engine->shutdownEngine();   // 主要是这一句释放的内存

    return 0;
}

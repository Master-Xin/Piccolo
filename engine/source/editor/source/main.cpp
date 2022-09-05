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
    // ��ȡ�����ļ��ĸ�Ŀ¼������ PILOT_ROOT_DIR ����Ԥ���������У��ǹ��� VS ���̵�ʱ�����ɵ�
    std::filesystem::path pilot_root_folder = std::filesystem::path(PILOT_XSTR(PILOT_ROOT_DIR));

    // ���������ʼ����������������ĸ�Ŀ¼�ļ��С������ļ���
    // ���� std::filesystem::path �� / �����Ǿ������صģ�ר��������·����ƴ��
    Pilot::EngineInitParams params;
    params.m_root_folder      = pilot_root_folder;
    params.m_config_file_path = pilot_root_folder / "PilotEditor.ini";

    // ʵ����һ�������࣬�����������ӹ���ģ��ļ���
    Pilot::PilotEngine* engine = new Pilot::PilotEngine();

    // �������������ϵͳ�ĳ�ʼ������
    engine->startEngine(params);
    engine->initialize();

    // ���༭����ʼ������ҪһЩ������ϵͳ����������Ϣ��context��
    Pilot::PilotEditor* editor = new Pilot::PilotEditor();
    editor->initialize(engine);

    // �༭����ʼ����
    editor->run();

    // ������Ϻ�����༭��ռ�õ��ڴ�
    editor->clear();

    // �����������ռ�õ��ڴ�
    engine->clear();
    engine->shutdownEngine();   // ��Ҫ����һ���ͷŵ��ڴ�

    return 0;
}

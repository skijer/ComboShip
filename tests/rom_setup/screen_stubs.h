#pragma once
// Replace only the OS/window/picker boundary; screen_behavior_test.py compiles the
// complete production screen and observes its prompts and extraction requests.
#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include "combo/ComboExtract.h"
#include "combo/ComboGameMode.h"
#include "combo/ComboExport.h"

struct ImVec2 { float x, y; ImVec2(float a=0,float b=0):x(a),y(b){} };
struct ImVec4 { float x,y,z,w; ImVec4(float a=0,float b=0,float c=0,float d=0):x(a),y(b),z(c),w(d){} };
enum { ImGuiCond_Always, ImGuiStyleVar_WindowPadding, ImGuiStyleVar_WindowRounding,
       ImGuiStyleVar_FrameRounding, ImGuiCol_WindowBg, ImGuiCol_Text,
       ImGuiCol_TextDisabled, ImGuiCol_FrameBg, ImGuiCol_PlotHistogram,
       ImGuiWindowFlags_NoTitleBar=1, ImGuiWindowFlags_NoResize=2,
       ImGuiWindowFlags_NoMove=4, ImGuiWindowFlags_NoCollapse=8,
       ImGuiWindowFlags_NoSavedSettings=16, ImGuiWindowFlags_AlwaysAutoResize=32 };
#define IM_COL32(...) 0
struct ImGuiViewport { ImVec2 Pos,Size{1280,960},WorkSize{1280,960}; ImVec2 GetCenter(){return {640,480};} };
namespace Test {
inline int frame=0;
inline int selectGame=-2;
inline bool selectQuit=false;
inline std::vector<bool> disabled;
inline bool picker=true;
inline bool skipOther=false, optionalStep=false, failExtraction=false;
inline bool changeOptional=true;
inline std::vector<std::string> pickPaths;
inline std::vector<std::string> prompts, jobs;
inline std::vector<int> promptFrames;
inline int visits=0;
inline int kind(const char* path) {
    const std::string p=path?path:"";
    if(p.find("vanilla")!=std::string::npos) return 1;
    if(p.find("mq")!=std::string::npos) return 2;
    return 0;
}
inline void Record(const char* format,va_list args) {
    char text[4096];vsnprintf(text,sizeof(text),format,args);
    prompts.emplace_back(text);promptFrames.push_back(frame);
    const std::string label=text;
    if(label=="Ocarina of Time" || label=="Majora's Mask") optionalStep=false;
    if(label=="Ocarina of Time: Master Quest" || label=="Ocarina of Time: Vanilla") optionalStep=true;
}
}
namespace ImGui {
inline void SetCurrentContext(void*){}
inline ImGuiViewport* GetMainViewport(){static ImGuiViewport v;return &v;}
struct DrawList { void AddRectFilled(ImVec2,ImVec2,int){} };
inline DrawList* GetBackgroundDrawList(){static DrawList d;return &d;}
inline void SetNextWindowPos(ImVec2,int,ImVec2){}
inline void SetNextWindowSizeConstraints(ImVec2,ImVec2){}
template<class T> void PushStyleVar(int,T){}
inline void PushStyleColor(int,ImVec4){}
inline void PopStyleColor(int=1){}
inline void PopStyleVar(int=1){}
inline bool Begin(const char*,void*,int){return true;}
inline void End(){}
inline void SetWindowFontScale(float){}
inline void Separator(){}
inline void Spacing(){}
inline void SameLine(){}
inline void PushID(int){}
inline void PopID(){}
inline void Text(const char* fmt,...){va_list a;va_start(a,fmt);Test::Record(fmt,a);va_end(a);}
inline void TextWrapped(const char* fmt,...){va_list a;va_start(a,fmt);Test::Record(fmt,a);va_end(a);}
inline void TextDisabled(const char* fmt,...){va_list a;va_start(a,fmt);Test::Record(fmt,a);va_end(a);}
inline void TextColored(ImVec4,const char* fmt,...){va_list a;va_start(a,fmt);Test::Record(fmt,a);va_end(a);}
struct Style { ImVec4 Colors[32]; };
inline Style& GetStyle(){static Style s;return s;}
inline void BeginDisabled(bool value=true){Test::disabled.push_back(value||(!Test::disabled.empty()&&Test::disabled.back()));}
inline void EndDisabled(){assert(!Test::disabled.empty());Test::disabled.pop_back();}
inline bool Button(const char* label,ImVec2={}) {
    if(!Test::disabled.empty()&&Test::disabled.back()) return false;
    const std::string b=label;
    if(b=="Combo (OoT + MM)") return Test::selectGame==0;
    if(b=="Ocarina of Time") return Test::selectGame==1;
    if(b=="Majora's Mask") return Test::selectGame==2;
    if(b=="Quit" && Test::selectQuit) return true;
    if(b=="Change" && Test::optionalStep && !Test::changeOptional) return false;
    if((b=="Browse..." || b=="Change") && Test::visits<int(Test::pickPaths.size()))
        return !(Test::optionalStep&&Test::skipOther);
    if(b=="Add ROM") return !Test::skipOther;
    if(b=="Skip") return Test::skipOther;
    if(b=="Quit") return Test::failExtraction;
    if(b=="Continue" || b=="Extract" || b=="Next") return true;
    return false;
}
inline ImVec2 GetContentRegionAvail(){return {600,400};}
inline void ProgressBar(float,ImVec2,const char*){}
}
namespace ComboRando {
inline void ComboMenu_PushButton(ImVec4){}
inline void ComboMenu_PopButton(){}
inline ImVec4 ComboMenu_ThemeColor(){return {};}
}
#ifndef COMBO_TEST_NATIVE_PICKER
namespace ComboFilePicker {
inline bool Available(){return Test::picker;}
inline std::string PickFile(const char*,const char*,std::initializer_list<const char*>) {
    return Test::pickPaths.at(Test::visits++);
}
}
#endif
namespace Ship {
class Gui { public: void* GetImGuiContext(){return nullptr;} void StartDraw(){} void EndDraw(){} };
class Window {
public:
    virtual ~Window()=default;
    std::shared_ptr<Gui> GetGui(){return std::make_shared<Gui>();}
    bool IsRunning(){return Test::frame<40;}
};
class FileDropMgr {
public: void RegisterDropHandler(bool(*)(char*)){} void UnregisterDropHandler(bool(*)(char*)){} void ClearDroppedFile(){}
};
class Context {
public:
    static std::shared_ptr<Context> GetRawInstance();
    std::shared_ptr<Window> GetWindow();
    std::shared_ptr<FileDropMgr> GetFileDropMgr(){return std::make_shared<FileDropMgr>();}
};
}
namespace Fast {
class Fast3dWindow:public Ship::Window {
public:
    void HandleEvents(){++Test::frame;}
    bool IsFrameReady(){return true;}
    void StartFrame(){} void EndFrame(){} void RunGuiOnly(){}
};
}
inline std::shared_ptr<Ship::Context> Ship::Context::GetRawInstance(){static auto c=std::make_shared<Context>();return c;}
inline std::shared_ptr<Ship::Window> Ship::Context::GetWindow(){static auto w=std::make_shared<Fast::Fast3dWindow>();return w;}

#pragma once
#ifdef COMBO_BUILD
#ifdef __cplusplus
extern "C" {
#endif
void ItemGrantAudit_Begin(const char* source, int item, int check, int quiet);
void ItemGrantAudit_End(void);
void ItemGrantAudit_Checkpoint(const char* phase);
void ItemGrantAudit_Suspend(void);
void ItemGrantAudit_Resume(void);
#ifdef __cplusplus
}
#endif
#else
#define ItemGrantAudit_Begin(source, item, check, quiet) ((void)0)
#define ItemGrantAudit_End() ((void)0)
#define ItemGrantAudit_Checkpoint(phase) ((void)0)
#define ItemGrantAudit_Suspend() ((void)0)
#define ItemGrantAudit_Resume() ((void)0)
#endif
#ifdef __cplusplus
namespace ItemGrantAudit {
class Scope {
  public:
    Scope(const char* source, int item = -1, int check = -1, bool quiet = false) {
#ifdef COMBO_BUILD
        ItemGrantAudit_Begin(source, item, check, quiet);
#else
        (void)source;
        (void)item;
        (void)check;
        (void)quiet;
#endif
    }
    ~Scope() {
        ItemGrantAudit_End();
    }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
};
} // namespace ItemGrantAudit
#endif

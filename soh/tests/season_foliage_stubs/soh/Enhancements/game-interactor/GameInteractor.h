#pragma once
class GameInteractor {
  public:
    static GameInteractor* Instance;
    struct OnPlayDrawEnd {};
    struct OnPlayDestroy {};
    struct OnSceneInit {};
    struct OnExitGame {};
    template <typename Hook, typename Callback> void RegisterGameHook(Callback) {
    }
};

#pragma once
#include <cmath>

namespace BlackMarketBrowsing {
enum class Phase { Browse, Confirm, Result, KeeperTalk, KeeperHelpChoice, KeeperHelp };
struct Point {
    float x, y, z;
};
struct View {
    Point eye, at;
    float fov;
};

// Scene-authored customer eye and pitch, shared by the native Data4 camera
// and MM's linked actor cutscenes. A row/column change never changes the shot.
inline constexpr Point kEye{ 0, 88, 253 };
inline constexpr float kPitchDegrees = -13;
inline float BankAngle(int slot) {
    return slot < 0 ? 0 : slot < 4 ? 30 : -30;
}
inline View Turn(float angle, float fov) {
    constexpr float radians = 3.14159265359f / 180;
    return { kEye,
             { kEye.x + 100 * std::sin(angle * radians), kEye.y + 100 * std::tan(kPitchDegrees * radians),
               kEye.z - 100 * std::cos(angle * radians) },
             fov };
}
inline View Target(int slot) {
    return Turn(BankAngle(slot), slot < 0 ? 46 : 72);
}
inline float Approach(float current, float target) {
    return std::abs(target - current) < .05f ? target : current + (target - current) * .4f;
}
struct Axis {
    int direction = 0, delay = 0;
    int Step(int value) {
        const int next = value > 30 ? 1 : value < -30 ? -1 : 0;
        if (!next) {
            direction = delay = 0;
            return 0;
        }
        if (next != direction) {
            direction = next;
            delay = 10;
            return next;
        }
        if (--delay <= 0) {
            delay = 4;
            return next;
        }
        return 0;
    }
};
struct State {
    int slot = -1, row = 0;
    Phase phase = Phase::Browse;
    View view = Target(-1);
    float faceAngle = 0;
    Axis horizontal, vertical;
    void ResetInput() {
        horizontal = {};
        vertical = {};
    }
    bool Move(int x, int y) {
        const int previous = slot;
        if (slot < 0) {
            if (x)
                slot = (x > 0 ? 0 : 4) + row;
        } else if (x) {
            const bool right = slot < 4, outer = (slot % 4) >= 2;
            if ((x > 0) == right) {
                if (!outer)
                    slot += 2;
            } else if (outer)
                slot -= 2;
            else {
                row = slot & 1;
                slot = -1;
            }
        } else if (y) {
            slot = (slot & ~1) + (y > 0 ? 1 : 0);
        }
        return previous != slot;
    }
    void TickCamera() {
        faceAngle = Approach(faceAngle, BankAngle(slot));
        view = Turn(faceAngle, Approach(view.fov, slot < 0 ? 46 : 72));
    }
    bool CameraSettled() const {
        const View target = Target(slot);
        return view.eye.x == target.eye.x && view.eye.y == target.eye.y && view.eye.z == target.eye.z &&
               view.at.x == target.at.x && view.at.y == target.at.y && view.at.z == target.at.z &&
               view.fov == target.fov;
    }
};
} // namespace BlackMarketBrowsing

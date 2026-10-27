#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace BlackMarketGranny {
struct Decoration {
    int16_t id, x, y, z, rx, ry, rz, params;
};
static_assert(sizeof(Decoration) == 16);

// Read metadata only. Deserializing the whole foreign Scene would also load
// collision/mesh resources through game-specific factories and shared caches.
class NativeMetadata {
  public:
    std::vector<std::array<uint8_t, 22>> lights;
    std::vector<Decoration> decorations;
    bool ReadScene(const std::vector<char>& bytes) {
        return Parse(bytes, false);
    }
    bool ReadRoom(const std::vector<char>& bytes) {
        return Parse(bytes, true);
    }

  private:
    class Reader {
      public:
        const std::vector<char>& bytes;
        size_t at = 0;
        explicit Reader(const std::vector<char>& value) : bytes(value) {
        }
        void Need(size_t size) const {
            if (at > bytes.size() || size > bytes.size() - at)
                throw std::runtime_error("Truncated scene");
        }
        uint8_t Byte() {
            Need(1);
            return static_cast<uint8_t>(bytes[at++]);
        }
        uint32_t Word() {
            uint32_t n = 0;
            for (int i = 0; i < 4; ++i)
                n |= uint32_t(Byte()) << (8 * i);
            return n;
        }
        void Skip(size_t size) {
            Need(size);
            at += size;
        }
        size_t Count(size_t maximum = 256) {
            const auto n = Word();
            if (n > maximum)
                throw std::runtime_error("Scene count");
            return n;
        }
        void String() {
            Skip(Count(4096));
        }
    };
    bool Parse(const std::vector<char>& bytes, bool room) {
        if (room)
            decorations.clear();
        else
            lights.clear();
        try {
            Reader r(bytes);
            if (r.Byte() != 0)
                return false;
            r.Skip(3);
            if (r.Word() != 0x4f524f4d || r.Word() != 0)
                return false;
            r.Skip(52);
            const auto commands = r.Count(64);
            bool ended = false;
            std::vector<std::array<uint8_t, 22>> readLights;
            std::vector<Decoration> readDecorations;
            for (size_t i = 0; i < commands; ++i) {
                const auto command = r.Word();
                switch (command) {
                    case 0x15:
                        r.Skip(3);
                        break;
                    case 0x04: {
                        const auto count = r.Count();
                        for (size_t j = 0; j < count; ++j) {
                            r.String();
                            r.Skip(8);
                        }
                        break;
                    }
                    case 0x03:
                        r.String();
                        break;
                    case 0x06:
                        r.Skip(r.Count() * 2);
                        break;
                    case 0x07:
                        r.Skip(3);
                        break;
                    case 0x00:
                        r.Skip(r.Count() * 16);
                        break;
                    case 0x01: {
                        const auto count = r.Count(64);
                        for (size_t j = 0; j < count; ++j) {
                            Decoration actor{};
                            int16_t* fields[] = { &actor.id, &actor.x,  &actor.y,  &actor.z,
                                                  &actor.rx, &actor.ry, &actor.rz, &actor.params };
                            for (auto* field : fields) {
                                const uint16_t low = r.Byte(), high = r.Byte();
                                *field = int16_t(low | (high << 8));
                            }
                            if (actor.id == 8)
                                readDecorations.push_back(actor);
                            else if (actor.id != 0x149)
                                return false; // Replace only Granny with our existing keeper.
                        }
                        break;
                    }
                    case 0x0b:
                        r.Skip(r.Count() * 2);
                        break;
                    case 0x0e:
                        r.Skip(r.Count() * 16);
                        break;
                    case 0x0f: {
                        const auto count = r.Count(32);
                        readLights.resize(count);
                        for (auto& light : readLights)
                            for (auto& value : light)
                                value = r.Byte();
                        break;
                    }
                    case 0x0a:
                        r.Skip(4);
                        r.String();
                        r.String();
                        break; // Audited single normal Granny room.
                    case 0x08:
                    case 0x19:
                        r.Skip(5);
                        break;
                    case 0x10:
                        r.Skip(3);
                        break;
                    case 0x11:
                        r.Skip(4);
                        break;
                    case 0x12:
                        r.Skip(2);
                        break;
                    case 0x13:
                        r.Skip(r.Count() * 2);
                        break;
                    case 0x16:
                        r.Skip(1);
                        break;
                    case 0x14:
                        ended = true;
                        if (i + 1 != commands)
                            return false;
                        break;
                    default:
                        return false;
                }
            }
            if (!ended || r.at != bytes.size())
                return false;
            if (room) {
                if (readDecorations.size() != 2)
                    return false;
                decorations = std::move(readDecorations);
            } else {
                if (readLights.empty())
                    return false;
                lights = std::move(readLights);
            }
            return true;
        } catch (const std::exception&) { return false; }
    }
};
} // namespace BlackMarketGranny

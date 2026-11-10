#pragma once
#include <array>
#include <cstdint>
#include <string>
namespace mtg {
enum class GameFormat : unsigned { Commander, Classic, TwoHeadedGiant, Brawl, Limited, Count };
const char* formatName(GameFormat format);
std::int64_t startingLife(GameFormat format);
bool usesCommanderDamage(GameFormat format);
enum class Counter : unsigned { Poison, Energy, Experience, Rad, Spells, Custom, Treasure, Shield, Count };
constexpr unsigned counterCount = static_cast<unsigned>(Counter::Count);
const char* counterName(Counter counter);
struct Player {
 std::uint32_t id = 1;
 std::string name = "PLAYER ONE";
 std::int64_t life = 40;
 std::array<std::int64_t, counterCount> counters{};
 std::array<bool, counterCount> enabled{{true, false, false, false, false, false, true, true}};
 std::string customName = "CUSTOM";
 bool monarch = false, initiative = false, cityBlessing = false;
};
struct CommanderSource {
 std::uint32_t id;
 unsigned owner;
 std::string name;
 std::int64_t damage = 0;
 std::int64_t lifeLinkedDamage = 0;
 unsigned recipient = 1;
};
struct OwnCommander { std::string name; std::int64_t casts = 0; };
enum class DayNight { Neither, Day, Night };
enum class Status { Monarch, Initiative, CityBlessing };
}

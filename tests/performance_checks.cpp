#include "EngineStubs.h"
#include "AnnotationCache.h"
#include "PoiseHealthHandler.h"
#include "RegenStubs.h"
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>
#include <stdexcept>

using namespace MaxsuPoise;

static void Check(bool a_result, const char* a_message)
{
	if (!a_result) {
		throw std::runtime_error(a_message);
	}
}

static void CheckSettings()
{
	auto* collection = RE::GameSettingCollection::GetSingleton();
	GameSettings::CachedSetting<float> setting{ "testFloat", 3.f };
	Check(setting.Get() == 3.f, "Missing setting must use fallback");
	RE::Setting live{ 8.f };
	collection->settings["testFloat"] = &live;
	Check(setting.Get() == 8.f, "Late settings must resolve");
	const auto lookups = collection->lookups;
	live.value = 11.f;
	Check(setting.Get() == 11.f, "Cached pointer must read current value");
	Check(collection->lookups == lookups, "Warm settings must avoid lookup");
	RE::Setting replacement{ 14.f };
	collection->settings["testFloat"] = &replacement;
	setting.Resolve();
	Check(setting.Get() == 14.f, "Refresh must update binding");
	GameSettings::CachedSetting<bool> boolean{ "testFloat", false };
	GameSettings::CachedSetting<std::uint32_t> integer{ "testFloat", 0 };
	Check(boolean.Get() && integer.Get() == 14, "Typed setting reads");
	collection->settings.erase("testFloat");
}

static float LegacyTotal(RE::Actor& a_actor)
{
	float result = 0.f;
	for (const auto& [slot, mult] : SettingsHandler::configuration->armorSlotMult) {
		auto* armor = a_actor.GetWornArmor(slot);
		const float health = 50.f * mult;
		if (armor && (armor->IsLightArmor() || armor->IsHeavyArmor())) {
			result += armor->IsLightArmor() ? health : health * 1.5f;
		} else {
			armor = a_actor.GetSkin(slot);
			if (armor && (armor->IsLightArmor() || armor->IsHeavyArmor())) {
				result += armor->IsLightArmor() ? health : health * 1.5f;
			}
		}
	}
	return 40.f * a_actor.mass * a_actor.scale + result;
}

static void CheckArmor()
{
	SettingsHandler::configuration->armorSlotMult = { { 1, 0.5f }, { 2, 0.25f }, { 4, 0.25f } };
	std::mt19937 random{ 1170 };
	std::size_t oldSnapshots = 0, newSnapshots = 0;
	for (int trial = 0; trial < 3000; ++trial) {
		RE::Actor actor;
		RE::TESObjectARMO skin;
		skin.armorClass = static_cast<int>(random() % 3);
		actor.skin = trial % 4 ? &skin : nullptr;
		actor.mass = static_cast<float>(random() % 100) / 10.f;
		actor.scale = static_cast<float>(random() % 100) / 10.f;
		std::array<RE::TESObjectARMO, 32> armor;
		for (auto& item : armor) {
			item.slots = random() % 8;
			item.armorClass = static_cast<int>(random() % 3);
			auto entry = std::make_shared<RE::InventoryEntryData>();
			entry->worn = random() % 3 == 0;
			actor.inventory[&item] = { static_cast<int>(random() % 4) - 1, entry };
		}
		const auto expected = LegacyTotal(actor);
		oldSnapshots += actor.snapshots;
		actor.snapshots = 0;
		const auto actual = PoiseHealthHandler::GetTotalPoiseHealth(&actor);
		Check(actual == expected, "Armor selection changed");
		Check(actor.snapshots == 1, "Armor must use one inventory snapshot");
		newSnapshots += actor.snapshots;
	}
	SettingsHandler::configuration->armorSlotMult.clear();
	RE::Actor actor;
	Check(PoiseHealthHandler::GetTotalPoiseHealth(&actor) == 40.f, "Empty slots base health");
	Check(actor.snapshots == 0, "Empty slots must skip inventory");
	Check(PoiseHealthHandler::GetTotalPoiseHealth(nullptr) == 0.f, "Null actor health");
	std::cout << "armor_trials=3000 legacy_snapshots=" << oldSnapshots
		<< " optimized_snapshots=" << newSnapshots << '\n';
}

static std::optional<AnnoInfo> Parse(std::string_view a_json)
{
	try {
		return json::parse(a_json).get<AnnoInfo>();
	} catch (const json::exception&) {
		return std::nullopt;
	}
}

static void CheckRegen()
{
	std::mt19937 random{ 597 };
	std::size_t skipped = 0;
	for (int trial = 0; trial < 5000; ++trial) {
		RE::Actor actor;
		actor.loaded = random() % 8 != 0;
		actor.initialized = random() % 8 != 0;
		actor.process.high = random() % 8 != 0;
		actor.runtime.currentProcess = random() % 8 ? &actor.process : nullptr;
		actor.combat = random() % 2;
		actor.staggering = random() % 2;
		actor.poise = static_cast<float>(random() % 1000) / 10.f;
		actor.delay = static_cast<float>(random() % 40) / 10.f - 1.f;
		actor.protect = static_cast<float>(random() % 40) / 10.f - 1.f;
		const float delta = static_cast<float>(random() % 100) / 100.f;
		float poise = actor.poise, delay = actor.delay, protect = actor.protect;
		const auto previous = poise;
		const bool active = actor.runtime.currentProcess && actor.process.high && actor.loaded && actor.initialized;
		if (active) {
			if (!actor.combat || poise - 40.f >= 0.01f) { poise = 40.f; }
			else if (!actor.staggering) {
				if (delay > 0.f) { delay -= delta; }
				else { poise += 40.f * delta * 0.05f; }
				if (protect > 0.f) { protect -= delta; }
			}
		}
		PoiseRegenHandler::UpdatePoiseValue(&actor, delta);
		Check(actor.poise == poise && actor.delay == delay && actor.protect == protect, "Regen timing changed");
		Check(actor.poiseWrites == static_cast<std::size_t>(poise != previous), "Regen writes only changed health");
		if (active && poise == previous) { ++skipped; }
	}
	PoiseRegenHandler::UpdatePoiseValue(nullptr, 0.1f);
	std::cout << "regen_trials=5000 unchanged_health_writes_skipped=" << skipped << '\n';
}

static void CheckAnnotations()
{
	AnnotationCache cache;
	std::size_t parses = 0;
	auto parser = [&](std::string_view text) { ++parses; return Parse(text); };
	const std::string text = R"({"duration":0.5,"immuneLevel":3,"damageMult":1.25})";
	auto first = cache.Get(text, parser);
	Check(first && first->immuneLevel == kLarge && first->damageMult == 1.25f, "Parsed annotation");
	first->startTime = 1.f;
	Check(first->IsInFrames(1.f) && first->IsInFrames(1.5f) && !first->IsInFrames(1.501f), "Inclusive time window");
	auto second = cache.Get(text, parser);
	Check(parses == 1 && second->startTime == -1.f, "Cache must not retain clip time");
	second->startTime = 3.f;
	Check(second->IsInFrames(3.2f) && !second->IsInFrames(1.2f), "Shared text separate times");
	Check(!cache.Get("invalid", parser), "Malformed annotation ignored");
	const auto afterInvalid = parses;
	Check(!cache.Get("invalid", parser) && parses == afterInvalid, "Malformed annotation cached");
	auto clamp = cache.Get(R"({"duration":-1,"immuneLevel":99,"damageMult":-2})", parser);
	Check(clamp && clamp->duration == 0.f && clamp->damageMult == 0.f && clamp->immuneLevel == kLargest, "Annotation clamps");
	for (std::size_t i = 0; i < AnnotationCache::maxEntries + 20; ++i) {
		cache.Get(std::to_string(i), parser);
		Check(cache.Size() <= AnnotationCache::maxEntries, "Entry bound");
	}
	const std::string large(AnnotationCache::maxTextBytes / 2 + 1, ' ');
	cache.Get(large + "1", parser);
	cache.Get(large + "2", parser);
	Check(cache.TextBytes() <= AnnotationCache::maxTextBytes && cache.Size() == 1, "Text budget eviction");
	const auto bytes = cache.TextBytes();
	const auto beforeOversized = parses;
	const std::string oversized(AnnotationCache::maxTextBytes + 1, ' ');
	Check(!cache.Get(oversized, parser) && !cache.Get(oversized, parser), "Oversized text rejected");
	Check(cache.TextBytes() == bytes && parses == beforeOversized, "Oversized text must not parse or cache");

	AnnotationCache warm;
	warm.Get(text, parser);
	constexpr int iterations = 100000;
	std::uint64_t checksum = 0;
	const auto start = std::chrono::steady_clock::now();
	for (int i = 0; i < iterations; ++i) { checksum += Parse(text)->immuneLevel; }
	const auto middle = std::chrono::steady_clock::now();
	const auto beforeWarm = parses;
	for (int i = 0; i < iterations; ++i) { checksum += warm.Get(text, parser)->immuneLevel; }
	const auto end = std::chrono::steady_clock::now();
	Check(parses == beforeWarm && checksum == 600000, "Warm cache must avoid parsing");
	const auto legacy = std::chrono::duration<double, std::milli>(middle - start).count();
	const auto cached = std::chrono::duration<double, std::milli>(end - middle).count();
	std::cout << "annotation_iterations=" << iterations << " legacy_ms=" << legacy
		<< " warm_cache_ms=" << cached << " ratio=" << legacy / cached << '\n';
}

int main()
{
	try {
		CheckSettings();
		CheckArmor();
		CheckRegen();
		CheckAnnotations();
		std::cout << "PASS\n";
		return EXIT_SUCCESS;
	} catch (const std::exception& ex) {
		std::cerr << "FAIL: " << ex.what() << '\n';
		return EXIT_FAILURE;
	}
}

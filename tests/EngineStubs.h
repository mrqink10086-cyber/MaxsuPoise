#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#define WARN(...) ((void)0)

namespace RE
{
	struct Setting
	{
		float value = 0.f;
		float GetFloat() const { return value; }
		bool GetBool() const { return value != 0.f; }
		std::uint32_t GetUnsignedInteger() const { return static_cast<std::uint32_t>(value); }
	};

	struct GameSettingCollection
	{
		std::map<std::string, Setting*> settings;
		std::size_t lookups = 0;
		static GameSettingCollection* GetSingleton()
		{
			static GameSettingCollection instance;
			return &instance;
		}
		Setting* GetSetting(const char* a_name)
		{
			++lookups;
			const auto it = settings.find(a_name);
			return it == settings.end() ? nullptr : it->second;
		}
	};

	struct BSFixedString
	{
		std::string value;
		explicit BSFixedString(const char* a_value) : value(a_value) {}
	};

	using BipedObjectSlot = std::uint32_t;
	struct TESObjectARMO;
	struct TESBoundObject
	{
		bool IsArmor() const { return true; }
		template <class T> T* As() { return static_cast<T*>(this); }
	};
	struct TESObjectARMO : TESBoundObject
	{
		std::uint32_t slots = 0;
		int armorClass = 0;
		bool IsLightArmor() const { return armorClass == 1; }
		bool IsHeavyArmor() const { return armorClass == 2; }
		bool HasPartOf(BipedObjectSlot a_slot) const { return (slots & a_slot) != 0; }
	};
	struct InventoryEntryData
	{
		bool worn = false;
		bool IsWorn() const { return worn; }
	};
	struct Actor
	{
		struct Process { bool high = true; };
		struct Runtime { Process* currentProcess = nullptr; };
		Process process;
		Runtime runtime{ &process };
		bool loaded = true, initialized = true, combat = true, staggering = false;
		bool Is3DLoaded() const { return loaded; }
		bool IsInitialized() const { return initialized; }
		bool IsInCombat() const { return combat; }
		bool IsStaggering() const { return staggering; }
		Runtime& GetActorRuntimeData() { return runtime; }
		using Inventory = std::map<TESBoundObject*, std::pair<int, std::shared_ptr<InventoryEntryData>>>;
		Inventory inventory;
		TESObjectARMO* skin = nullptr;
		std::size_t snapshots = 0;
		float mass = 1.f;
		float scale = 1.f;
		float poise = 0.f;
		float delay = 0.f, protect = 0.f;
		std::size_t poiseWrites = 0;
		float GetScale() const { return scale; }
		template <class Filter> Inventory GetInventory(Filter a_filter)
		{
			++snapshots;
			Inventory result;
			for (const auto& [item, data] : inventory) {
				if (a_filter(*item)) {
					result.emplace(item, data);
				}
			}
			return result;
		}
		TESObjectARMO* GetWornArmor(BipedObjectSlot a_slot)
		{
			const auto inv = GetInventory([](TESBoundObject& a_item) { return a_item.IsArmor(); });
			for (const auto& [item, data] : inv) {
				if (data.first > 0 && data.second->IsWorn()) {
					auto* armor = item->As<TESObjectARMO>();
					if (armor->HasPartOf(a_slot)) {
						return armor;
					}
				}
			}
			return nullptr;
		}
		TESObjectARMO* GetSkin() { return skin; }
		TESObjectARMO* GetSkin(BipedObjectSlot a_slot)
		{
			if (auto* worn = GetWornArmor(a_slot)) {
				return worn;
			}
			return skin;
		}
		bool GetGraphVariableFloat(const BSFixedString& a_name, float& a_out)
		{
			a_out = a_name.value == "MaxsuPoise_RegenDelayTimer" ? delay :
				a_name.value == "MaxsuPoise_StaggerProtectTimer" ? protect : poise;
			return true;
		}
		bool SetGraphVariableFloat(const BSFixedString& a_name, float a_value)
		{
			if (a_name.value == "MaxsuPoise_RegenDelayTimer") { delay = a_value; }
			else if (a_name.value == "MaxsuPoise_StaggerProtectTimer") { protect = a_value; }
			else { poise = a_value; ++poiseWrites; }
			return true;
		}
	};
	inline float GetSecondsSinceLastFrame() { return 1.f / 60.f; }
}

namespace MaxsuPoise
{
	struct SettingsHandler
	{
		struct Configuration { std::vector<std::pair<RE::BipedObjectSlot, float>> armorSlotMult; };
		static inline auto configuration = std::make_shared<Configuration>();
		static std::shared_ptr<const Configuration> GetConfiguration() { return configuration; }
	};
	inline float GetActorMass(RE::Actor* a_actor) { return a_actor ? a_actor->mass : 0.f; }
}

#include "GameSettings.h"

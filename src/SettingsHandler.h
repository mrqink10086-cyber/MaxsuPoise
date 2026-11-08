#pragma once
#include "DKUtil/Config.hpp"
#include "GameSettings.h"
#include <memory>

namespace MaxsuPoise
{
	using WEAPON_TYPE = RE::WEAPON_TYPE;
	using BipedSlot = RE::BipedObjectSlot;
	using EventResult = RE::BSEventNotifyControl;

	class SettingsHandler : public RE::BSTEventSink<SKSE::ModCallbackEvent>
	{
	public:
		struct Configuration
		{
			std::array<float, static_cast<std::size_t>(WEAPON_TYPE::kTotal)> weapTypeMult{};
			std::vector<std::pair<BipedSlot, float>> armorSlotMult;
		};

		static std::shared_ptr<const Configuration> GetConfiguration()
		{
			return configuration.load(std::memory_order_acquire);
		}

		static constexpr char modName[] = "MaxsuPoise";
		static bool Register();

		virtual EventResult ProcessEvent(const SKSE::ModCallbackEvent* a_event, RE::BSTEventSource<SKSE::ModCallbackEvent>* a_eventSource)
		{
			if (a_event && _strcmpi(a_event->eventName.c_str(), "dmenu_updateSettings") == 0 && _strcmpi(a_event->strArg.c_str(), modName) == 0) {
				if (const auto* tasks = SKSE::GetTaskInterface()) {
					tasks->AddTask([] {
						ReloadConfiguration();
						GameSettings::Refresh();
					});
				}
			}

			return EventResult::kContinue;
		}

	private:
		SettingsHandler() = default;

		static void ReloadConfiguration();
		static void UpdateWeapTypeMult(Configuration& a_configuration);
		static void InitArmorSlotMult(Configuration& a_configuration);
		static inline std::atomic<std::shared_ptr<const Configuration>> configuration{ std::make_shared<const Configuration>() };
	};

}

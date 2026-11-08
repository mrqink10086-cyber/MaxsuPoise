#pragma once
#include "EngineStubs.h"

namespace MaxsuPoise
{
	struct PoiseRegenHandler
	{
		struct CharacterEx : RE::Actor
		{
			static inline void (*func)(CharacterEx*, float) = [](CharacterEx*, float) {};
			void Hook_Update(float a_delta);
		};
		struct PlayerEx : RE::Actor
		{
			static inline void (*func)(PlayerEx*, float) = [](PlayerEx*, float) {};
			void Hook_Update(float a_delta);
		};
		static float GetPoiseRegenRate();
		static void UpdatePoiseValue(RE::Actor* a_target, float a_delta);
	};
	struct RegenDelayHandler
	{
		static constexpr char CURRENT_REGEN_DELAY_GV[] = "MaxsuPoise_RegenDelayTimer";
		static float GetPoiseRegenDelayTimer(RE::Actor* a_target);
		static bool SetPoiseRegenDelayTimer(RE::Actor* a_target, const float& a_in);
		static float GetMaxRegenDelayTime();
	};
	struct StaggerProtectHandler
	{
		static constexpr char CURRENT_STAGGER_TIMER_GV[] = "MaxsuPoise_StaggerProtectTimer";
		static float GetStaggerProtectTimer(RE::Actor* a_target);
		static bool SetStaggerProtectTimer(RE::Actor* a_target, const float& a_in);
		static float GetMaxStaggerProtectTime();
	};
}

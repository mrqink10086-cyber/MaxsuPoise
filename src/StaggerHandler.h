#pragma once
#include "StaggerLevel.h"

namespace MaxsuPoise
{
	class StaggerHandler
	{
	public:
		static void ProcessWeaponStagger(const RE::HitData* a_hitdata);
		static void ProcessMagicStagger(RE::Actor* a_target, float a_staggerMult, RE::Actor* a_aggressor);

	private:
		static StaggerLevel GetStaggerLevel(const float& a_DamagePercent);

		StaggerHandler() = delete;

		~StaggerHandler() = delete;
	};

	class ImmuneLevelCalculator
	{
	public:
		static StaggerLevel GetTotalImmuneLevel(RE::Actor* a_target);
		static void InitializeKeywords();

	private:
		ImmuneLevelCalculator() = delete;

		~ImmuneLevelCalculator() = delete;

		static StaggerLevel GetAnimImmuneLevel(RE::Actor* a_target);
		static StaggerLevel GetAnimImmuneLevel(RE::hkbClipGenerator* a_clip);

		static inline std::array<RE::BGSKeyword*, StaggerLevel::kLargest> immuneKeywords{};
	};
}

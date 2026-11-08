#include "StaggerHandler.h"
#include "AnnotationCache.h"
#include "Hooks/PoiseRegenHandler.h"
#include "PoiseDamageCalculator.h"
#include "PoiseHealthHandler.h"
#include "Utils.h"

namespace MaxsuPoise
{
	static inline void TryStagger(RE::Actor* a_target, float a_staggerMult, RE::Actor* a_aggressor)
	{
		static const RE::BSFixedString staggerLevelName{ "MSL_StaggerLevel" };
		static const RE::BSFixedString staggerMagnitudeName{ "StaggerMagnitude" };
		std::int32_t staggerLevelGV = 0;
		if (a_target->IsStaggering() && !a_target->GetGraphVariableInt(staggerLevelName, staggerLevelGV)) {
			float currentStaggerMagnitude = 0.f;
			if (a_target->GetGraphVariableFloat(staggerMagnitudeName, currentStaggerMagnitude) && currentStaggerMagnitude - a_staggerMult >= 0.1f) {
				return;
			}
		}

		using func_t = decltype(&TryStagger);
		static REL::Relocation<func_t> func{ REL::RelocationID(36700, 37710) };
		func(a_target, a_staggerMult, a_aggressor);
	}

	void StaggerHandler::ProcessWeaponStagger(const RE::HitData* a_hitData)
	{
		if (!a_hitData) {
			return;
		}
		const auto targetRef = a_hitData->target.get();
		auto* target = targetRef.get();
		if (!target)
			return;

		const auto aggressorRef = a_hitData->aggressor.get();
		auto* aggressor = aggressorRef.get();
		if (!aggressor)
			return;

		auto staggerProtectTime = StaggerProtectHandler::GetStaggerProtectTimer(target);
		if (staggerProtectTime > 0.f && target->IsStaggering())
			return;

		auto totalPoiseHealth = PoiseHealthHandler::GetTotalPoiseHealth(target);
		auto currentPoiseHealth = PoiseHealthHandler::GetCurrentPoiseHealth(target);
		auto poiseDamage = PoiseDamageCalculator::GetWeaponPoiseDamage(a_hitData, aggressor, target);
		currentPoiseHealth -= poiseDamage;

		const auto selectedRef = RE::Console::GetSelectedRef();
		const bool diagnostics = selectedRef && target == selectedRef.get();
		auto staggerLevel = diagnostics ? GetStaggerLevel(poiseDamage / totalPoiseHealth) : StaggerLevel::kNone;
		auto immuneLevel = diagnostics ? ImmuneLevelCalculator::GetTotalImmuneLevel(target) : StaggerLevel::kNone;
		if (currentPoiseHealth <= 0.f) {
			TryStagger(target, 1.0f, aggressor);
			if (target->IsStaggering()) {
				staggerProtectTime = StaggerProtectHandler::GetMaxStaggerProtectTime();
				StaggerProtectHandler::SetStaggerProtectTimer(target, staggerProtectTime);
				currentPoiseHealth = totalPoiseHealth;
			}
		} else if (staggerProtectTime <= 0.f) {
			if (!diagnostics) {
				staggerLevel = GetStaggerLevel(poiseDamage / totalPoiseHealth);
			}
			if (staggerLevel) {
				if (!diagnostics) {
					immuneLevel = ImmuneLevelCalculator::GetTotalImmuneLevel(target);
				}
				if (staggerLevel > immuneLevel) {
					TryStagger(target, 0.25f * static_cast<float>(staggerLevel) + 0.01f, aggressor);
				}
			}
		}

		PoiseHealthHandler::SetCurrentPoiseHealth(target, currentPoiseHealth);
		RegenDelayHandler::SetPoiseRegenDelayTimer(target, RegenDelayHandler::GetMaxRegenDelayTime());

		if (diagnostics) {
			std::ostringstream logs;
			logs << "-------MaxsuPoise Weapon Stagger Result-------" << std::endl;

			if (staggerProtectTime > 0.f)
				logs << "Largest Stagger! TotalHealth: " << totalPoiseHealth << std::endl;
			else {
				logs << "TotalHealth: " << totalPoiseHealth << std::endl;
				logs << "Damage: " << poiseDamage << std::endl;
				logs << "CurrentHealth: " << currentPoiseHealth << std::endl;
				logs << "StaggerLevel: " << staggerLevel << std::endl;
				logs << "ImmuneLevel: " << immuneLevel << std::endl;
			}

			logs << "---------------------------------------" << std::endl;
			CPrint(logs.str().c_str());
		}
	}

	void StaggerHandler::ProcessMagicStagger(RE::Actor* a_target, float a_staggerMult, RE::Actor* a_aggressor)
	{
		if (!a_target)
			return;
		const RE::NiPointer<RE::Actor> targetRef{ a_target };
		const RE::NiPointer<RE::Actor> aggressorRef{ a_aggressor };

		if (a_staggerMult <= 0.f) {
			return;
		}

		if (a_target == a_aggressor)
			a_aggressor = nullptr;

		auto staggerProtectTime = StaggerProtectHandler::GetStaggerProtectTimer(a_target);
		if (staggerProtectTime > 0.f && a_target->IsStaggering())
			return;

		auto totalPoiseHealth = PoiseHealthHandler::GetTotalPoiseHealth(a_target);
		auto currentPoiseHealth = PoiseHealthHandler::GetCurrentPoiseHealth(a_target);
		auto poiseDamage = PoiseDamageCalculator::GetMagicPoiseDamage(a_target, a_staggerMult, a_aggressor);
		currentPoiseHealth -= poiseDamage;

		const auto selectedRef = RE::Console::GetSelectedRef();
		const bool diagnostics = selectedRef && a_target == selectedRef.get();
		auto staggerLevel = diagnostics ? GetStaggerLevel(poiseDamage / totalPoiseHealth) : StaggerLevel::kNone;
		auto immuneLevel = diagnostics ? ImmuneLevelCalculator::GetTotalImmuneLevel(a_target) : StaggerLevel::kNone;
		if (currentPoiseHealth <= 0.f) {
			TryStagger(a_target, 1.0f, a_aggressor);
			if (a_target->IsStaggering()) {
				staggerProtectTime = StaggerProtectHandler::GetMaxStaggerProtectTime();
				StaggerProtectHandler::SetStaggerProtectTimer(a_target, staggerProtectTime);
				currentPoiseHealth = totalPoiseHealth;
			}
		} else if (staggerProtectTime <= 0.f) {
			if (!diagnostics) {
				staggerLevel = GetStaggerLevel(poiseDamage / totalPoiseHealth);
			}
			if (staggerLevel) {
				if (!diagnostics) {
					immuneLevel = ImmuneLevelCalculator::GetTotalImmuneLevel(a_target);
				}
				if (staggerLevel > immuneLevel) {
					TryStagger(a_target, 0.25f * static_cast<float>(staggerLevel) + 0.01f, a_aggressor);
				}
			}
		}

		PoiseHealthHandler::SetCurrentPoiseHealth(a_target, currentPoiseHealth);
		RegenDelayHandler::SetPoiseRegenDelayTimer(a_target, RegenDelayHandler::GetMaxRegenDelayTime());

		if (diagnostics) {
			std::ostringstream logs;
			logs << "-------MaxsuPoise Magic Stagger Result-------" << std::endl;

			if (staggerProtectTime > 0.f)
				logs << "Largest Stagger! TotalHealth: " << totalPoiseHealth << std::endl;
			else {
				logs << "TotalHealth: " << totalPoiseHealth << std::endl;
				logs << "Damage: " << poiseDamage << std::endl;
				logs << "CurrentHealth: " << currentPoiseHealth << std::endl;
				logs << "StaggerLevel: " << staggerLevel << std::endl;
				logs << "ImmuneLevel: " << immuneLevel << std::endl;
			}

			logs << "---------------------------------------" << std::endl;
			CPrint(logs.str().c_str());
		}
	}

	StaggerLevel StaggerHandler::GetStaggerLevel(const float& a_DamagePercent)
	{
		if (a_DamagePercent < GameSettings::SmallStaggerTHLD.Get()) {
			return StaggerLevel::kNone;
		}
		if (a_DamagePercent < GameSettings::MediumStaggerTHLD.Get()) {
			return StaggerLevel::kSmall;
		}
		if (a_DamagePercent < GameSettings::LargeStaggerTHLD.Get()) {
			return StaggerLevel::kMedium;
		}
		return a_DamagePercent < 1.f ? StaggerLevel::kLarge : StaggerLevel::kLargest;
	}

	void ImmuneLevelCalculator::InitializeKeywords()
	{
		static constexpr std::array names{
			"MaxsuPoise_ImmuneSmall", "MaxsuPoise_ImmuneMedium",
			"MaxsuPoise_ImmuneLarge", "MaxsuPoise_ImmuneLargest"
		};
		for (std::size_t i = 0; i < names.size(); ++i) {
			immuneKeywords[i] = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(names[i]);
		}
	}

	StaggerLevel ImmuneLevelCalculator::GetTotalImmuneLevel(RE::Actor* a_target)
	{
		auto result = StaggerLevel::kNone;
		if (!a_target)
			return result;

		for (std::size_t i = immuneKeywords.size(); i > 0; --i) {
			if (immuneKeywords[i - 1] && a_target->HasKeyword(immuneKeywords[i - 1])) {
				result = static_cast<StaggerLevel>(i);
				break;
			}
		}
		if (result == StaggerLevel::kLargest) {
			return result;
		}

		if (const auto effects = a_target->AsMagicTarget()->GetActiveEffectList(); effects) {
			for (const auto effect : *effects) {
				if (!effect || effect->conditionStatus.get() != RE::ActiveEffect::ConditionStatus::kTrue) {
					continue;
				}
				const auto* base = effect->GetBaseObject();
				if (!base) {
					continue;
				}
				for (std::size_t i = immuneKeywords.size(); i > result; --i) {
					if (immuneKeywords[i - 1] && base->HasKeyword(immuneKeywords[i - 1])) {
						result = static_cast<StaggerLevel>(i);
						break;
					}
				}
				if (result == StaggerLevel::kLargest) {
					return result;
				}
			}
		}
		return std::max(result, GetAnimImmuneLevel(a_target));
	}

	StaggerLevel ImmuneLevelCalculator::GetAnimImmuneLevel(RE::Actor* a_actor)
	{
		StaggerLevel result = StaggerLevel::kNone;

		if (!a_actor || !a_actor->Is3DLoaded())
			return result;

		RE::BSAnimationGraphManagerPtr graphMgr;
		if (a_actor->GetAnimationGraphManager(graphMgr) && graphMgr) {
			const RE::BSSpinLockGuard guard{ graphMgr->GetRuntimeData().updateLock };
			if (graphMgr->graphs.empty()) {
				return result;
			}
			auto behaviourGraph = graphMgr->graphs[0] ? graphMgr->graphs[0]->behaviorGraph : nullptr;
			auto activeNodes = behaviourGraph ? behaviourGraph->activeNodes : nullptr;
			if (activeNodes) {
				for (const auto& nodeInfo : *activeNodes) {
					auto nodeClone = nodeInfo.nodeClone;
					if (nodeClone) {
						auto clipGenrator = ToClipGenerator(nodeClone);
						if (clipGenrator) {
							auto immuneLevel = GetAnimImmuneLevel(clipGenrator);
							if (immuneLevel > result) {
								result = immuneLevel;
							}
							if (result == StaggerLevel::kLargest) {
								return result;
							}
						}
					}
				}
			}
		}

		return result;
	}

	StaggerLevel ImmuneLevelCalculator::GetAnimImmuneLevel(RE::hkbClipGenerator* a_clip)
	{
		StaggerLevel result = StaggerLevel::kNone;

		if (!a_clip)
			return result;

		static constexpr std::string_view prefix = "MaxsuPoise";

		auto binding = a_clip ? a_clip->binding : nullptr;
		auto animation = binding ? binding->animation : nullptr;

		if (!animation || animation->annotationTracks.empty())
			return result;

		static thread_local AnnotationCache cache;
		for (const auto& anno : animation->annotationTracks[0].annotations) {
			const auto* rawText = anno.text.c_str();
			if (!rawText) {
				continue;
			}
			std::string_view text{ rawText };
			if (text.starts_with(prefix)) {
				auto annoInfo = cache.Get(text.substr(prefix.size()), [](std::string_view a_json) -> std::optional<AnnoInfo> {
					try {
						return json::parse(a_json).get<AnnoInfo>();
					} catch (const json::exception& ex) {
						ERROR("Caught an exception when converting annotation: {}", ex.what());
						return std::nullopt;
					}
				});
				if (!annoInfo) {
					continue;
				}
				annoInfo->startTime = anno.time;
				if (annoInfo->IsInFrames(a_clip->localTime) && annoInfo->immuneLevel > result) {
					result = annoInfo->immuneLevel;
					if (result == StaggerLevel::kLargest) {
						return result;
					}
				}
			}
		}

		return result;
	}
}

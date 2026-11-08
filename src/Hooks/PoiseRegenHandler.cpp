#include "PoiseRegenHandler.h"
#include "PoiseHealthHandler.h"
#include "Utils.h"

namespace MaxsuPoise
{
	void PoiseRegenHandler::CharacterEx::Hook_Update(float a_delta)
	{
		func(this, a_delta);
		UpdatePoiseValue(this, RE::GetSecondsSinceLastFrame());
	}

	void PoiseRegenHandler::PlayerEx::Hook_Update(float a_delta)
	{
		func(this, a_delta);
		UpdatePoiseValue(this, a_delta);
	}

	void PoiseRegenHandler::UpdatePoiseValue(RE::Actor* a_target, float a_delta)
	{
		if (!a_target || !a_target->GetActorRuntimeData().currentProcess || !a_target->GetActorRuntimeData().currentProcess->high || !a_target->Is3DLoaded() || !a_target->IsInitialized())
			return;

		const auto totalPoiseHealth = PoiseHealthHandler::GetTotalPoiseHealth(a_target);
		auto currentPoiseHealth = PoiseHealthHandler::GetCurrentPoiseHealth(a_target);
		const auto previousPoiseHealth = currentPoiseHealth;

		if (!a_target->IsInCombat() || (currentPoiseHealth - totalPoiseHealth) >= 0.01f) {
			currentPoiseHealth = totalPoiseHealth;
		} else if (!a_target->IsStaggering()) {
			auto regenDelayTimer = RegenDelayHandler::GetPoiseRegenDelayTimer(a_target);
			if (regenDelayTimer > 0.f)
				RegenDelayHandler::SetPoiseRegenDelayTimer(a_target, regenDelayTimer - a_delta);
			else {
				currentPoiseHealth += totalPoiseHealth * a_delta * GetPoiseRegenRate();
			}

			auto staggerProtectTimer = StaggerProtectHandler::GetStaggerProtectTimer(a_target);
			if (staggerProtectTimer > 0.f)
				StaggerProtectHandler::SetStaggerProtectTimer(a_target, staggerProtectTimer - a_delta);
		}

		if (currentPoiseHealth != previousPoiseHealth) {
			PoiseHealthHandler::SetCurrentPoiseHealth(a_target, currentPoiseHealth);
		}
	}

	float PoiseRegenHandler::GetPoiseRegenRate()
	{
		return GameSettings::PoiseRegen.Get();
	}

	float RegenDelayHandler::GetMaxRegenDelayTime()
	{
		return GameSettings::RegenDelayTime.Get();
	}

	float RegenDelayHandler::GetPoiseRegenDelayTimer(RE::Actor* a_target)
	{
		static const RE::BSFixedString variableName{ CURRENT_REGEN_DELAY_GV };
		float result = 0.f;
		if (!a_target || !a_target->GetGraphVariableFloat(variableName, result))
			WARN("Not Graph Variable Float Found: {}", CURRENT_REGEN_DELAY_GV);

		return result;
	}

	bool RegenDelayHandler::SetPoiseRegenDelayTimer(RE::Actor* a_target, const float& a_in)
	{
		static const RE::BSFixedString variableName{ CURRENT_REGEN_DELAY_GV };
		return a_target && a_target->SetGraphVariableFloat(variableName, a_in);
	}

	float StaggerProtectHandler::GetStaggerProtectTimer(RE::Actor* a_target)
	{
		static const RE::BSFixedString variableName{ CURRENT_STAGGER_TIMER_GV };
		float result = 0.f;
		if (!a_target || !a_target->GetGraphVariableFloat(variableName, result))
			WARN("Not Graph Variable Float Found: {}", CURRENT_STAGGER_TIMER_GV);

		return result;
	}

	bool StaggerProtectHandler::SetStaggerProtectTimer(RE::Actor* a_target, const float& a_in)
	{
		static const RE::BSFixedString variableName{ CURRENT_STAGGER_TIMER_GV };
		return a_target && a_target->SetGraphVariableFloat(variableName, a_in);
	}

	float StaggerProtectHandler::GetMaxStaggerProtectTime()
	{
		return GameSettings::StaggerProtectTime.Get();
	}

}

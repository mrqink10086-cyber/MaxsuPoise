#include "PoiseHealthHandler.h"
#include "SettingsHandler.h"
#include "Utils.h"

namespace MaxsuPoise
{
	static constexpr char CURRENT_POISE_HEALTH_GV[] = "MaxsuPoise_PoiseHealth";

	float PoiseHealthHandler::GetCurrentPoiseHealth(RE::Actor* a_target)
	{
		static const RE::BSFixedString variableName{ CURRENT_POISE_HEALTH_GV };
		float result = 0.f;
		if (!a_target || !a_target->GetGraphVariableFloat(variableName, result))
			WARN("Not Graph Variable Float Found: {}", CURRENT_POISE_HEALTH_GV);

		return result;
	}

	bool PoiseHealthHandler::SetCurrentPoiseHealth(RE::Actor* a_target, const float& a_in)
	{
		static const RE::BSFixedString variableName{ CURRENT_POISE_HEALTH_GV };
		return a_target && a_target->SetGraphVariableFloat(variableName, a_in);
	}

	float PoiseHealthHandler::GetTotalPoiseHealth(RE::Actor* a_target)
	{
		if (!a_target)
			return 0.f;

		auto actorPoiseHealth = GetBasePoiseHealth() * GetActorMass(a_target) * a_target->GetScale();
		auto armorPoiseHealth = GetTotalArmorPoiseHealth(a_target);
		return actorPoiseHealth + armorPoiseHealth;
	}

	float PoiseHealthHandler::GetBasePoiseHealth()
	{
		return GameSettings::BasePoiseHealth.Get();
	}

	float PoiseHealthHandler::GetTotalArmorPoiseHealth(RE::Actor* a_target)
	{
		float result = 0.f;
		const auto configuration = SettingsHandler::GetConfiguration();

		if (!a_target || configuration->armorSlotMult.empty())
			return result;

		auto baseArmorPoiseHealth = GetBaseArmorPoiseHealth();
		auto heavyArmorBouns = GetHeavyArmorBouns();

		const auto inventory = a_target->GetInventory([](RE::TESBoundObject& a_object) {
			return a_object.IsArmor();
		});
		const auto* skin = a_target->GetSkin();
		for (const auto& pair : configuration->armorSlotMult) {
			auto slot = pair.first;
			const RE::TESObjectARMO* armor = nullptr;
			for (const auto& [item, data] : inventory) {
				const auto& [count, entry] = data;
				if (count > 0 && entry && entry->IsWorn()) {
					const auto* candidate = item->As<RE::TESObjectARMO>();
					if (candidate && candidate->HasPartOf(slot)) {
						armor = candidate;
						break;
					}
				}
			}
			if (!armor) {
				armor = skin;
			}
			auto armorHealth = baseArmorPoiseHealth * pair.second;
			if (armor && (armor->IsLightArmor() || armor->IsHeavyArmor())) {
				result += armor->IsLightArmor() ? armorHealth : armorHealth * (1 + heavyArmorBouns);
			}
		}

		return result;
	}

	float PoiseHealthHandler::GetBaseArmorPoiseHealth()
	{
		return GameSettings::BaseArmorPoiseHealth.Get();
	}

	float PoiseHealthHandler::GetHeavyArmorBouns()
	{
		return GameSettings::HeavyArmorPoiseBonus.Get();
	}
}

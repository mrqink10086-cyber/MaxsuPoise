#pragma once

#include <atomic>
#include <cstdint>
#include <type_traits>

namespace MaxsuPoise::GameSettings
{
	template <class T>
	class CachedSetting
	{
	public:
		constexpr CachedSetting(const char* a_name, T a_fallback) :
			_name(a_name), _fallback(a_fallback)
		{}

		T Get() const
		{
			auto* setting = _setting.load(std::memory_order_relaxed);
			if (!setting) {
				setting = Resolve();
			}
			if (!setting) {
				return _fallback;
			}
			if constexpr (std::is_same_v<T, float>) {
				return setting->GetFloat();
			} else if constexpr (std::is_same_v<T, bool>) {
				return setting->GetBool();
			} else {
				return setting->GetUnsignedInteger();
			}
		}

		RE::Setting* Resolve() const
		{
			auto* collection = RE::GameSettingCollection::GetSingleton();
			auto* setting = collection ? collection->GetSetting(_name) : nullptr;
			_setting.store(setting, std::memory_order_relaxed);
			return setting;
		}

	private:
		const char* _name;
		T _fallback;
		mutable std::atomic<RE::Setting*> _setting{ nullptr };
	};

	inline CachedSetting<float> PoiseRegen{ "fMaxsuPoise_PoiseRegen", 0.05f };
	inline CachedSetting<float> RegenDelayTime{ "fMaxsuPoise_RegenDelayTime", 1.0f };
	inline CachedSetting<float> StaggerProtectTime{ "fMaxsuPoise_StaggerProtectTime", 0.85f };
	inline CachedSetting<float> BasePoiseHealth{ "fMaxsuPoise_BasePoiseHealth", 40.f };
	inline CachedSetting<float> BaseArmorPoiseHealth{ "fMaxsuPoise_BaseArmorPoiseHealth", 50.f };
	inline CachedSetting<float> HeavyArmorPoiseBonus{ "fMaxsuPoise_HeavyArmorPoiseBonus", 0.5f };
	inline CachedSetting<float> BaseMeleePoiseDamage{ "fMaxsuPoise_BaseMeleePoiseDamage", 10.5f };
	inline CachedSetting<float> BaseRangePoiseDamage{ "fMaxsuPoise_BaseRangePoiseDamage", 10.5f };
	inline CachedSetting<float> BaseMagicPoiseDamage{ "fMaxsuPoise_BaseMagicPoiseDamage", 100.f };
	inline CachedSetting<float> SmallStaggerTHLD{ "fMaxsuPoise_SmallStaggerTHLD", 0.17f };
	inline CachedSetting<float> MediumStaggerTHLD{ "fMaxsuPoise_MediumStaggerTHLD", 0.25f };
	inline CachedSetting<float> LargeStaggerTHLD{ "fMaxsuPoise_LargeStaggerTHLD", 0.5f };
	inline CachedSetting<float> BarFlashThreshold{ "fMaxsuPoise_BarFlashThreshold", 30.f };
	inline CachedSetting<std::uint32_t> BlockedMode{ "uMaxsuPoise_BlockedMode", 0 };
	inline CachedSetting<bool> EnableTrueHUD{ "bMaxsuPoise_EnableTrueHUD", true };

	inline void Refresh()
	{
		PoiseRegen.Resolve();
		RegenDelayTime.Resolve();
		StaggerProtectTime.Resolve();
		BasePoiseHealth.Resolve();
		BaseArmorPoiseHealth.Resolve();
		HeavyArmorPoiseBonus.Resolve();
		BaseMeleePoiseDamage.Resolve();
		BaseRangePoiseDamage.Resolve();
		BaseMagicPoiseDamage.Resolve();
		SmallStaggerTHLD.Resolve();
		MediumStaggerTHLD.Resolve();
		LargeStaggerTHLD.Resolve();
		BarFlashThreshold.Resolve();
		BlockedMode.Resolve();
		EnableTrueHUD.Resolve();
	}
}

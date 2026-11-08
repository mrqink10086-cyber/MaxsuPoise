#include "SettingsHandler.h"
#include "SimpleIni.h"

namespace MaxsuPoise
{
	static constexpr char fileName[] = R"(Data\SKSE\Plugins\MaxsuPoise.ini)";

	bool SettingsHandler::Register()
	{
		ReloadConfiguration();

		static SettingsHandler singleton;
		auto eventSource = SKSE::GetModCallbackEventSource();
		if (!eventSource) {
			ERROR("EventSource not found!");
			return false;
		}

		eventSource->AddEventSink(&singleton);

		INFO("Register {}", typeid(singleton).name());

		return true;
	}

	void SettingsHandler::ReloadConfiguration()
	{
		try {
			auto next = std::make_shared<Configuration>(*GetConfiguration());
			UpdateWeapTypeMult(*next);
			InitArmorSlotMult(*next);
			configuration.store(std::move(next), std::memory_order_release);
		} catch (const std::exception& ex) {
			ERROR("Unable to reload configuration: {}", ex.what());
		}
	}

	void SettingsHandler::UpdateWeapTypeMult(Configuration& a_configuration)
	{
		auto& WeapTypeEnumTbl = dku::static_enum<WEAPON_TYPE>();

		CSimpleIniA ini;
		if (ini.LoadFile(fileName))  // Load the ini file
			ERROR("Get Error When loading file {}", fileName);

		// get a pointer to the "WeaponTypeMult" section
		const CSimpleIniA::TKeyVal* section = ini.GetSection("WeaponTypeMult");
		if (!section) {
			return;
		}

		// iterate through the key-value pairs in the section
		for (CSimpleIniA::TKeyVal::const_iterator it = section->begin(); it != section->end(); ++it) {
			const char* key = it->first.pItem;
			const char* value = it->second;
			auto weapEnum = WeapTypeEnumTbl.from_string(key);
			if (weapEnum.has_value()) {
				const auto index = static_cast<std::size_t>(weapEnum.value());
				if (index < a_configuration.weapTypeMult.size()) {
					a_configuration.weapTypeMult[index] = std::stof(value);
				}
			}
		}
	}

	void SettingsHandler::InitArmorSlotMult(Configuration& a_configuration)
	{
		static constexpr char armorSlotFile[] = R"(Data\SKSE\Plugins\MaxsuPoise_ArmorSlot.ini)";

		auto& BipeSlotEnumTbl = dku::static_enum<BipedSlot>();

		CSimpleIniA ini;
		if (ini.LoadFile(armorSlotFile) < 0) {
			ERROR("Get Error When loading file {}", armorSlotFile);
			return;
		}

		// get a pointer to the "ArmorSlotMult" section
		const CSimpleIniA::TKeyVal* section = ini.GetSection("ArmorSlotMult");
		if (!section) {
			return;
		}

		std::map<BipedSlot, float> slotMult;
		for (CSimpleIniA::TKeyVal::const_iterator it = section->begin(); it != section->end(); ++it) {
			const char* key = it->first.pItem;
			const char* value = it->second;
			auto armorSlotEnum = BipeSlotEnumTbl.from_string(key);
			if (armorSlotEnum.has_value()) {
				slotMult[armorSlotEnum.value()] = std::stof(value);
			}
		}
		a_configuration.armorSlotMult.assign(slotMult.begin(), slotMult.end());
	}

}

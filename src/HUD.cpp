#include "HUD.h"
#include "PoiseHealthHandler.h"
#include "SettingsHandler.h"
#include "Utils.h"

namespace MaxsuPoise
{
	HUD::HUD()
	{
		if (!GameSettings::EnableTrueHUD.Get()) {
			WARN("TrueHUD integration disabled");
			return;
		}

		api.store(reinterpret_cast<TRUEHUD_API::IVTrueHUD4*>(TRUEHUD_API::RequestPluginAPI(TRUEHUD_API::InterfaceVersion::V4)), std::memory_order_release);
		if (!api.load(std::memory_order_acquire)) {
			WARN("TrueHUD not found");
		}
	}

	void HUD::FlashSpecialBar(RE::Actor* actor, float current, float total)
	{
		auto* hudAPI = api.load(std::memory_order_acquire);
		if (!hudAPI || !actor || !(total > 0.f)) {
			return;
		}
		float barFlashThreshold = GameSettings::BarFlashThreshold.Get();

		if (actor && barFlashThreshold > 0) {
			bool belowThreshold = ((current / total) * 100.0) < barFlashThreshold;
			int flashType = 0;

			if (belowThreshold) {
				if (actor->IsPlayerRef()) {
					if (flashed < 0 || current < flashed) {
						flashType = 1;  // long
						flashed = current;
					} else {
						flashType = 0;
					}
				} else if (actor->AsActorState()->actorState2.staggered) {
					flashType = 2;  // short
				}

				if (flashType > 0) {
					hudAPI->FlashActorSpecialBar(SKSE::GetPluginHandle(), actor->GetHandle(), flashType == 1 ? true : false);
					if (actor->IsPlayerRef() && flashed > 0) {
						hudAPI->OverrideSpecialBarColor(actor->GetHandle(), TRUEHUD_API::BarColorType::FlashColor, 0xFF0000);
					}
				}
			} else if (actor->IsPlayerRef()) {
				if (flashed >= 0) {
					hudAPI->RevertSpecialBarColor(actor->GetHandle(), TRUEHUD_API::BarColorType::FlashColor);
					flashed = -1;
				}
			}
		}
	}

	void HUD::Initialize()
	{
		auto* hudAPI = api.load(std::memory_order_acquire);
		if (!hudAPI) {
			return;
		}

		auto getCurrentSpecial = [](RE::Actor* actor) -> float {
			const RE::NiPointer<RE::Actor> actorRef{ actor };
			float total = PoiseHealthHandler::GetTotalPoiseHealth(actorRef.get());
			float current = PoiseHealthHandler::GetCurrentPoiseHealth(actorRef.get());
			FlashSpecialBar(actorRef.get(), current, total);
			return current;
		};

		if (hudAPI->RequestSpecialResourceBarsControl(SKSE::GetPluginHandle()) == TRUEHUD_API::APIResult::OK) {
			if (hudAPI->RegisterSpecialResourceFunctions(SKSE::GetPluginHandle(),
					getCurrentSpecial,
					PoiseHealthHandler::GetTotalPoiseHealth, true) != TRUEHUD_API::APIResult::OK) {
				WARN("Error registering TrueHUD callback functions");
				api.store(nullptr, std::memory_order_release);
			}
		}
	}
}

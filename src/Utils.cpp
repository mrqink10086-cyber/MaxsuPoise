#include "Utils.h"

namespace MaxsuPoise
{

	float GetActorMass(RE::Actor* a_target)
	{
		auto* base = a_target ? a_target->GetActorBase() : nullptr;
		if (!base || !base->race)
			return 0.f;

		return base->GetActorValue(RE::ActorValue::kMass);
	}

	void CPrint(const char* a_fmt, ...)
	{
		auto console = RE::ConsoleLog::GetSingleton();
		if (console) {
			std::va_list args;
			va_start(args, a_fmt);
			console->VPrint(a_fmt, args);
			va_end(args);
		}
	}

	void ApplyPerkEntryPoint(INT32 entry, RE::Actor* actor_a, RE::Actor* actor_b, float* out)
	{
		RE::BGSEntryPoint::HandleEntryPoint(static_cast<RE::BGSEntryPoint::ENTRY_POINT>(entry), actor_a, actor_b, out);
	}

	RE::hkbClipGenerator* ToClipGenerator(RE::hkbNode* a_node)
	{
		constexpr char CLASS_NAME[] = "hkbClipGenerator";

		if (const auto* type = a_node ? a_node->GetClassType() : nullptr; type) {
			if (type->name && _strcmpi(type->name, CLASS_NAME) == 0)
				return skyrim_cast<RE::hkbClipGenerator*>(a_node);
		}

		return nullptr;
	}

}

#pragma once
#include "AnnoInfo.h"
#include "GameSettings.h"

namespace MaxsuPoise
{

	float GetActorMass(RE::Actor* a_target);

	void CPrint(const char* a_fmt, ...);

	void ApplyPerkEntryPoint(INT32 entry, RE::Actor* actor_a, RE::Actor* actor_b, float* out);

	RE::hkbClipGenerator* ToClipGenerator(RE::hkbNode* a_node);

}

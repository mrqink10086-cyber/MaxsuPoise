#include "LoadGame.h"

DLLEXPORT constinit auto SKSEPlugin_Version = []() noexcept {
	SKSE::PluginVersionData data{};

	data.PluginVersion(Plugin::Version);
	data.PluginName(Plugin::NAME);
	data.AuthorName(Plugin::AUTHOR);
	data.UsesUpdatedStructs();
#if defined(ENABLE_SKYRIM_SE) && defined(ENABLE_SKYRIM_AE)
	data.CompatibleVersions({ SKSE::RUNTIME_SSE_1_5_97, SKSE::RUNTIME_SSE_1_6_1170 });
#elif defined(ENABLE_SKYRIM_SE)
	data.CompatibleVersions({ SKSE::RUNTIME_SSE_1_5_97 });
#else
	data.CompatibleVersions({ SKSE::RUNTIME_SSE_1_6_1170 });
#endif
#ifdef ENABLE_SKYRIM_SE
	data.MinimumRequiredXSEVersion(REL::Version{ 2, 0, 20, 0 });
#else
	data.MinimumRequiredXSEVersion(REL::Version{ 2, 2, 6, 0 });
#endif

	return data;
}();

SKSE_PLUGIN_QUERY(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;

#ifdef ENABLE_SKYRIM_SE
	return !a_skse->IsEditor() && a_skse->RuntimeVersion() == SKSE::RUNTIME_SSE_1_5_97;
#else
	return false;
#endif
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse)
{
	const auto runtime = a_skse->RuntimeVersion();
	bool supported = false;
#ifdef ENABLE_SKYRIM_SE
	supported |= runtime == SKSE::RUNTIME_SSE_1_5_97;
#endif
#ifdef ENABLE_SKYRIM_AE
	supported |= runtime == SKSE::RUNTIME_SSE_1_6_1170;
#endif
	const auto minimumSKSE = runtime == SKSE::RUNTIME_SSE_1_6_1170 ? REL::Version{ 2, 2, 6, 0 } : REL::Version{ 2, 0, 20, 0 };
	if (a_skse->IsEditor() || !supported || a_skse->SKSEVersion() < minimumSKSE.pack()) {
		return false;
	}

	SKSE::Init(a_skse, { .trampoline = true, .trampolineSize = 4 * 14 });

	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version.string());

	auto g_message = SKSE::GetMessagingInterface();
	if (!g_message) {
		ERROR("Messaging Interface Not Found!");
		return false;
	}

	return g_message->RegisterListener(MaxsuPoise::EventCallback);
}

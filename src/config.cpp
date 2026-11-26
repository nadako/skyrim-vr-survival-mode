#include "config.h"
#include "REX/REX/INI.h"

namespace Config {
	namespace {
		REX::INI::Str<> sLogLevel{"Debug", "sLogLevel", "info"};
		REX::INI::Bool<> sPoachedBugsCompat{"Compat", "bPoachedBugsVR060GetWeightRedirect", true};
	}

	bool IsPoachedBugsVRCompatEnabled() {
		return sPoachedBugsCompat.GetValue();
	}

	void LoadConfigFile() {
		static auto configPath = std::format("Data/SKSE/Plugins/{}.ini", SKSE::GetPluginName());

		auto store = REX::INI::SettingStore::GetSingleton();
		store->Init(configPath.c_str(), "");
		store->Load();

		auto logLevel = spdlog::level::from_str(sLogLevel.GetValue());
		spdlog::set_level(logLevel);
		spdlog::flush_on(logLevel);
	}
}

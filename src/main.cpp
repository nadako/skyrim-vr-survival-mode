#include "config.h"
#include "dobj.h"
#include "gmst.h"
#include "papyrus.h"
#include "conditions.h"
#include "fasttravel.h"
#include "itemweight.h"
#include "levelup.h"
#include "toggle.h"
#include "description.h"
#include "hud.h"

namespace {
	void OnMessage(SKSE::MessagingInterface::Message *message) {
		switch (message->type) {
			case SKSE::MessagingInterface::kDataLoaded:
				DOBJ::ResolveForms();
				break;
		}
	}
}

SKSEPluginLoad(SKSE::LoadInterface *skse) {
	constexpr size_t trampolineSize = DOBJ::trampolineSize + Papyrus::trampolineSize + FastTravel::trampolineSize + ItemWeight::trampolineSize + LevelUp::trampolineSize + Description::trampolineSize + HUD::trampolineSize;
	SKSE::Init(skse, {.trampoline = true, .trampolineSize = trampolineSize});

	auto plugin = SKSE::PluginDeclaration::GetSingleton();
	logger::info("Initializing version: {}", plugin->GetVersion());

	Config::LoadConfigFile();
	DOBJ::InstallHooks();
	GMST::RegisterGameSettings();
	Papyrus::RegisterNatives();
	Papyrus::InstallHooks();
	Conditions::Install();
	FastTravel::InstallHooks();
	ItemWeight::InstallHooks();
	LevelUp::InstallHooks();
	Toggle::InstallHooks();
	Description::InstallHooks();
	HUD::InstallHooks();
	SKSE::GetMessagingInterface()->RegisterListener(OnMessage);

	logger::info("Initialization complete");
	return true;
}

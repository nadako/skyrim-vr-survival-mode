#include "hud.h"
#include "dobj.h"

namespace HUD {
	namespace {
		struct State {
			int modeToggle = -1;
			int temperatureLevel = -1;
			int coldPenaltyPercent = -1;
			int hungerPenaltyPercent = -1;
			int exhaustionPenaltyPercent = -1;
		};

		State lastState;

		int GetGlobalValue(DOBJ::DefaultObject object) {
			// globals aren't there until we resolve the forms
			auto global = DOBJ::GetGlobal(object);
			return global ? (int) global->value : 0;
		}

		// SkyrimSE.exe+934a50
		struct HUDMenu_AdvanceMovie {
			static void thunk(RE::HUDMenu *menu, float interval, uint32_t currentTime) {
				original(menu, interval, currentTime);

				if (RE::UI::GetSingleton()->GameIsPaused()) return;

				State newState;
				newState.modeToggle = GetGlobalValue(DOBJ::kDOBJModeToggle);
				newState.temperatureLevel = GetGlobalValue(DOBJ::kDOBJTemperature);
				newState.coldPenaltyPercent = GetGlobalValue(DOBJ::kDOBJColdPenalty);
				newState.hungerPenaltyPercent = GetGlobalValue(DOBJ::kDOBJHungerPenalty);
				newState.exhaustionPenaltyPercent = GetGlobalValue(DOBJ::kDOBJSleepPenalty);

				bool valuesChanged = newState.temperatureLevel != lastState.temperatureLevel ||
				                     newState.coldPenaltyPercent != lastState.coldPenaltyPercent ||
				                     newState.hungerPenaltyPercent != lastState.hungerPenaltyPercent ||
				                     newState.exhaustionPenaltyPercent != lastState.exhaustionPenaltyPercent;
				bool show = newState.modeToggle == -1 || newState.modeToggle != lastState.modeToggle;
				if (newState.modeToggle != lastState.modeToggle || (lastState.modeToggle == 1 && valuesChanged)) {
					lastState = newState;
					// original code does this too - send a message to the same instance, let's keep it I guess for compatibility
					auto ui = RE::UIMessageQueue::GetSingleton();
					auto strings = RE::InterfaceStrings::GetSingleton();
					auto hudData = (RE::HUDData *) ui->CreateUIMessageData(strings->hudData);
					if (!hudData) return;
					hudData->type = RE::GetHUDMessageType(RE::HUD_MESSAGE_TYPE::kSurvival);
					hudData->show = show;
					ui->AddMessage(strings->hudMenu, RE::UI_MESSAGE_TYPE::kUpdate, hudData);
				}
			}

			static inline REL::Relocation<decltype(thunk)> original;

			static void Install() {
				logger::debug("Installing HUDMenu::AdvanceMovie hook");
				REL::Relocation<uintptr_t> vtable{RE::HUDMenu::VTABLE[0]};
				original = vtable.write_vfunc(5, thunk);
			}
		};

		// SkyrimSE.exe+933180
		struct HUDMenu_ProcessMessage {
			static RE::UI_MESSAGE_RESULTS thunk(RE::HUDMenu *menu, RE::UIMessage &message) {
				// check if it's the message that we have just sent to ourselves :)
				bool isSurvivalUpdate = message.type == RE::UI_MESSAGE_TYPE::kUpdate &&
				                        message.menu == RE::InterfaceStrings::GetSingleton()->hudMenu &&
				                        message.data && ((RE::HUDData *) message.data)->type == RE::GetHUDMessageType(RE::HUD_MESSAGE_TYPE::kSurvival);
				if (!isSurvivalUpdate) return original(menu, message);

				if (!menu->uiMovie) return RE::UI_MESSAGE_RESULTS::kPassOn; // AE does this too (it also does loading menu checks, but eh, we don't post survival updates when the game is paused anyway)

				RE::GFxValue args[3];
				args[0].SetBoolean(lastState.modeToggle == 1);
				menu->uiMovie->CreateObject(&args[1]);
				args[1].SetMember("aTemperatureLevel", lastState.temperatureLevel);
				args[1].SetMember("aColdPenaltyPercent", lastState.coldPenaltyPercent);
				args[1].SetMember("aHungerPenaltyPercent", lastState.hungerPenaltyPercent);
				args[1].SetMember("aExhaustionPenaltyPercent", lastState.exhaustionPenaltyPercent);
				args[2].SetBoolean(((RE::HUDData *) message.data)->show);
				menu->GetRuntimeData().root.Invoke("ShowSurvivalElements", nullptr, args, 3);

				return RE::UI_MESSAGE_RESULTS::kPassOn;
			}

			static inline REL::Relocation<decltype(thunk)> original;

			static void Install() {
				logger::debug("Installing HUDMenu::ProcessMessage hook");
				REL::Relocation<uintptr_t> vtable{RE::HUDMenu::VTABLE[0]};
				original = vtable.write_vfunc(4, thunk);
			}
		};

		// SkyrimSE.exe+935660 (we hook the only call to it for the same effect)
		struct HUDMenu_Reset_Call {
			static void thunk() {
				original();
				lastState = {};
			}

			static inline REL::Relocation<decltype(thunk)> original;

			static void Install() {
				logger::debug("Installing HUDMenu reset call hook");
				REL::Relocation<uintptr_t> callOffset{REL::Offset(0x9080F9)};
				original = callOffset.write_call<5>(thunk);
			}
		};
	}

	void InstallHooks() {
		logger::info("Installing HUD hooks");
		HUDMenu_AdvanceMovie::Install();
		HUDMenu_ProcessMessage::Install();
		HUDMenu_Reset_Call::Install();
		logger::info("HUD hooks installed");
	}
}

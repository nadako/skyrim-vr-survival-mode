#include "levelup.h"
#include "dobj.h"

namespace LevelUp {
	namespace {
		// this flag is set to true by default, waiting and serving jailtime sets it to false, EndSleepWait (below) checks it and resets back to true
		bool WasItSleep() {
			static REL::Relocation<bool *> wasItSleep{REL::Offset(0x1EAD84D)};
			return *wasItSleep;
		}

		void EndSleepWait(RE::PlayerCharacter *player) {
			static REL::Relocation<decltype(EndSleepWait)> endSleepWait{REL::Offset(0x6BB060)};
			endSleepWait(player);
		}

		// RE'd from SkyrimSE.exe+7415a0
		// in AE, EndSleepWait has a second bool argument. if it's true, it does this and early returns
		// in VR, there's no second argument, instead we hook this into the only two call sites that pass true on AE
		bool TryOpenLevelUpMenu(RE::PlayerCharacter *player) {
			if (player->GetPlayerFlags().isLoading || !WasItSleep()) return false;

			auto infoData = player->GetVRInfoRuntimeData();
			if (!infoData->skills->CanLevelUp() || !DOBJ::IsSurvivalModeActive()) return false;

			// so if we're in survival mode and can level up, don't end the sleep and instead open StatsMenu
			// another hook in StatsMenu destructor will call the original EndSleepWait, so player wakes up after level-up :)

			infoData->sleepSeconds = 0; // this is always zeroed, before the early return
			RE::UIMessageQueue::GetSingleton()->AddMessage(RE::InterfaceStrings::GetSingleton()->statsMenu, RE::UI_MESSAGE_TYPE::kShow, nullptr);
			return true;
		}

		// SkyrimSE.exe+749725, first call of EndSleepWait(player, true)
		struct Actor_AdvanceSleepWaitTick_EndSleepWait_Call {
			static void thunk(RE::PlayerCharacter *player) {
				bool levelUpMenuOpened = TryOpenLevelUpMenu(player);
				if (!levelUpMenuOpened) original(player);
			}

			static inline REL::Relocation<decltype(thunk)> original;

			static void Install() {
				logger::debug("Installing PlayerCharacter::EndSleepWait call hook in Actor::AdvanceSleepWaitTick");
				REL::Relocation<uintptr_t> callOffset{REL::Offset(0x6C2DE6)};
				original = callOffset.write_call<5>(thunk);
			}
		};

		// SkyrimSE.exe+741589, second call of EndSleepWait(player, true)
		struct PlayerCharacter_InterruptSleepWait_EndSleepWait_Jump {
			static void thunk(RE::PlayerCharacter *player) {
				bool levelUpMenuOpened = TryOpenLevelUpMenu(player);
				if (!levelUpMenuOpened) original(player);
			}

			static inline REL::Relocation<decltype(thunk)> original;

			static void Install() {
				logger::debug("Installing PlayerCharacter::EndSleepWait jump hook in PlayerCharacter::InterruptSleepWait");
				// this is a jmp, not call, so we write_branch, not write_call
				REL::Relocation<uintptr_t> jumpOffset{REL::Offset(0x6BB047)};
				original = jumpOffset.write_branch<5>(thunk);
			}
		};

		// SkyrimSE.exe+9767cf
		// the logic here has nothing to do with CacheDrive (whatever it is), we just hook a call in the StatsMenu destructor
		// that, on AE, happens to be right after the EndSleepWait(player, false) code, so the order of operations is preserved exactly
		struct StatsMenu_DtorImpl_CacheDrive_Call {
			static void thunk() {
				auto player = RE::PlayerCharacter::GetSingleton();
				if (player->GetPlayerFlags().sleeping) {
					EndSleepWait(player); // EndSleepWait(player, false) on AE, which is the same as our EndSleepWait
				}
				original(); // continue with the destructor code
			}

			static inline REL::Relocation<decltype(thunk)> original;

			static void Install() {
				logger::debug("Installing CacheDrive call hook in StatsMenu destructor");
				REL::Relocation<uintptr_t> callOffset{REL::Offset(0x8EC0D9)};
				original = callOffset.write_call<5>(thunk);
			}
		};

		// SkyrimSE.exe+977a00
		// this blocks the level up logic in the perks menu in survival mode, unless it is showing when sleeping (so from our TryOpenLevelUpMenu)
		struct StatsMenu_ProcessMessage_CanLevelUp_Call {
			static bool thunk(RE::PlayerCharacter::PlayerSkills *skills) {
				// AE calls CanLevelUp first, then does the survival mode + sleeping check, we keep the same logic exactly
				if (!original(skills)) {
					return false;
				}
				return !DOBJ::IsSurvivalModeActive() || RE::PlayerCharacter::GetSingleton()->GetPlayerFlags().sleeping;
			}

			static inline REL::Relocation<decltype(thunk)> original;

			static void Install() {
				logger::debug("Installing PlayerSkills::CanLevelUp call hook in StatsMenu::ProcessMessage");
				REL::Relocation<uintptr_t> callOffset{REL::Offset(0x8ED3EE)};
				original = callOffset.write_call<5>(thunk);
			}
		};

		// VR has additional CanLevelUp check in StatsMenu::CanProcess: it handles the input only if we cannot level-up
		// so we need to enrich the CanLevelUp check with the same survival mode + sleeping check
		struct StatsMenu_CanProcess_CanLevelUp_Call {
			static bool thunk(RE::PlayerCharacter::PlayerSkills *skills) {
				if (!original(skills)) {
					return false;
				}
				return !DOBJ::IsSurvivalModeActive() || RE::PlayerCharacter::GetSingleton()->GetPlayerFlags().sleeping;
			}

			static inline REL::Relocation<decltype(thunk)> original;

			static void Install() {
				logger::debug("Installing PlayerSkills::CanLevelUp call hook in StatsMenu::CanProcess");
				REL::Relocation<uintptr_t> callOffset{REL::Offset(0x8ED8CE)};
				original = callOffset.write_call<5>(thunk);
			}
		};
		
		// SkyrimSE.exe+98878b
		// this extends the "level up" check in the tween menu. survival mode disables that completely, since you can only level up after sleep
		struct TweenMenu_StartOpenMenuAnim_CanLevelUp_Call {
			static bool thunk(RE::PlayerCharacter::PlayerSkills *skills) {
				// AE calls CanLevelUp first, then does the survival mode check, we keep the same logic exactly
				return original(skills) && !DOBJ::IsSurvivalModeActive();
			}

			static inline REL::Relocation<decltype(thunk)> original;

			static void Install() {
				logger::debug("Installing PlayerSkills::CanLevelUp call hook in TweenMenu::StartOpenMenuAnim");
				REL::Relocation<uintptr_t> callOffset{REL::Offset(0x8FE3E2)};
				original = callOffset.write_call<5>(thunk);
			}
		};
	}

	void InstallHooks() {
		logger::info("Installing level up hooks");
		Actor_AdvanceSleepWaitTick_EndSleepWait_Call::Install();
		PlayerCharacter_InterruptSleepWait_EndSleepWait_Jump::Install();
		StatsMenu_DtorImpl_CacheDrive_Call::Install();
		StatsMenu_ProcessMessage_CanLevelUp_Call::Install();
		StatsMenu_CanProcess_CanLevelUp_Call::Install();
		TweenMenu_StartOpenMenuAnim_CanLevelUp_Call::Install();
		logger::info("Level up hooks installed");
	}
}

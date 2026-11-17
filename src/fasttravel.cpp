#include "fasttravel.h"
#include "papyrus.h"

namespace FastTravel {
	namespace {
		RE::BSTEventSource<RE::TESFastTravelEndEvent> endEventSource;

		// SkyrimSE.exe+744600 (ExecuteCellTransition) sends the TESFastTravelEndEvent at the end, we instead hook its only call-site (UpdateCellTransition)
		struct PlayerCharacter_UpdateCellTransition_ExecuteCellTransition_Call {
			static void thunk(RE::PlayerCharacter *player, RE::TESObjectREFR *destination, bool autoSave) {
				// original does this at start to early-return, it should be cheap enough for a one-time fast-travel event, so let's just check again
				if (player->IsOverEncumbered()) {
					original(player, destination, autoSave);
					return;
				}

				// let's hope noone messed up with the calendar that would make our travelHours calculation different from the one inside the original
				auto calendar = RE::Calendar::GetSingleton();
				float hoursBefore = calendar->GetHoursPassed();
				original(player, destination, autoSave);
				float travelHours = calendar->GetHoursPassed() - hoursBefore;

				RE::TESFastTravelEndEvent event{travelHours};
				endEventSource.SendEvent(&event);
			}

			static inline REL::Relocation<decltype(thunk)> original;

			static void Install() {
				logger::debug("Installing PlayerCharacter::ExecuteCellTransition call hook in PlayerCharacter::UpdateCellTransition");
				REL::Relocation<uintptr_t> callOffset{REL::Offset(0x6BCF1A)};
				original = callOffset.write_call<5>(thunk);
			}
		};

		// SkyrimSE.exe+9e0ff0
		struct PapyrusEndEventSink : RE::BSTEventSink<RE::TESFastTravelEndEvent> {
			RE::BSEventNotifyControl ProcessEvent(const RE::TESFastTravelEndEvent *event, RE::BSTEventSource<RE::TESFastTravelEndEvent> *) override {
				// NOTE: po3's Papyrus Extender has its own OnPlayerFastTravelEnd, delivered only to forms registered via
				// RegisterForOnPlayerFastTravelEnd. A player script/alias/effect that also registers there gets the event twice.
				auto player = RE::PlayerCharacter::GetSingleton();
				static RE::BSFixedString eventName("OnPlayerFastTravelEnd");
				Papyrus::SendAndRelayEvent(player, eventName, event->fastTravelEndHours);
				return RE::BSEventNotifyControl::kContinue;
			}
		};
	}

	void InstallHooks() {
		logger::info("Installing fast travel hooks");
		PlayerCharacter_UpdateCellTransition_ExecuteCellTransition_Call::Install();
		static PapyrusEndEventSink papyrusEndEventSink;
		endEventSource.AddEventSink(&papyrusEndEventSink);
		logger::info("Fast travel hooks installed");
	}

	RE::BSTEventSource<RE::TESFastTravelEndEvent> *GetEndEventSource() {
		return &endEventSource;
	}
}

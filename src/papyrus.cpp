#include "papyrus.h"
#include "warmth.h"

namespace Papyrus {
	namespace {
		// SkyrimSE.exe+a00c50
		float Actor_GetWarmthRating(RE::Actor *self) {
			return Warmth::GetActorWarmthRating(self);
		}

		// SkyrimSE.exe+a0d720
		bool Actor_IsOverEncumbered(RE::Actor *self) {
			return self->IsOverEncumbered();
		}

		// SkyrimSE.exe+a00570
		float Actor_GetActorValueMax(RE::BSScript::IVirtualMachine *vm, RE::VMStackID stackID, RE::Actor *self, std::string_view valueName) {
			auto actorValue = RE::ActorValueList::LookupActorValueByName(valueName.data());
			if (actorValue == RE::ActorValue::kTotal) {
				vm->TraceStack(std::format("Actor value \"{}\" does not exist.", valueName).c_str(), stackID);
				return 0.0f;
			}
			return self->GetActorValueMax(actorValue);
		}

		// SkyrimSE.exe+a01e70
		void Actor_SendLycanthropyStateChanged(RE::Actor *self, bool isWerewolf) {
			static RE::BSFixedString eventName("OnLycanthropyStateChanged");
			SendAndRelayEvent(self, eventName, isWerewolf);
		}

		// SkyrimSE.exe+a01ed0
		void Actor_SendVampirismStateChanged(RE::Actor *self, bool isVampire) {
			static RE::BSFixedString eventName("OnVampirismStateChanged");
			SendAndRelayEvent(self, eventName, isVampire);
		}

		// SkyrimSE.exe+a03320
		struct Actor_StartVampireFeed_InitiateVampireFeedPackage_Call {
			static void thunk(RE::Actor *self, RE::Actor *target, RE::TESObjectREFR *furniture) {
				// AE function is identical to VR except that it sends an event at the end
				self->InitiateVampireFeedPackage(target, furniture);
				static RE::BSFixedString eventName("OnVampireFeed");
				SendAndRelayEvent(self, eventName, target);
			}

			static void Install() {
				logger::debug("Installing Actor::InitiateVampireFeedPackage call hook in Actor.StartVampireFeed Papyrus native");
				REL::Relocation<uintptr_t> callOffset{REL::Offset(0x987956)};
				callOffset.write_call<6>(thunk); // vtable call
			}
		};

		// SkyrimSE.exe+a3ea20
		float Armor_GetWarmthRating(RE::TESObjectARMO *self) {
			return Warmth::GetBoundObjectWarmthRating(self);
		}

		// SkyrimSE.exe+a412b0
		float Light_GetWarmthRating(RE::TESObjectLIGH *self) {
			return Warmth::GetBoundObjectWarmthRating(self);
		}

		bool RegisterFunctions(RE::BSScript::IVirtualMachine *vm) {
			vm->RegisterFunction("GetWarmthRating", "Actor", Actor_GetWarmthRating);
			vm->RegisterFunction("IsOverEncumbered", "Actor", Actor_IsOverEncumbered);
			vm->RegisterFunction("GetActorValueMax", "Actor", Actor_GetActorValueMax);
			vm->RegisterFunction("SendLycanthropyStateChanged", "Actor", Actor_SendLycanthropyStateChanged);
			vm->RegisterFunction("SendVampirismStateChanged", "Actor", Actor_SendVampirismStateChanged);

			vm->RegisterFunction("GetWarmthRating", "Armor", Armor_GetWarmthRating);

			vm->RegisterFunction("GetWarmthRating", "Light", Light_GetWarmthRating);

			return true;
		}
	}

	void RegisterNatives() {
		logger::info("Registering Papyrus natives");
		SKSE::GetPapyrusInterface()->Register(RegisterFunctions);
		logger::info("Papyrus natives registered");
	}

	void InstallHooks() {
		logger::info("Installing Papyrus hooks");
		Actor_StartVampireFeed_InitiateVampireFeedPackage_Call::Install();
		logger::info("Papyrus hooks installed");
	}
}

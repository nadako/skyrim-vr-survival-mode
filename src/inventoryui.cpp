#include "inventoryui.h"
#include "dobj.h"
#include "utils.h"
#include "warmth.h"

namespace InventoryUI {
	namespace {
		// SkyrimSE.exe+9472c0
		struct ItemCard_SetItem {
			static void thunk(RE::ItemCard *itemCard, RE::InventoryEntryData *item, bool ignoreFlag) {
				original(itemCard, item, ignoreFlag);

				if (!item->object || !DOBJ::IsSurvivalModeActive()) return;
				auto armor = item->object->As<RE::TESObjectARMO>();
				if (!armor) return;

				float warmthRating = Warmth::GetBoundObjectWarmthRating(item->object);
				warmthRating = RoundPositive(warmthRating);
				itemCard->obj.SetMember("warmth", warmthRating);

				// this is roughly what AE does, but simpler
				// ignoreFlag, I believe, is only true for someone else's inventory, so it doesn't matter whether it's worn by them or not
				float warmthChange = 0;
				if (ignoreFlag || !item->IsWorn()) {
					warmthChange = warmthRating;

					auto slotMask = armor->GetSlotMask();
					auto inventoryChanges = RE::PlayerCharacter::GetSingleton()->GetInventoryChanges();
					if (inventoryChanges && inventoryChanges->entryList) {
						for (auto entry: *inventoryChanges->entryList) {
							if (!entry || !entry->object) continue;
							auto wornArmor = entry->object->As<RE::TESObjectARMO>();
							if (!wornArmor || !(wornArmor->GetSlotMask() & slotMask)) continue;
							if (entry->IsWorn()) {
								// IsWorn is last here because it does iterations and other checks are very basic
								// if we get here that means equipping the given item will unequip this one,
								// so we subtract its warmth
								warmthChange -= RoundPositive(Warmth::GetBoundObjectWarmthRating(wornArmor));
							}
						}
					}
				}
				itemCard->obj.SetMember("warmthChange", warmthChange);
			}

			static inline REL::Relocation<decltype(thunk)> original;

			static void Install() {
				logger::debug("Installing ItemCard::SetItem hook");
				// first two instructions (mov rax, rsp / mov [rax+18h], r8b) give us 7 bytes that we can replace with a jump, no rip-relatives, so just copy them
				constexpr uint8_t prologue[kSetItemPrologueSize] = {0x48, 0x8B, 0xC4, 0x44, 0x88, 0x40, 0x18};
				REL::Relocation<uintptr_t> function{REL::Offset(0x8BEBD0)};
				original = WritePrologueDetour(function.address(), prologue, sizeof(prologue), (uintptr_t) thunk, "ItemCard::SetItem");
			}
		};

		// SkyrimSE.exe+966d10
		struct FillPlayerInfoObj {
			static void thunk(RE::GFxValue *playerInfoObj) {
				original(playerInfoObj);
				if (!DOBJ::IsSurvivalModeActive()) return;
				float warmthRating = Warmth::GetActorWarmthRating(RE::PlayerCharacter::GetSingleton());
				playerInfoObj->SetMember("warmth", warmthRating);
			}

			static inline REL::Relocation<decltype(thunk)> original;

			static void Install() {
				logger::debug("Installing FillPlayerInfoObj hook");
				// first three instructions (mov rax, rsp / push rbp / push rsi) give us 5 bytes that we can replace with a jump, no rip-relatives, so just copy them
				constexpr uint8_t prologue[kFillPlayerInfoObjPrologueSize] = {0x48, 0x8B, 0xC4, 0x55, 0x56};
				REL::Relocation<uintptr_t> function{REL::Offset(0x8DC6D0)};
				original = WritePrologueDetour(function.address(), prologue, sizeof(prologue), (uintptr_t) thunk, "FillPlayerInfoObj");
			}
		};
	}

	void InstallHooks() {
		logger::info("Installing inventory UI hooks");
		ItemCard_SetItem::Install();
		FillPlayerInfoObj::Install();
		logger::info("Inventory UI hooks installed");
	}
}

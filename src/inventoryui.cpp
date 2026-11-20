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
				if (!item->object || !item->object->IsArmor() || !DOBJ::IsSurvivalModeActive()) return;
				float warmthRating = Warmth::GetBoundObjectWarmthRating(item->object);
				warmthRating = RoundPositive(warmthRating);
				itemCard->obj.SetMember("warmth", warmthRating);
				// TODO: add warmthChange for the bottombar
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
	}

	void InstallHooks() {
		logger::info("Installing inventory UI hooks");
		ItemCard_SetItem::Install();
		logger::info("Inventory UI hooks installed");
	}
}

#include "warmth.h"
#include "gmst.h"
#include "dobj.h"

namespace Warmth {
	namespace {
		constexpr RE::ActorValue kWarmthActorValue = RE::ActorValue::kVariable09;
		static_assert((int) kWarmthActorValue == 76);

		// SkyrimSE.exe+41e3c0 (also inlined in 41e180)
		bool ClassifyKeyword(RE::BGSKeyword *keyword, uint32_t &slotMask, GMST::Row &row) {
			if (keyword == DOBJ::GetKeyword(DOBJ::kDOBJKeywordJewelry)) {
				// jewelry never gives warmth, I guess
				slotMask = 0;
				return false;
			}

			if (keyword == DOBJ::GetKeyword(DOBJ::kDOBJKeywordCold)) {
				row = GMST::kRowCold;
			} else if (keyword == DOBJ::GetKeyword(DOBJ::kDOBJKeywordWarm)) {
				row = GMST::kRowWarm;
			} else if (keyword == DOBJ::GetKeyword(DOBJ::kDOBJKeywordArmorBody) ||
			           keyword == DOBJ::GetKeyword(DOBJ::kDOBJKeywordClothingBody)) {
				slotMask |= 1 << GMST::kSlotBody;
			} else if (keyword == DOBJ::GetKeyword(DOBJ::kDOBJKeywordArmorHead) ||
			           keyword == DOBJ::GetKeyword(DOBJ::kDOBJKeywordClothingHead)) {
				slotMask |= 1 << GMST::kSlotHead;
			} else if (keyword == DOBJ::GetKeyword(DOBJ::kDOBJKeywordArmorHands) ||
			           keyword == DOBJ::GetKeyword(DOBJ::kDOBJKeywordClothingHands)) {
				slotMask |= 1 << GMST::kSlotHands;
			} else if (keyword == DOBJ::GetKeyword(DOBJ::kDOBJKeywordArmorFeet) ||
			           keyword == DOBJ::GetKeyword(DOBJ::kDOBJKeywordClothingFeet)) {
				slotMask |= 1 << GMST::kSlotFeet;
			}
			return true;
		}
	}

	// SkyrimSE.exe+41dda0
	float GetBoundObjectWarmthRating(RE::TESBoundObject *object) {
		if (object->GetFormType() != RE::FormType::Armor) {
			if (object->GetFormType() == RE::FormType::Light) {
				return GMST::GetTorchBonus();
			}
			return 0;
		}

		auto armor = (RE::TESObjectARMO *) object;

		uint32_t slotMask = 0;
		GMST::Row row = GMST::kRowNormal;

		for (uint32_t i = 0; i < armor->numKeywords; i++) {
			auto keyword = armor->keywords[i];
			if (keyword && !ClassifyKeyword(keyword, slotMask, row)) {
				break;
			}
		}
		if (auto defaultKeyword = armor->GetDefaultKeyword()) {
			ClassifyKeyword(defaultKeyword, slotMask, row);
		}

		if (slotMask == 0) return 0.0f;

		float warmth = 0.0f;
		for (uint32_t slot = 0; slot < GMST::kSlotCount; slot++) {
			if (slotMask & (1 << slot)) {
				warmth += GMST::GetSlotBonus(row, (GMST::Slot) slot);
			}
		}
		return warmth * GMST::GetArmorScalar();
	}

	// SkyrimSE.exe+41de80
	float GetActorWarmthRating(RE::Actor *actor) {
		struct WornItemsVisitor : RE::InventoryChanges::IItemChangeVisitor {
			float warmth = 0.0f;
			// SkyrimSE.exe+41e700
			virtual RE::BSContainer::ForEachResult Visit(RE::InventoryEntryData *entry) {
				if (entry->object)
					warmth += GetBoundObjectWarmthRating(entry->object);
				return RE::BSContainer::ForEachResult::kContinue;
			}
		};
		float warmth = 0.0f;
		if (auto inventoryChanges = actor->GetInventoryChanges(true)) {
			WornItemsVisitor visitor;
			inventoryChanges->VisitWornItems(visitor);
			warmth = visitor.warmth;
		}
		return warmth + actor->AsActorValueOwner()->GetActorValue(kWarmthActorValue);
	}
}

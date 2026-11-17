#pragma once

namespace GMST {
	// don't reorder, those are used as slot mask bits
	enum Slot : uint32_t {
		kSlotBody,
		kSlotHead,
		kSlotHands,
		kSlotFeet,
		kSlotCount
	};

	enum Row : uint32_t {
		kRowNormal,
		kRowWarm,
		kRowCold,
		kRowCount
	};

	float GetArmorScalar();
	float GetTorchBonus();
	float GetSlotBonus(Row row, Slot slot);
	void RegisterGameSettings();
}

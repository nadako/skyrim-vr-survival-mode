#pragma once

namespace InventoryUI {
	constexpr size_t kSetItemPrologueSize = 7;
	constexpr size_t trampolineSize = kSetItemPrologueSize + kTrampolineJumpSize + kTrampolineJumpSize;

	void InstallHooks();
}

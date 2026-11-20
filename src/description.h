#pragma once

namespace Description {
	constexpr size_t kGetDescriptionPrologueSize = 6;
	constexpr size_t kGetMagicItemDescriptionPrologueSize = 5;
	constexpr size_t trampolineSize = kGetDescriptionPrologueSize + kGetMagicItemDescriptionPrologueSize + kTrampolineJumpSize + kTrampolineJumpSize + kTrampolineJumpSize + kTrampolineJumpSize;

	void InstallHooks();
}

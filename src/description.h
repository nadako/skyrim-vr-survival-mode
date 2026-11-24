#pragma once

namespace Description {
	constexpr size_t kGetDescriptionPrologueSize = 6;
	constexpr size_t kGetMagicItemDescriptionPrologueSize = 5;
	constexpr size_t kGetActiveEffectDescriptionPrologueSize = 8;
	constexpr size_t trampolineSize = kGetDescriptionPrologueSize + kGetMagicItemDescriptionPrologueSize + kGetActiveEffectDescriptionPrologueSize + kTrampolineJumpSize + kTrampolineJumpSize + kTrampolineJumpSize + kTrampolineJumpSize + kTrampolineJumpSize + kTrampolineJumpSize;

	void InstallHooks();
}

#pragma once

namespace Description {
	constexpr size_t kGetDescriptionPrologueSize = 6;
	constexpr size_t kGetMagicItemDescriptionPrologueSize = 5;
	constexpr size_t kStubReturnJumpSize = 12; // mov+jmp
	constexpr size_t trampolineSize = kGetDescriptionPrologueSize + kGetMagicItemDescriptionPrologueSize + kStubReturnJumpSize + kStubReturnJumpSize + kTrampolineJumpSize + kTrampolineJumpSize;

	void InstallHooks();
}

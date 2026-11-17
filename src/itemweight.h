#pragma once

namespace ItemWeight {
	constexpr size_t kGetWeightStubSize = 26; // see GetWeight codegen
	constexpr size_t trampolineSize = kGetWeightStubSize + kTrampolineJumpSize + kTrampolineJumpSize;

	void InstallHooks();
}

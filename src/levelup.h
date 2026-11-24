#pragma once

namespace LevelUp {
	// this one has a lot of call hooks...
	constexpr size_t trampolineSize = kTrampolineJumpSize + kTrampolineJumpSize + kTrampolineJumpSize + kTrampolineJumpSize + kTrampolineJumpSize + kTrampolineJumpSize;

	void InstallHooks();
}

#pragma once

namespace Compat {
	constexpr size_t kGetWeightCallStubSize = 97;
	constexpr size_t trampolineSize = kGetWeightCallStubSize;

	void InstallHooks();
}

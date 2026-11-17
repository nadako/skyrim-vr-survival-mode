#pragma once

namespace FastTravel {
	constexpr size_t trampolineSize = kTrampolineJumpSize;

	void InstallHooks();

	// for non-VR, other mods subscribe on ScriptEventSourceHolder, for VR, we have to provide a separate API
	RE::BSTEventSource<RE::TESFastTravelEndEvent> *GetEndEventSource();
}

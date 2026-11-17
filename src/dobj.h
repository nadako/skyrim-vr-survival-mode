#pragma once

namespace DOBJ {
	constexpr size_t trampolineSize = kTrampolineJumpSize;

	enum DefaultObject : uint32_t {
		kDOBJModeToggle,  // whether survival mode is enabled by the user (through gameplay option or first prompt) - 328 on AE 1.7
		kDOBJModeEnabled, // whether survival scripts have actually started (they react on toggle) - 329 on AE 1.7
		kDOBJModeShowOption, // whether to show the enable option (main script sets it after MQ101 or when unbound and outdoors, SMI forces it to 1) - 330 on AE 1.7

		kDOBJTemperature,

		kDOBJColdPenalty,
		kDOBJHungerPenalty,
		kDOBJSleepPenalty,

		kDOBJKeywordCold,
		kDOBJKeywordWarm,

		kDOBJKeywordArmorHands,
		kDOBJKeywordClothingHands,
		kDOBJKeywordArmorFeet,
		kDOBJKeywordClothingFeet,
		kDOBJKeywordArmorBody,
		kDOBJKeywordClothingBody,
		kDOBJKeywordArmorHead,
		kDOBJKeywordClothingHead,

		kDOBJKeywordJewelry,

		kDOBJCount
	};

	void InstallHooks();
	void ResolveForms();

	RE::TESGlobal *GetGlobal(DefaultObject object);
	RE::BGSKeyword *GetKeyword(DefaultObject object);
	bool IsSurvivalModeActive();
}

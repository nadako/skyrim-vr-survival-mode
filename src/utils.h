#pragma once

constexpr uint32_t Sig2Int(const char (&key)[5]) {
	return (uint32_t) (uint8_t) key[0] |
	       (uint32_t) (uint8_t) key[1] << 8 |
	       (uint32_t) (uint8_t) key[2] << 16 |
	       (uint32_t) (uint8_t) key[3] << 24;
}

inline float RoundPositive(float value) {
	// AE does this for warmth at SkyrimSE.exe+947f43, so here it is
	return (float) ((int) value + ((value - (float) (int) value) >= 0.5f));
}

inline uintptr_t WritePrologueDetour(uintptr_t address, const uint8_t *prologue, size_t prologueSize, uintptr_t thunk, const char *name) {
	if (memcmp((void *) address, prologue, prologueSize) != 0) {
		SKSE::stl::report_and_fail(std::format("{} doesn't start with the expected instructions.\nAnother mod has hooked the function?", name));
	}

	struct Stub : Xbyak::CodeGenerator {
		Stub(const uint8_t *prologue, size_t prologueSize, uintptr_t returnAddress) {
			db(prologue, prologueSize); // copy whatever was in the original prologue
			jmp(ptr[rip]);
			dq(returnAddress);
		}
	};
	Stub stub(prologue, prologueSize, address + prologueSize);
	if (stub.getSize() != prologueSize + kTrampolineJumpSize) {
		// this won't happen, lol
		SKSE::stl::report_and_fail(std::format("{} prologue stub is {} bytes, expected {}.", name, stub.getSize(), prologueSize + kTrampolineJumpSize));
	}
	void *original = SKSE::GetTrampoline().allocate(stub);
	SKSE::GetTrampoline().write_branch<5>(address, thunk, true);
	return (uintptr_t) original;
}

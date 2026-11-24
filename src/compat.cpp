#include "compat.h"

namespace {
	// PoachedBugsVR.dll has a custom GetWeight implementation compiled in, coming from the version of CommonLibVR it uses, so us hooking it does nothing.
	// I have reported it to the author, but for now let's just redirect it to the real engine method.
	struct PoachedBugsVR_GetWeight {
		static void Install() {
			uintptr_t poachedBugsVR = (uintptr_t) GetModuleHandleA("PoachedBugsVR.dll");
			if (!poachedBugsVR) {
				logger::info("PoachedBugsVR.dll not loaded, compat patch not needed");
				return;
			}

			uintptr_t staticGetWeightAddr = poachedBugsVR + 0x32D30; // their compiled GetWeight
			constexpr uint8_t staticGetWeightPrologue[14]{0x0F, 0xB6, 0x41, 0x1A, 0x4C, 0x8D, 0x0D, 0xC5, 0xD2, 0xFC, 0xFF, 0x83, 0xC0, 0xC3}; // first 3 instructions

			REL::Relocation<uintptr_t> realGetWeight{REL::Offset(0x1B1470)}; // game's GetWeight (that we're hooking in ItemWeight)

			struct Patch : Xbyak::CodeGenerator {
				explicit Patch(uintptr_t addr) {
					jmp(ptr[rip]); // jmp to the address below
					dq(addr);
				}
			};
			Patch patch(realGetWeight.address());

			// sanity checks
			constexpr size_t jumpSize = 14;
			static_assert(sizeof(staticGetWeightPrologue) == jumpSize);
			if (patch.getSize() != jumpSize) {
				SKSE::stl::report_and_fail(std::format("TESForm::GetWeight patch is {} bytes, expected {}.", patch.getSize(), jumpSize));
			}

			bool success = REL::safe_write(staticGetWeightAddr, patch.getCode(), patch.getSize(), staticGetWeightPrologue, sizeof(staticGetWeightPrologue));
			if (success) logger::info("PoachedBugsVR.dll patched: TESForm::GetWeight redirected into real one at {:X}", realGetWeight.address());
			else logger::warn("PoachedBugsVR.dll patch failed: TESForm::GetWeight function did not have expected prologue. DLL version mismatch?");
		}
	};
}

namespace Compat {
	void InstallHooks() {
		logger::info("Installing mod compatibility hooks");
		PoachedBugsVR_GetWeight::Install();
		logger::info("Mod compatibility hooks installed");
	}
}

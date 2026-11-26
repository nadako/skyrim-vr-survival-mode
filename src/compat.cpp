#include "compat.h"
#include "config.h"

namespace {
	// PoachedBugsVR.dll has a custom GetWeight implementation compiled in, coming from the version of CommonLibVR it uses, so us hooking it does nothing.
	// I have reported it to the author, but for now let's just redirect it to the real engine method.
	struct PoachedBugsVR_GetWeight {
		static void Install() {
			if (!Config::IsPoachedBugsVRCompatEnabled()) {
				logger::info("PoachedBugsVR.dll GetWeight patch is disabled in INI");
				return;
			}

			uintptr_t poachedBugsVR = (uintptr_t) GetModuleHandleA("PoachedBugsVR.dll");
			if (!poachedBugsVR) {
				logger::info("PoachedBugsVR.dll not loaded, compat patch not needed");
				return;
			}

			uintptr_t staticGetWeightAddr = poachedBugsVR + 0x32D30; // their compiled GetWeight
			constexpr uint8_t staticGetWeightPrologue[14]{0x0F, 0xB6, 0x41, 0x1A, 0x4C, 0x8D, 0x0D, 0xC5, 0xD2, 0xFC, 0xFF, 0x83, 0xC0, 0xC3}; // first 3 instructions

			REL::Relocation<uintptr_t> realGetWeight{REL::Offset(0x1B1470)}; // game's GetWeight (that we're hooking in ItemWeight)

			// this has to be a bit complicated because PoachedBugsVR is compiled with interprocedural optimizations,
			// so it doesn't respect volatile registers because it knows that GetWeight doesn't use them (e.g. r10)
			struct GetWeightCallStub : Xbyak::CodeGenerator {
				explicit GetWeightCallStub(uintptr_t getWeightAddress) {
					sub(rsp, 0x88); // move stack pointer: shadow space (32 bytes) plus the volatile registers not touched by PoachedBugsVR's GetWeight

					movups(ptr[rsp + 0x20], xmm1); // save registers
					movups(ptr[rsp + 0x30], xmm2);
					movups(ptr[rsp + 0x40], xmm3);
					movups(ptr[rsp + 0x50], xmm4);
					movups(ptr[rsp + 0x60], xmm5);
					mov(ptr[rsp + 0x70], r10);
					mov(ptr[rsp + 0x78], r11);

					mov(rax, getWeightAddress); // call engine GetWeight
					call(rax);

					movups(xmm1, ptr[rsp + 0x20]); // restore register values
					movups(xmm2, ptr[rsp + 0x30]);
					movups(xmm3, ptr[rsp + 0x40]);
					movups(xmm4, ptr[rsp + 0x50]);
					movups(xmm5, ptr[rsp + 0x60]);
					mov(r10, ptr[rsp + 0x70]);
					mov(r11, ptr[rsp + 0x78]);

					add(rsp, 0x88); // restore stack pointer and return
					ret();
				}
			};
			GetWeightCallStub getWeightCallStub(realGetWeight.address());
			if (getWeightCallStub.getSize() != Compat::kGetWeightCallStubSize) {
				SKSE::stl::report_and_fail(std::format("TESForm::GetWeight call stub is {} bytes, expected {}.", getWeightCallStub.getSize(), Compat::kGetWeightCallStubSize));
			}
			uintptr_t callStubAddress = (uintptr_t) SKSE::GetTrampoline().allocate(getWeightCallStub);

			struct Patch : Xbyak::CodeGenerator {
				explicit Patch(uintptr_t addr) {
					jmp(ptr[rip]); // jmp to the address below
					dq(addr);
				}
			};
			Patch patch(callStubAddress);

			// sanity checks
			constexpr size_t jumpSize = 14;
			static_assert(sizeof(staticGetWeightPrologue) == jumpSize);
			if (patch.getSize() != jumpSize) {
				SKSE::stl::report_and_fail(std::format("TESForm::GetWeight patch is {} bytes, expected {}.", patch.getSize(), jumpSize));
			}

			bool success = REL::safe_write(staticGetWeightAddr, patch.getCode(), patch.getSize(), staticGetWeightPrologue, sizeof(staticGetWeightPrologue));
			if (success) logger::info("PoachedBugsVR.dll patched: TESForm::GetWeight redirected into real one at {:X} through a register-preserving stub at {:X}", realGetWeight.address(), callStubAddress);
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

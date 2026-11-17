#include "itemweight.h"
#include "dobj.h"
#include "utils.h"

namespace ItemWeight {
	namespace {
		// TODO: allow run-time (load-time without locks?) modification
		std::unordered_map<RE::FormID, float> ammoWeights;

		struct TESForm_ReadDataChunk_ReadData_Call {
			// SkyrimSE.exe+1e37d0, TESForm::Load methods call it, we hook into an actual data read inside to extract ammo weight values
			static bool thunk(RE::TESFile *file, void *buffer, uint32_t size) {
				if (!original(file, buffer, size)) {
					return false;
				}

				// ReadData has successfully read the... data :) check whether we're reading DATA chunk of AMMO record
				if (file->currentform.form != Sig2Int("AMMO") || file->GetCurrentChunkID() != Sig2Int("DATA")) {
					return true;
				}

				// see https://en.uesp.net/wiki/Skyrim_Mod:Mod_File_Format/AMMO, SSE structs contain extra float field for weight
				struct AMMO {
					uint8_t _data[16]; // this was read normally
					float weight; // this is what we read
				};

				if (size < sizeof(AMMO)) return true; // shouldn't really happen with valid data, but oh well

				RE::FormID formID = file->currentform.formID;
				RE::TESForm::AddCompileIndex(formID, file);
				auto entry = (AMMO *) buffer;
				ammoWeights[formID] = entry->weight;
				logger::debug("Found ammo weight in {}: {:08X} -> {}", file->fileName, formID, entry->weight);
				return true;
			}

			static inline REL::Relocation<decltype(thunk)> original;

			static void Install() {
				logger::debug("Installing TESFile::ReadData call hook in TESForm::ReadDataChunk"); // the ReadDataChunk isn't in commonlib, but we need some name :)
				REL::Relocation<uintptr_t> callOffset{REL::Offset(0x1A2120)};
				original = callOffset.write_call<5>(thunk);
			}
		};

		// SkyrimSE.exe+1f37b0
		struct TESForm_GetWeight {
			static float thunk(RE::TESForm *form) {
				float weight = original(form);

				if (!form) return weight;

				// lockpicks have zero weight in VR esm, but 0.1 in AE esm, AE engine only applies the weight in survival mode
				if (form->IsLockpick() && !DOBJ::IsSurvivalModeActive()) {
					return -1.0f;
				}

				// ammo has weight in survival mode
				if (form->IsAmmo()) {
					if (!DOBJ::IsSurvivalModeActive())
						return -1.0f;
					auto it = ammoWeights.find(form->GetFormID());
					return it != ammoWeights.end() ? it->second : 0.0f;
				}

				return weight;
			}

			static inline REL::Relocation<decltype(thunk)> original;

			static void Install() {
				logger::debug("Installing TESForm::GetWeight hook");
				REL::Relocation<uintptr_t> func{REL::Offset(0x1B1470)};
				uintptr_t address = func.address();

				// original starts with a "movss xmm0, dword ptr [rip+disp32]", it's a simple "result = -1.0f" assignment (-1.0f is a shared constant)
				constexpr uint8_t firstInstruction[4] = {0xF3, 0x0F, 0x10, 0x05}; // first four bytes define the instruction,
				constexpr uint32_t firstInstructionSize = 8;                      // next 4 bytes is the disp32 operand, so 8 in total

				auto code = (uint8_t *) address;
				if (memcmp(code, firstInstruction, 4) != 0) {
					SKSE::stl::report_and_fail("TESForm::GetWeight doesn't start with the expected `movss xmm0, dword ptr [rip+disp32]` instruction.\nAnother mod has hooked the function?");
				}

				struct Stub : Xbyak::CodeGenerator {
					explicit Stub(uintptr_t returnAddress) {
						Xbyak::Label minusOne, target;
						movss(xmm0, ptr[rip + minusOne]); // same instruction as the original, but pointing into the stub's own -1.0
						jmp(ptr[rip + target]); // jmp to the address below
						L(minusOne); dd(0xBF800000); // -1.0f
						L(target); dq(returnAddress); // next instruction in the original
					}
				};
				Stub stub(address + firstInstructionSize);
				if (stub.getSize() != kGetWeightStubSize) { // 8 movss + 6 jmp + 4 const + 8 addr
					// this won't happen, lol
					SKSE::stl::report_and_fail(std::format("TESForm::GetWeight stub is {} bytes, expected {}.", stub.getSize(), kGetWeightStubSize));
				}

				original = (uintptr_t) SKSE::GetTrampoline().allocate(stub);
				SKSE::GetTrampoline().write_branch<5>(address, thunk, true);
			}
		};
	}

	void InstallHooks() {
		logger::info("Installing item weight hooks");
		TESForm_ReadDataChunk_ReadData_Call::Install();
		TESForm_GetWeight::Install();
		logger::info("Item weight hooks installed");
	}
}

#include "description.h"

#include "dobj.h"

namespace Description {
	namespace {
		// SkyrimSE.exe+949980 (equivalent, modifying BSString in-place, because the filtered string is always shorter)
		void FilterDescriptionTags(RE::BSString *text) {
			char *read = strchr(text->data(), '[');
			if (!read) return;

			char *write = read;
			while (*read) {
				if (*read != '[') {
					*write++ = *read++;
					continue;
				}

				char *close = strchr(read, ']');
				if (!close) {
					*write++ = *read++;
					continue;
				}

				char *equals = (char *) memchr(read + 1, '=', close - read - 1);
				if (equals && (equals - read == 5) && _strnicmp(read + 1, "SURV", 4) == 0 && DOBJ::IsSurvivalModeActive()) {
					size_t valueLength = close - equals - 1;
					memmove(write, equals + 1, valueLength);
					write += valueLength;
				}
				read = close + 1;
			}
			*write = 0;

			size_t length = write - text->data();

			struct BSStringLayout {
				char *data;
				uint16_t size;
			};
			((BSStringLayout *) text)->size = length < 0xFFFF ? (uint16_t) length : 0xFFFF;
		}

		uintptr_t WritePrologueDetour(uintptr_t address, const uint8_t *prologue, size_t prologueSize, uintptr_t thunk, const char *name) {
			if (memcmp((void *) address, prologue, prologueSize) != 0) {
				SKSE::stl::report_and_fail(std::format("{} doesn't start with the expected instructions.\nAnother mod has hooked the function?", name));
			}

			struct Stub : Xbyak::CodeGenerator {
				Stub(const uint8_t *prologue, size_t prologueSize, uintptr_t returnAddress) {
					db(prologue, prologueSize); // copy whatever was in the original prologue
					mov(rax, returnAddress); // setting rax should be fine as it's a scratch and nothing in our prologues moves to it
					jmp(rax);
				}
			};
			Stub stub(prologue, prologueSize, address + prologueSize);
			if (stub.getSize() != prologueSize + kStubReturnJumpSize) {
				// this won't happen, lol
				SKSE::stl::report_and_fail(std::format("{} prologue stub is {} bytes, expected {}.", name, stub.getSize(), prologueSize + kStubReturnJumpSize));
			}
			void *original = SKSE::GetTrampoline().allocate(stub);
			SKSE::GetTrampoline().write_branch<5>(address, thunk, true);
			return (uintptr_t) original;
		}

		// SkyrimSE.exe+1e1530 (this is the one used by ItemCard, don't confuse with commonlib's TESDescription::GetDescription overload)
		struct TESDescription_GetDescription {
			static void thunk(RE::TESDescription *description, RE::BSString *out, char *openTag, char *closeTag, RE::TESForm *parent, uint32_t fieldType) {
				original(description, out, openTag, closeTag, parent, fieldType);
				FilterDescriptionTags(out);
			}

			static inline REL::Relocation<decltype(thunk)> original;

			static void Install() {
				logger::debug("Installing TESDescription::GetDescription (item card overload) hook");
				// first two instructions (push rdi / sub rsp, 60h) give us 6 bytes that we can replace with a jump, no rip-relatives, so just copy them
				constexpr uint8_t prologue[kGetDescriptionPrologueSize] = {0x40, 0x57, 0x48, 0x83, 0xEC, 0x60};
				REL::Relocation<uintptr_t> function{REL::Offset(0x1A0220)};
				original = WritePrologueDetour(function.address(), prologue, sizeof(prologue), (uintptr_t) thunk, "TESDescription::GetDescription (item card overload)");
			}
		};

		// SkyrimSE.exe+9492e0 (another one used by ItemCard, don't confuse with commonlib's MagicSystem::GetMagicItemDescription)
		struct ItemCard_GetMagicItemDescription {
			static bool thunk(void *unused, RE::MagicItem *item, RE::BSString *out) {
				bool result = original(unused, item, out);
				FilterDescriptionTags(out);
				return result;
			}

			static inline REL::Relocation<decltype(thunk)> original;

			static void Install() {
				logger::debug("Installing ItemCard::GetMagicItemDescription hook");
				// first three instructions (push rbp / push rdi / push r12) gives us 5 bytes that we can replace with a jump, no rip relatives, so just copy them
				constexpr uint8_t prologue[kGetMagicItemDescriptionPrologueSize] = {0x40, 0x55, 0x57, 0x41, 0x54};
				REL::Relocation<uintptr_t> function{REL::Offset(0x8C0940)};
				original = WritePrologueDetour(function.address(), prologue, sizeof(prologue), (uintptr_t) thunk, "ItemCard::GetMagicItemDescription");
			}
		};
	}

	void InstallHooks() {
		logger::info("Installing description hooks");
		TESDescription_GetDescription::Install();
		ItemCard_GetMagicItemDescription::Install();
		logger::info("Description hooks installed");
	}
}

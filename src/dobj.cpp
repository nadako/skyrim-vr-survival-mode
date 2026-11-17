#include "dobj.h"
#include "utils.h"

namespace DOBJ {
	namespace {
		RE::FormID defaultObjectFormIDs[kDOBJCount];
		RE::TESForm *defaultObjectForms[kDOBJCount];

		struct DefaultObjectDef {
			uint32_t key;
			RE::FormType formType;
			const char *name;
		};

		constexpr DefaultObjectDef defaultObjectDefs[] = {
			// names come from xEdit (displayed there as "Use")
			{Sig2Int("SRVT"), RE::FormType::Global, "Survival Mode - Toggle"},
			{Sig2Int("SRVE"), RE::FormType::Global, "Survival Mode - Enabled"},
			{Sig2Int("SRVS"), RE::FormType::Global, "Survival Mode - Show Option"},

			{Sig2Int("SRTP"), RE::FormType::Global, "Survival - Temperature"},

			{Sig2Int("SRCP"), RE::FormType::Global, "Survival - Cold Penalty"},
			{Sig2Int("SRHP"), RE::FormType::Global, "Survival - Hunger Penalty"},
			{Sig2Int("SRSP"), RE::FormType::Global, "Survival - Sleep Penalty"},

			{Sig2Int("SKCD"), RE::FormType::Keyword, "Survival - Keyword Cold"},
			{Sig2Int("SKWM"), RE::FormType::Keyword, "Survival - Keyword Warm"},

			{Sig2Int("SKAH"), RE::FormType::Keyword, "Survival - Keyword Armor Hands"},
			{Sig2Int("SKCH"), RE::FormType::Keyword, "Survival - Keyword Clothing Hands"},
			{Sig2Int("SKAF"), RE::FormType::Keyword, "Survival - Keyword Armor Feet"},
			{Sig2Int("SKCF"), RE::FormType::Keyword, "Survival - Keyword Clothing Feet"},
			{Sig2Int("SKAB"), RE::FormType::Keyword, "Survival - Keyword Armor Body"},
			{Sig2Int("SKCB"), RE::FormType::Keyword, "Survival - Keyword Clothing Body"},
			{Sig2Int("SKAO"), RE::FormType::Keyword, "Survival - Keyword Armor Head"},
			{Sig2Int("SKCO"), RE::FormType::Keyword, "Survival - Keyword Clothing Head"},

			{Sig2Int("JWLR"), RE::FormType::Keyword, "Keyword - Jewelry"},
		};
		static_assert(std::size(defaultObjectDefs) == kDOBJCount);

		struct BGSDefaultObjectManager_Load_ReadData_Call {
			static bool thunk(RE::TESFile *file, void *buffer, uint32_t size) {
				if (!original(file, buffer, size)) return false;

				struct Entry {
					uint32_t key;
					RE::FormID formID;
				};
				static_assert(sizeof(Entry) == 8);

				auto entries = (Entry *) buffer;
				uint32_t entryCount = size / sizeof(Entry);
				for (uint32_t entryIndex = 0; entryIndex < entryCount; entryIndex++) {
					auto &entry = entries[entryIndex];
					for (uint32_t dobjIndex = 0; dobjIndex < kDOBJCount; dobjIndex++) {
						auto &def = defaultObjectDefs[dobjIndex];
						if (entry.key != def.key) continue;

						RE::FormID formID = entry.formID;
						if (formID != 0) {
							// SkyrimVRESL hooks this for ESL support
							RE::TESForm::AddCompileIndex(formID, file);
						}
						defaultObjectFormIDs[dobjIndex] = formID;
						logger::debug("Found default object in {}: '{}' -> {:08X}", file->fileName, def.name, formID);
						break;
					}
				}

				return true;
			}

			static inline REL::Relocation<decltype(thunk)> original;

			static void Install() {
				logger::debug("Installing TESFile::ReadData call hook in BGSDefaultObjectManager::Load");
				REL::Relocation<uintptr_t> callOffset{REL::Offset(0x2D209F)};
				original = callOffset.write_call<5>(thunk); // direct call
			}
		};
	}

	void InstallHooks() {
		logger::info("Installing default object hooks");
		BGSDefaultObjectManager_Load_ReadData_Call::Install();
		logger::info("Default object hooks installed");
	}

	void ResolveForms() {
		logger::info("Resolving forms");
		std::string errors;
		for (uint32_t dobjIndex = 0; dobjIndex < kDOBJCount; dobjIndex++) {
			auto &def = defaultObjectDefs[dobjIndex];
			RE::FormID formID = defaultObjectFormIDs[dobjIndex];

			if (formID == 0) {
				errors += std::format("{}: FormID not found in game files\n", def.name);
				continue;
			}

			auto form = RE::TESForm::LookupByID(formID);
			if (!form) {
				errors += std::format("{}: Form {:08X} not found\n", def.name, formID);
				continue;
			}

			if (form->GetFormType() != def.formType) {
				errors += std::format("{}: Form {:08X} has unexpected type {} (should be {})\n", def.name, formID, form->GetFormType(), def.formType);
				continue;
			}

			defaultObjectForms[dobjIndex] = form;
			logger::debug("Resolved default object '{}': {:08X} {}", def.name, formID, form->GetFormEditorID());
		}
		if (errors.empty()) {
			logger::info("Forms resolved");
		} else {
			SKSE::stl::report_and_fail(std::format("Could not resolve Survival Mode forms:\n\n{}", errors));
		}
	}

	RE::TESGlobal *GetGlobal(DefaultObject object) {
		return (RE::TESGlobal *) defaultObjectForms[object];
	}

	RE::BGSKeyword *GetKeyword(DefaultObject object) {
		return (RE::BGSKeyword *) defaultObjectForms[object];
	}

	// SkyrimSE.exe+9924d0
	bool IsSurvivalModeActive() {
		RE::TESGlobal *toggle = GetGlobal(kDOBJModeToggle);
		return toggle && toggle->value == 1.0f;
	}
}

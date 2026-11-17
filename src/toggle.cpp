#include "toggle.h"
#include "dobj.h"
#include "utils.h"

namespace Toggle {
	namespace {
		constexpr double kSurvivalModeOptionID = Sig2Int("SRVT"); // ID doesn't have to be this, but it's a random huge number that definitely won't clash with other option ids

		RE::FxDelegateHandler::CallbackFn *originalRequestGameplayOptions = nullptr;
		RE::FxDelegateHandler::CallbackFn *originalOptionChange = nullptr;

		// SkyrimSE.exe+9a8f30
		void RequestGameplayOptions(const RE::FxDelegateArgs &args) {
			originalRequestGameplayOptions(args);

			auto showOption = DOBJ::GetGlobal(DOBJ::kDOBJModeShowOption);
			if (!showOption || showOption->value != 1.0f) return;
			if (args.GetArgCount() < 1 || !args[0].IsArray()) return;
			RE::GFxValue entries = args[0];

			RE::GFxValue entry;
			args.GetMovie()->CreateObject(&entry);
			entry.SetMember("text", "$Survival Mode");
			entry.SetMember("movieType", 2); // 2 is checkbox
			entry.SetMember("ID", kSurvivalModeOptionID);
			entry.SetMember("value", DOBJ::IsSurvivalModeActive() ? 1.0 : 0.0);
			entry.SetMember("defaultVal", 0.0);
			entries.PushBack(entry);
		}

		// SkyrimSE.exe+9ab260
		void OptionChange(const RE::FxDelegateArgs &args) {
			bool isSurvivalOption = args.GetArgCount() >= 2 && args[0].IsNumber() && args[0].GetNumber() == kSurvivalModeOptionID;
			if (!isSurvivalOption) {
				originalOptionChange(args);
				return;
			}

			auto toggle = DOBJ::GetGlobal(DOBJ::kDOBJModeToggle);
			if (toggle && args[1].IsNumber()) {
				toggle->value = (float) args[1].GetNumber();
			}
		}

		struct CallbackProcessorProxy : RE::FxDelegateHandler::CallbackProcessor {
			RE::FxDelegateHandler::CallbackProcessor *original;

			explicit CallbackProcessorProxy(RE::FxDelegateHandler::CallbackProcessor *target) : original(target) {
			}

			void Process(const RE::GString &methodName, RE::FxDelegateHandler::CallbackFn *method) override {
				if (methodName == "RequestGameplayOptions") {
					originalRequestGameplayOptions = method;
					method = RequestGameplayOptions;
				} else if (methodName == "OptionChange") {
					originalOptionChange = method;
					method = OptionChange;
				}
				original->Process(methodName, method);
			}
		};

		struct Journal_SystemTab_Accept {
			static void thunk(RE::Journal_SystemTab *tab, RE::FxDelegateHandler::CallbackProcessor *processor) {
				CallbackProcessorProxy proxy(processor);
				original(tab, &proxy);
			}

			static inline REL::Relocation<decltype(thunk)> original;

			static void Install() {
				logger::debug("Installing Journal_SystemTab::Accept hook");
				REL::Relocation<uintptr_t> vtable{RE::Journal_SystemTab::VTABLE[0]};
				original = vtable.write_vfunc(1, thunk);
			}
		};
	}

	void InstallHooks() {
		logger::info("Installing survival mode toggle hooks");
		Journal_SystemTab_Accept::Install();
		logger::info("Survival mode toggle hooks installed");
	}
}

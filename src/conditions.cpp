#include "conditions.h"
#include "warmth.h"

namespace Conditions {
	namespace {
		RE::SCRIPT_FUNCTION *GetCommand(RE::FUNCTION_DATA::FunctionID id, const char *expectedName) {
			auto command = &RE::SCRIPT_FUNCTION::GetFirstScriptCommand()[(uint16_t) id];
			if (!command->functionName || strcmp(command->functionName, expectedName) != 0) {
				SKSE::stl::report_and_fail(std::format(
					"Unexpected script command at condition function index {}: '{}' (expected '{}')",
					(int) id, command->functionName ? command->functionName : "", expectedName));
			}
			if (command->conditionFunction) {
				SKSE::stl::report_and_fail(std::format(
					"Script command {} already has conditionFunction set: '{}'", (int) id, command->functionName));
			}
			return command;
		}

		// SkyrimSE.exe+332ff0
		bool IsOverEncumbered(RE::TESObjectREFR *subject, void *, void *, double &result) {
			if (!subject) return false;
			auto actor = subject->As<RE::Actor>();
			result = actor && actor->IsOverEncumbered() ? 1.0 : 0.0;
			return true;
		}

		// SkyrimSE.exe+333110
		bool GetActorWarmth(RE::TESObjectREFR *subject, void *, void *, double &result) {
			if (!subject) return false;
			auto actor = subject->As<RE::Actor>();
			result = actor ? Warmth::GetActorWarmthRating(actor) : 0.0;
			return true;
		}
	}

	void Install() {
		logger::info("Installing condition functions");

		// functions are in a fixed array, first script and condition functions, then the sentinel with this name, then the console commands.
		// AE and VR differ here (AE has 3 more functions: IsOverEncumbered, GetActorWarmth, and... KillAllHostile?), but we can't really
		// insert them in VR, because that would mess up the positions of console commands. thankfully though, we can safely set the
		// conditionFunction into the records of the sentinel (IsOverEncumbered on AE) and the Show console command (GetActorWarmth), since
		// only TESConditionItem::IsTrue reads conditionFunction, and it just calls it if it's not null
		auto isOverEncumbered = GetCommand(RE::FUNCTION_DATA::FunctionID::kIsOverEncumbered, "ADD NEW FUNCTIONS BEFORE THIS ONE!!!");
		isOverEncumbered->conditionFunction = IsOverEncumbered;

		auto getActorWarmth = GetCommand(RE::FUNCTION_DATA::FunctionID::kGetActorWarmth, "Show");
		getActorWarmth->conditionFunction = GetActorWarmth;

		logger::info("Condition functions installed");
	}
}

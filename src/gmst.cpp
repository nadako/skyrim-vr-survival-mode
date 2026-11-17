#include "gmst.h"

namespace GMST {
	namespace {
		RE::Setting *slotBonuses[kRowCount][kSlotCount];
		RE::Setting *fSurvArmorScalar;
		RE::Setting *fSurvTorchBonus;

		RE::Setting *CreateGameSetting(const char *name, float defaultValue) {
			auto collection = RE::GameSettingCollection::GetSingleton();
			if (auto existing = collection->GetSetting(name)) {
				logger::warn("Game setting {} already exists (value {}), reusing it", name, existing->GetFloat());
				return existing;
			}

			logger::debug("Creating setting {} with value {}", name, defaultValue);
			auto *setting = new RE::Setting(name, defaultValue);
			collection->InsertSetting(setting);
			return setting;
		}
	}

	float GetSlotBonus(Row row, Slot slot) {
		return slotBonuses[row][slot]->GetFloat();
	}

	float GetArmorScalar() {
		return fSurvArmorScalar->GetFloat();
	}

	float GetTorchBonus() {
		return fSurvTorchBonus->GetFloat();
	}

	void RegisterGameSettings() {
		logger::info("Registering game settings");

		// RE'd from SkyrimSE.exe, default values consistent with https://en.uesp.net/wiki/Skyrim:Cold#Warmth
		struct Pair {
			const char *name;
			float defaultValue;
		};
		Pair settings[kRowCount][kSlotCount] = {
			//@formatter:off
			{{"fSurvNormalBodyBonus", 27}, {"fSurvNormalHeadBonus", 18}, {"fSurvNormalHandsBonus", 13}, {"fSurvNormalFeetBonus", 13}},
			{{"fSurvWarmBodyBonus",   54}, {"fSurvWarmHeadBonus",   29}, {"fSurvWarmHandsBonus",   24}, {"fSurvWarmFeetBonus",   24}},
			{{"fSurvColdBodyBonus",   17}, {"fSurvColdHeadBonus",   8},  {"fSurvColdHandsBonus",   7},  {"fSurvColdFeetBonus",   7}},
			//@formatter:on
		};
		for (uint32_t row = 0; row < kRowCount; ++row) {
			for (uint32_t slot = 0; slot < kSlotCount; ++slot) {
				slotBonuses[row][slot] = CreateGameSetting(settings[row][slot].name, settings[row][slot].defaultValue);
			}
		}
		fSurvArmorScalar = CreateGameSetting("fSurvArmorScalar", 1);
		fSurvTorchBonus = CreateGameSetting("fSurvTorchBonus", 50);

		logger::info("Game settings registered");
	}
}

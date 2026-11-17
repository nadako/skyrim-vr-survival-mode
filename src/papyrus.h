#pragma once

namespace Papyrus {
	constexpr size_t trampolineSize = kTrampolineAddressSize;

	void RegisterNatives();

	void InstallHooks();

	template<class T>
	void SendAndRelayEvent(RE::TESObjectREFR *ref, RE::BSFixedString &eventName, T value) {
		auto vm = RE::SkyrimVM::GetSingleton();
		auto handlePolicy = static_cast<RE::BSScript::IObjectHandlePolicy *>(&vm->GetVMRuntimeData().handlePolicy);
		auto handle = handlePolicy->GetHandleForObject(ref->GetFormType(), ref);
		RE::FunctionArguments<T> args(std::move(value));
		vm->SendAndRelayEvent(handle, &eventName, &args, nullptr);
	}
}

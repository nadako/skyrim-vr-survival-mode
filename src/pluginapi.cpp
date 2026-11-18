#include "survivalmodevr_api.h"
#include "fasttravel.h"
#include "warmth.h"

namespace {
	SurvivalModeVR::API1 api1{
		.GetActorWarmthRating = &Warmth::GetActorWarmthRating,
		.GetFastTravelEndEventSource = &FastTravel::GetEndEventSource,
	};
}

extern "C" __declspec(dllexport) void *GetPluginAPI(SurvivalModeVR::APIVersion apiVersion) {
	switch (apiVersion) {
		case SurvivalModeVR::API_V1: {
			logger::debug("Returning requested API version {}", (int) apiVersion);
			return &api1;
		}
		default: {
			logger::warn("Unsupported requested API version {}", (int) apiVersion);
			return nullptr;
		}
	}
}

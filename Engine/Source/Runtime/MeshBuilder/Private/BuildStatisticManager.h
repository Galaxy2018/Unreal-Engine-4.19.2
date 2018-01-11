// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMeshDescriptionBuildStatistic, Log, All);

namespace BuildStatisticManager
{
	struct FStatisticData
	{
	public:
		FStatisticData()
			: TotalTime(0.0)
			, Counter(0)
		{ }
		double TotalTime;
		uint32 Counter;
	};

	class FBuildStatisticScope
	{
	public:
		FBuildStatisticScope(const FString& InTimerDescription, FStatisticData& InStatisticData)
			: StatisticData(InStatisticData)
			, TimerDescription(InTimerDescription)
		{
			StartTime = FPlatformTime::Cycles();
		}
		~FBuildStatisticScope()
		{
			double ScopeTime = FPlatformTime::ToSeconds64(FPlatformTime::Cycles() - StartTime);
			StatisticData.TotalTime += ScopeTime;
			StatisticData.Counter += 1;
			UE_LOG(LogMeshDescriptionBuildStatistic, Log, TEXT("%s: %f seconds - Counter: %d - Total Time: %f"), *TimerDescription, ScopeTime, StatisticData.Counter, StatisticData.TotalTime);
		}
	private:
		int32 StartTime;
		FString TimerDescription;
		FStatisticData& StatisticData;
	};
}
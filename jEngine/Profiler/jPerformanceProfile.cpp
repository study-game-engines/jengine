﻿#include "pch.h"
#include "jPerformanceProfile.h"

jPerformanceProfile* jPerformanceProfile::_instance = nullptr;

jMutexRWLock ScopedCPULock;
jMutexRWLock ScopedGPULock;
robin_hood::unordered_map<jPriorityName, jScopedProfileData, jPriorityNameHashFunc> ScopedProfileCPUMap[MaxProfileFrame];
robin_hood::unordered_map<jPriorityName, jScopedProfileData, jPriorityNameHashFunc> ScopedProfileGPUMap[MaxProfileFrame];
static int32 PerformanceFrame = 0;

robin_hood::unordered_set<jQuery*> jQueryTimePool::s_running;
robin_hood::unordered_set<jQuery*> jQueryTimePool::s_resting;

std::atomic<int32> jScopedProfile_CPU::s_priority = 0;
std::atomic<int32> jScopedProfile_GPU::s_priority = 0;

thread_local std::atomic<int32> ScopedProfilerCPUIndent;
thread_local std::atomic<int32> ScopedProfilerGPUIndent;

std::vector<jProfile_GPU> jProfile_GPU::WatingResultList[jRHI::MaxWaitingQuerySet];
int32 jProfile_GPU::CurrentWatingResultListIndex = 0;

int32 NextFrame()
{
	jScopedProfile_CPU::ResetPriority();
	jScopedProfile_GPU::ResetPriority();

	PerformanceFrame = (PerformanceFrame + 1) % MaxProfileFrame;

	// 이번에 수집할 프로파일링 데이터 초기화
	{
		jScopeWriteLock s(&ScopedCPULock);
		ScopedProfileCPUMap[PerformanceFrame].clear();
	}
	{
		jScopeWriteLock s(&ScopedGPULock);
		ScopedProfileGPUMap[PerformanceFrame].clear();
	}
	return PerformanceFrame;
}

void ClearScopedProfileCPU()
{
	jScopeWriteLock s(&ScopedCPULock);

	for (int32 i = 0; i < MaxProfileFrame; ++i)
		ScopedProfileCPUMap[i].clear();
}

void AddScopedProfileCPU(const jPriorityName& name, uint64 elapsedTick, int32 Indent)
{
	jScopeWriteLock s(&ScopedCPULock);
	ScopedProfileCPUMap[PerformanceFrame][name] = jScopedProfileData(elapsedTick, Indent, std::this_thread::get_id());
}

void ClearScopedProfileGPU()
{
	jScopeWriteLock s(&ScopedGPULock);
	for (int32 i = 0; i < MaxProfileFrame; ++i)
		ScopedProfileGPUMap[i].clear();
}

void AddScopedProfileGPU(const jPriorityName& name, uint64 elapsedTick, int32 Indent)
{
	jScopeWriteLock s(&ScopedGPULock);
	ScopedProfileGPUMap[PerformanceFrame][name] = jScopedProfileData(elapsedTick, Indent, std::this_thread::get_id());
}

void jPerformanceProfile::Update(float deltaTime)
{
	jProfile_GPU::ProcessWaitings();

	CalcAvg();
	NextFrame();

    //if (TRUE_PER_MS(1000))
    //    PrintOutputDebugString();
}

void jPerformanceProfile::CalcAvg()
{
	{
        robin_hood::unordered_map<jPriorityName, jAvgProfile, jPriorityNameHashFunc> SumOfScopedProfileCPUMap;
		{
			jScopeReadLock s(&ScopedCPULock);
			for (int32 i = 0; i < MaxProfileFrame; ++i)
			{
				const auto& scopedProfileMap = ScopedProfileCPUMap[i];
				for (auto& iter : scopedProfileMap)
				{
					auto& avgProfile = SumOfScopedProfileCPUMap[iter.first];
					avgProfile.TotalElapsedTick += iter.second.ElapsedTick;
					avgProfile.Indent = iter.second.Indent;
					avgProfile.ThreadId = iter.second.ThreadId;
					++avgProfile.TotalSampleCount;
				}
			}
		}

		CPUAvgProfileMap.clear();
		for (auto& iter : SumOfScopedProfileCPUMap)
		{
			JASSERT(iter.second.TotalSampleCount > 0);
			iter.second.AvgElapsedMS = (iter.second.TotalElapsedTick / static_cast<double>(iter.second.TotalSampleCount)) * 0.000001;	// ns -> ms
			CPUAvgProfileMap[iter.first] = iter.second;
		}
	}

	{
		robin_hood::unordered_map<jPriorityName, jAvgProfile, jPriorityNameHashFunc> SumOfScopedProfileGPUMap;
		{
			jScopeReadLock s(&ScopedGPULock);
			for (int32 i = 0; i < MaxProfileFrame; ++i)
			{
				const auto& scopedProfileMap = ScopedProfileGPUMap[i];
				for (auto& iter : scopedProfileMap)
				{
					auto& avgProfile = SumOfScopedProfileGPUMap[iter.first];
					avgProfile.TotalElapsedTick += iter.second.ElapsedTick;
					avgProfile.Indent = iter.second.Indent;
					avgProfile.ThreadId = iter.second.ThreadId;
					++avgProfile.TotalSampleCount;
				}
			}
		}

        GPUAvgProfileMap.clear();
		for (auto& iter : SumOfScopedProfileGPUMap)
		{
			JASSERT(iter.second.TotalSampleCount > 0);
			iter.second.AvgElapsedMS = (iter.second.TotalElapsedTick / static_cast<double>(iter.second.TotalSampleCount)) * 0.000001;	// ns -> ms
			GPUAvgProfileMap[iter.first] = iter.second;
		}
	}
}

void jPerformanceProfile::PrintOutputDebugString()
{
	std::string result;
	char szTemp[128] = { 0, };
	if (!CPUAvgProfileMap.empty())
	{
		result += "-----CPU---PerformanceProfile----------\n";
		for (auto& iter : CPUAvgProfileMap)
		{
			sprintf_s(szTemp, sizeof(szTemp), "%s : \t\t\t\t%lf ms", iter.first.ToStr(), iter.second.AvgElapsedMS);
			result += szTemp;
			result += "\n";
		}
	}

	if (!GPUAvgProfileMap.empty())
	{
		result += "-----GPU---PerformanceProfile----------\n";
		for (auto& iter : GPUAvgProfileMap)
		{
			sprintf_s(szTemp, sizeof(szTemp), "%s : \t\t\t\t%lf ms", iter.first.ToStr(), iter.second.AvgElapsedMS);
			result += szTemp;
			result += "\n";
		}
	}
	OutputDebugStringA(result.c_str());
}

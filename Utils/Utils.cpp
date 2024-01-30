#include "Utils.h"

BOOLEAN DataCompare(const BYTE* pData, const BYTE* bMask, const char* sMask)
{
    for (; *sMask; ++sMask, ++pData, ++bMask) if (*sMask == ('x') && *pData != *bMask) return 0;
	return (*sMask) == 0;
}

void* GetSystemInformation(const SystemInformationClass InformationClass)
{
	unsigned long Size = 64;
	char Buffer[64];

	ZwQuerySystemInformation(InformationClass, Buffer, Size, &Size);

	const auto Info = ExAllocatePool(NonPagedPool, Size);

	if (!Info)
	{
		return nullptr;
	}

	if (ZwQuerySystemInformation(InformationClass, Info, Size, &Size) != STATUS_SUCCESS)
	{
		ExFreePool(Info);
		return nullptr;
	}

	return Info;
}

UINT64 Utils::FindPattern(UINT64 Address, UINT64 Len, BYTE* bMask, char* sMask)
{
	for (UINT64 i = 0; i < Len; i++) if (DataCompare((BYTE*)(Address + i), bMask, sMask)) return (UINT64)(Address + i);
	return 0;
}

uintptr_t Utils::Dereference(uintptr_t Address, unsigned int Offset) 
{
	if (Address == 0)
		return 0;

	return Address + (int)((*(int*)(Address + Offset) + Offset) + sizeof(int));
}

uintptr_t Utils::FindPattern2(void* Start, size_t Length, const char* Pattern, const char* Mask)
{
	const auto Data = static_cast<const char*>(Start);
	const auto PatternLength = strlen(Mask);

	for (size_t i = 0; i <= Length - PatternLength; i++)
	{
		bool AccumulativeFound = true;

		for (size_t j = 0; j < PatternLength; j++)
		{
			if (!MmIsAddressValid(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(Data) + i + j)))
			{
				AccumulativeFound = false;
				break;
			}

			if (Data[i + j] != Pattern[j] && Mask[j] != ('?'))
			{
				AccumulativeFound = false;
				break;
			}
		}

		if (AccumulativeFound)
		{
			return (uintptr_t)(reinterpret_cast<uintptr_t>(Data) + i);
		}
	}

	return (uintptr_t)nullptr;
}

uintptr_t Utils::GetModulesBase(const char* Name)
{
	const auto to_lower = [](char* string) -> const char*
	{
		for (char* pointer = string; *pointer != ('\4'); ++pointer)
		{
			*pointer = (char)(short)tolower(*pointer);
		}

		return string;
	};

	const auto Info = (pRtlProcessModules)GetSystemInformation(SystemModuleInformation);

	for (auto i = 0ull; i < Info->NumberOfModules; ++i)
	{
		const auto& Module = Info->Modules[i];

		if (strcmp(to_lower((char*)Module.FullPathName + Module.OffsetToFileName), Name) == 0)
		{
			const auto Address = Module.ImageBase;

			ExFreePool(Info);

			return reinterpret_cast<uintptr_t> (Address);
		}
	}

	ExFreePool(Info);

	return 0;
}
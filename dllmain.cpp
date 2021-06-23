#include <iostream>
#include <string>
#include <Windows.h>
#include <Psapi.h>

//Love Kronos
namespace Pattern
{
#define INRANGE(x,a,b)      (x >= a && x <= b) 
#define getBits(x)          (INRANGE(x,'0','9') ? (x - '0') : ((x&(~0x20)) - 'A' + 0xa))
#define getByte(x)          (getBits(x[0]) << 4 | getBits(x[1]))

	static PBYTE BaseAddress = NULL;
	static MODULEINFO ModuleInfo;

	static PBYTE Find(const char* pattern, PBYTE rangeStart = NULL, PBYTE rangeEnd = NULL)
	{
		if (BaseAddress == NULL)
		{
			BaseAddress = (PBYTE)GetModuleHandle(NULL);
			GetModuleInformation(GetCurrentProcess(), (HMODULE)BaseAddress, &ModuleInfo, sizeof(ModuleInfo));
		}

		if (rangeStart == NULL) rangeStart = BaseAddress;
		if (rangeEnd == NULL) rangeEnd = rangeStart + ModuleInfo.SizeOfImage;

		const unsigned char* pat = reinterpret_cast<const unsigned char*>(pattern);
		PBYTE firstMatch = 0;
		for (PBYTE pCur = rangeStart; pCur < rangeEnd; ++pCur)
		{
			if (*(PBYTE)pat == (BYTE)'\?' || *pCur == getByte(pat))
			{
				if (!firstMatch) firstMatch = pCur;
				pat += (*(PWORD)pat == (WORD)'\?\?' || *(PBYTE)pat != (BYTE)'\?') ? 2 : 1;
				if (!*pat) return firstMatch;
				pat++;
				if (!*pat) return firstMatch;
			}
			else if (firstMatch)
			{
				pCur = firstMatch;
				pat = reinterpret_cast<const unsigned char*>(pattern);
				firstMatch = 0;
			}
		}
		return NULL;
	}
}

// Default Structures
template < class T > struct TArray
{
	T*              Data;
	DWORD   Num;
	DWORD   Max;
};
struct FName
{
	int32_t ComparisonIndex;
	int32_t Number;
};
struct UObject
{
public:
	char UnknownData_00[8];
	int32_t ObjectFlags;
	int32_t InternalIndex;
	char UnknownData_01[8];
	FName Name;
	UObject* Outer;
};

class FUObjectItem
{
public:
	UObject * Object;
	__int32 Flags;
	__int32 ClusterIndex;
	__int32 SerialNumber;
};

class TUObjectArray
{
public:
	FUObjectItem* Objects;
	__int32 MaxElements;
	__int32 NumElements;

	FUObjectItem& GetByIndex(int Index)
	{
		return this->Objects[Index];
	}
};

class FUObjectArray
{
public:
	int ObjFirstGCIndex;
	int ObjLastNonGCIndex;
	int MaxObjectsNotConsideredByGC;
	bool OpenForDisregardForGC;
	TUObjectArray ObjObjects;
};
FUObjectArray* GObjObjects;

//Changed for SoD2
struct FNameEntry
{
public:
	int64_t UnknownData_00;
	char AnsiName[1024];
};


//Changed for SoD2
//The class probably isnt layed out like this but I don't care it works lmao
DWORD64 NameArrayFuncAddy;
class TNameEntryArray
{
public:
	static FNameEntry* GetById(int Index)
	{
		return reinterpret_cast<FNameEntry* (*)(__int64, int)>(NameArrayFuncAddy)(0, Index);
	}
};

char* GetName(UObject* Object)
{
	if (Object)
	{
		if (Object->Name.ComparisonIndex < 0 || !TNameEntryArray::GetById(Object->Name.ComparisonIndex))
		{
			static char ret[256];
			sprintf_s(ret, "INVALID NAME INDEX : %i", Object->Name.ComparisonIndex);
			return ret;
		}
		else
		{
			return (char*)TNameEntryArray::GetById(Object->Name.ComparisonIndex)->AnsiName;
		}
	}
	else
	{
		return (char*)"Package";
	}
}

void ObjectDump()
{
	FILE* Log = NULL;
	fopen_s(&Log, "ObjectDump.txt", "w+");
	std::cout << "Starting Dump" << std::endl;
	for (DWORD64 i = 0x0; i < GObjObjects->ObjObjects.NumElements; i++)
	{
		if (!GObjObjects->ObjObjects.GetByIndex(i).Object) { continue; }
		
		fprintf(Log, "[%06i] 0x%llX %s.%s\n", i, GObjObjects->ObjObjects.GetByIndex(i).Object, GetName(GObjObjects->ObjObjects.GetByIndex(i).Object->Outer), GetName(GObjObjects->ObjObjects.GetByIndex(i).Object));
	}
	std::cout << "Dump Finished" << std::endl;
	fclose(Log);
}

void Init()
{
	NameArrayFuncAddy = (DWORD64)Pattern::Find("85 D2 78 37 81 FA ? ? ? ? 7D 11 48 63 C2 48 8D 0D ? ? ? ? 8B 14 81 85 D2 74 1E");
	auto GObjectPattern = (DWORD64)Pattern::Find("48 8D 0D ? ? ? ? 48 8B D3 E8 ? ? ? ? 85 FF 74 22");
	GObjObjects = (FUObjectArray*)(GObjectPattern + *(DWORD*)(GObjectPattern + 0x3) + 0x7);
	std::cout << "GObjObjects: " << GObjObjects << std::endl;
	ObjectDump();
}
void onAttach()
{
	AllocConsole();
	ShowWindow(GetConsoleWindow(), SW_SHOW);
	FILE* fp;
	freopen_s(&fp, "CONOIN$", "r", stdin);
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);
	Init();
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)onAttach, NULL, 0, NULL);
		return true;
		break;

	case DLL_PROCESS_DETACH:
		return true;
		break;
	}
}
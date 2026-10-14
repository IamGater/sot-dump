// Runtime helpers: name lookup and object enumeration for code running inside the game.
// Call SoT::Init(moduleBase) once (e.g. GetModuleHandle(nullptr)) before using them.
#pragma once

#include "Basic.hpp"
#include "Offsets.hpp"
#include "Forward.hpp"
#include "Structs.hpp"

namespace SoT
{
	namespace Detail
	{
		inline FNameEntry*** g_NameChunks = nullptr;
		inline FUObjectItem* g_Objects = nullptr;
		inline const int32_t* g_ObjectCount = nullptr;
	}

	inline void Init(uintptr_t moduleBase)
	{
		Detail::g_NameChunks = *reinterpret_cast<FNameEntry****>(moduleBase + Offsets::GNames);
		Detail::g_Objects = *reinterpret_cast<FUObjectItem**>(moduleBase + Offsets::GObjects);
		Detail::g_ObjectCount = reinterpret_cast<const int32_t*>(moduleBase + Offsets::GObjects - 4);
	}

	inline std::string FName::ToString() const
	{
		if (!Detail::g_NameChunks)
			return std::string();
		FNameEntry** chunk = Detail::g_NameChunks[Index >> 14];
		if (!chunk)
			return std::string();
		const FNameEntry* entry = chunk[Index & 0x3FFF];
		if (!entry)
			return std::string();
		std::string s = entry->AnsiName;
		if (Number > 0)
			s += "_" + std::to_string(Number - 1);
		return s;
	}

	inline int32_t UObject::ObjectCount() { return Detail::g_ObjectCount ? *Detail::g_ObjectCount : 0; }

	inline UObject* UObject::GetByIndex(int32_t index)
	{
		return (index >= 0 && index < ObjectCount()) ? Detail::g_Objects[index].Object : nullptr;
	}

	inline std::string UObject::GetName() const { return Name.ToString(); }

	inline std::string PathOf(const UObject* obj)
	{
		std::string path;
		for (const UObject* o = obj; o; o = o->Outer)
			path = o->GetName() + (path.empty() ? "" : "." + path);
		return path;
	}

	inline std::string UObject::GetFullName() const
	{
		return (Class ? Class->GetName() : std::string("?")) + " " + PathOf(this);
	}

	inline bool UObject::IsA(const UClass* cls) const
	{
		for (const UStruct* s = Class; s; s = s->SuperField)
			if (s == cls)
				return true;
		return false;
	}

	inline void UObject::ProcessEvent(UFunction* function, void* params)
	{
		using Fn = void (*)(UObject*, UFunction*, void*);
		reinterpret_cast<Fn>(VTable[Offsets::ProcessEventIndex])(this, function, params);
	}

	inline UObject* UObject::FindObject(const char* fullPath)
	{
		for (int32_t i = 0; i < ObjectCount(); ++i)
		{
			UObject* o = GetByIndex(i);
			if (o && PathOf(o) == fullPath)
				return o;
		}
		return nullptr;
	}
}

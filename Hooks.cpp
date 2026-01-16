#include "Hooks.h"
#include "Defs.h"
#include "SafeWrite.cpp"
#include <GameMenus.h>

namespace RealTimeMenus
{

	static const UInt32 Unk58CallAddr = 0x004CCCA8;
	static const UInt32 Unk58CallReturn = 0x004CCCAA;

	const UInt32 ActorCheckAddr = 0x00674A82;
	const UInt32 ActorCheckSkip = 0x00674DD1;  // Where it jumps if skipping
	const UInt32 ActorCheckContinue = 0x00674A8A;  // Where it continues if processing

	static Actor* GetCurrentDialogueSpeaker()
	{
		if ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Loading)
			|| ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Dialog))
			|| ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Main))
			|| ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Pause))
			|| ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Persuasion))
			|| ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Repair))
			|| ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Negotiate))
			|| ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_SpellPurchase)))
		{
			DialogMenu* dialogMenu = (DialogMenu*)InterfaceManager::GetSingleton()->activeMenu;
			if (dialogMenu)
			{
				Character* speaker = dialogMenu->speaker;
				if (speaker)
				{
					return (Actor*)speaker;
				}
			}
		}
		return nullptr;
	}

	bool __cdecl IsMenuMode_AIUpdate_Hook()
	{
		Actor* currentActor = *(Actor**)0xB3BCF8;
		Actor* dialogueSpeaker = GetCurrentDialogueSpeaker();

		_MESSAGE("IsMenuMode check - Current: %08X, Speaker: %08X", currentActor, dialogueSpeaker);

		if (dialogueSpeaker && currentActor == dialogueSpeaker)
		{
			_MESSAGE("Returning TRUE for dialogue speaker");
			return true;
		}

		return false;
	}

	bool __cdecl IsDialogueSpeakerAI()
	{
		Actor* currentActor = *(Actor**)0xB3BCF8;  // dword_B3BCF8
		if (!currentActor)
			return false;

		Actor* dialogueSpeaker = GetCurrentDialogueSpeaker();
		return (currentActor == dialogueSpeaker);
	}

	__declspec(naked) static void AI_CheckDialogueSpeaker()
	{
		__asm
		{
			// Original instruction
			test    ecx, ecx
			jz      skip_actor      // If null, skip

			// Check if this is the dialogue speaker
			push    eax
			push    edx
			push    ecx
			call    IsDialogueSpeakerAI
			test    al, al
			pop     ecx
			pop     edx
			pop     eax
			jnz     skip_actor      // If dialogue speaker, skip

			// Continue normal processing
			jmp[ActorCheckContinue]

			skip_actor:
			jmp[ActorCheckSkip]
		}
	}

	static bool __cdecl ShouldSkipActorUpdate(TESObjectREFR* refr)
	{
		if (!refr)
			return false;

		Actor* dialogueSpeaker = GetCurrentDialogueSpeaker();

		if (!dialogueSpeaker)
			return false;

		return (refr == dialogueSpeaker);
	}

	__declspec(naked) static void Unk58_CheckHook()
	{
		__asm
		{
			push eax
			push ecx
			push edx

			push esi
			call ShouldSkipActorUpdate
			add esp, 4

			pop edx
			pop ecx

			test al, al
			pop eax
			jnz skip_update

			// Original instructions we overwrote:
			mov eax, [esi]           // mov eax, [esi]
			mov edx, [eax + 160h]      // mov edx, [eax+160h]
			mov ecx, esi             // mov ecx, esi
			call edx                 // call edx

			skip_update :
			jmp[Unk58CallReturn]
		}
	}

	struct AIUpdate
	{
		static void Install()
		{
			// We need to overwrite from 0x674A82 to 0x674A84 (test ecx, ecx = 2 bytes)
			// Plus the jz (6 bytes) = 8 bytes total to fit our 5-byte jump
			WriteRelJump(ActorCheckAddr, (UInt32)&AI_CheckDialogueSpeaker);
			SafeWrite8(ActorCheckAddr + 5, 0x90);  // NOP
			SafeWrite8(ActorCheckAddr + 6, 0x90);  // NOP
			SafeWrite8(ActorCheckAddr + 7, 0x90);  // NOP

			_MESSAGE("Installed AIUpdate hook", ActorCheckAddr);
		}
	};

	struct AnimUpdate
	{
		static void Install()
		{
			WriteRelJump(0x004CCC9E, (UInt32)&Unk58_CheckHook);
			SafeWrite8(0x004CCC9E + 5, 0x90);
			_MESSAGE("Installed AnimUpdate hook");
		}
	};

	struct PlaySoundImpl
	{
		static bool __fastcall PlaySound_Hook(int* a1, void* edx)
		{
			if (!g_thePlayer || !(*g_thePlayer) || ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Loading)))
			{
				return true;
			}

			return false;
		}

		static void Install()
		{
			WriteRelCall(0x540627, (UInt32)(&PlaySound_Hook));
		}
	};

	struct IsMenuMode
	{
		static char __cdecl IsMenuMode_Hook()
		{
			if (!g_thePlayer || !(*g_thePlayer) || ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Loading))
				|| ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Main))
				|| ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Pause))) {
				return 1;
			}

			return 0;
		}

		static char __cdecl IsMenuMode_HookDialogue()
		{
			if (!g_thePlayer || !(*g_thePlayer) || ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Loading))
				|| ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Dialog))
				|| ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Main))
				|| ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Pause))
				|| ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Persuasion))
				|| ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Repair))
				|| ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Negotiate))
				|| ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_SpellPurchase)))
			{
				return 1;
			}

			return 0;
		}

		static void Install()
		{
			//WriteRelCall(0x40D809, (UInt32)(&IsMenuMode_Hook));
			//WriteRelCall(0x40D817, (UInt32)(&IsMenuMode_Hook));
			//WriteRelCall(0x40D9C6, (UInt32)(&IsMenuMode_Hook));
			WriteRelCall(0x40D9D4, (UInt32)(&IsMenuMode_Hook)); // menu shader
			//WriteRelCall(0x40DA78, (UInt32)(&IsMenuMode_Hook));
			WriteRelCall(0x40DBAB, (UInt32)(&IsMenuMode_HookDialogue)); // sound
			WriteRelCall(0x40DB5B, (UInt32)(&IsMenuMode_Hook)); // animations
			WriteRelCall(0x40DBFB, (UInt32)(&IsMenuMode_Hook)); // actors
			//WriteRelCall(0x40DC61, (UInt32)(&IsMenuMode_Hook)); // player controls
			//WriteRelCall(0x40DD6E, (UInt32)(&IsMenuMode_Hook));
			//WriteRelCall(0x40DD5F, (UInt32)(&IsMenuMode_Hook));
			WriteRelCall(0x40DE3F, (UInt32)(&IsMenuMode_Hook)); // physics
			WriteRelCall(0x40DE76, (UInt32)(&IsMenuMode_Hook)); // weather
			//WriteRelCall(0x40DEA6, (UInt32)(&IsMenuMode_Hook));
			//WriteRelCall(0x4F4582, (UInt32)(&IsMenuMode_Hook)); // scripts??
			WriteRelCall(0x663176, (UInt32)(&IsMenuMode_Hook)); // scripts??
			//WriteRelCall(0x405297, (UInt32)(&IsMenuMode_Hook));

		}
	};

	struct SetHavokPaused
	{
		using TrampolineFn = void(*)();
		using SetHavokPaused_t = void(__cdecl*)(UInt8 paused);
		static inline SetHavokPaused_t g_originalSetHavokPaused = nullptr;

		const static UInt32 HAVOK_PAUSED_FLAG = 0x00BA790A;

		static void ForceHavokUnpaused()
		{
			*reinterpret_cast<volatile UInt8*>(HAVOK_PAUSED_FLAG) = 0;
		}

		static void __cdecl SetHavokPaused_Hook(UInt8 paused)
		{

			// Player not created yet -> main menu / loading
			if (!g_thePlayer || !(*g_thePlayer) || ((InterfaceManager::GetSingleton()->GetTopVisibleMenuID() == kMenuType_Loading))) {
				g_originalSetHavokPaused(paused);
				return;
			}

			// In-game: always force unpaused
			g_originalSetHavokPaused(0);
			ForceHavokUnpaused();
		}


		static void Install()
		{
			uintptr_t callSite = 0x0040D80F;
			uintptr_t target = 0x00889A30; // SetHavokPaused

			g_originalSetHavokPaused = reinterpret_cast<SetHavokPaused_t>(target);

			// write CALL rel32
			intptr_t rel = (intptr_t)&SetHavokPaused_Hook - (callSite + 5);

			SafeWrite8(callSite, 0xE8);
			SafeWrite32(callSite + 1, static_cast<UInt32>(rel));

			_MESSAGE("Installed SetHavokPaused hook");
		}
	};

	void Install()
	{
		_MESSAGE("-HOOKS-");
		SetHavokPaused::Install();
		IsMenuMode::Install();
		PlaySoundImpl::Install();
		AnimUpdate::Install();
		AIUpdate::Install();
		_MESSAGE("Installed all hooks");

	}
}
/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// Ammo.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "pm_shared.h"

#include <string.h>
#include <stdio.h>

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "Weapons.h"

#include "CWeaponHUDInfo.h"

#include "ammohistory.h"
#include "vgui_TeamFortressViewport.h"

#include "CWeaponInfoCache.h"

WEAPON *gpActiveSel;	// NULL means off, 1 means just the menu bar, otherwise
						// this points to the active weapon menu item
WEAPON *gpLastSel;		// Last weapon menu selection 

WeaponsResource gWR;

int g_weaponselect = 0;

void WeaponsResource::Init()
{
	memset( rgWeapons, 0, sizeof rgWeapons );
	Reset();
}

void WeaponsResource::VidInit()
{
	//Zero out the weapon info so custom weapons don't get stuck in the list after map changes. - Solokiller
	memset( rgWeapons, 0, sizeof( rgWeapons ) );
}

int WeaponsResource::CountAmmo( int iId ) const
{ 
	if ( iId < 0 )
		return 0;

	return riAmmo[iId];
}

bool WeaponsResource::HasAmmo( const WEAPON* const p ) const
{
	if ( !p || !p->pInfo )
		return false;

	// weapons with no max ammo can always be selected
	if ( !p->pInfo->GetPrimaryAmmo() || p->pInfo->GetPrimaryAmmo()->GetMaxCarry() == WEAPON_NOCLIP )
		return true;

	return p->iClip > 0 ||
		CountAmmo( p->pInfo->GetPrimaryAmmo()->GetID() )||
		( p->pInfo->GetSecondaryAmmo() && CountAmmo( p->pInfo->GetSecondaryAmmo()->GetID() ) ) ||
		( p->pInfo->GetFlags() & ITEM_FLAG_SELECTONEMPTY );
}

// Returns the first weapon for a given slot.
WEAPON *WeaponsResource :: GetFirstPos( int iSlot )
{
	WEAPON *pret = NULL;

	for (int i = 0; i < MAX_WEAPON_POSITIONS; i++)
	{
		if ( rgSlots[iSlot][i] && HasAmmo( rgSlots[iSlot][i] ) )
		{
			pret = rgSlots[iSlot][i];
			break;
		}
	}

	return pret;
}


WEAPON* WeaponsResource :: GetNextActivePos( int iSlot, int iSlotPos )
{
	if ( iSlotPos >= MAX_WEAPON_POSITIONS || iSlot >= MAX_WEAPON_SLOTS )
		return NULL;

	WEAPON *p = gWR.rgSlots[ iSlot ][ iSlotPos+1 ];
	
	if ( !p || !gWR.HasAmmo(p) )
		return GetNextActivePos( iSlot, iSlotPos + 1 );

	return p;
}

int giBucketHeight, giBucketWidth, giABHeight, giABWidth; // Ammo Bar width and height

HSPRITE ghsprBuckets;					// Sprite for top row of weapons menu

DECLARE_MESSAGE(m_Ammo, CurWeapon );	// Current weapon and clip
DECLARE_MESSAGE(m_Ammo, AmmoX);			// update known ammo type's count
DECLARE_MESSAGE(m_Ammo, AmmoPickup);	// flashes an ammo pickup record
DECLARE_MESSAGE(m_Ammo, WeapPickup);    // flashes a weapon pickup record
DECLARE_MESSAGE(m_Ammo, HideWeapon);	// hides the weapon, ammo, and crosshair displays temporarily
DECLARE_MESSAGE(m_Ammo, ItemPickup);

DECLARE_COMMAND(m_Ammo, Slot1);
DECLARE_COMMAND(m_Ammo, Slot2);
DECLARE_COMMAND(m_Ammo, Slot3);
DECLARE_COMMAND(m_Ammo, Slot4);
DECLARE_COMMAND(m_Ammo, Slot5);
DECLARE_COMMAND(m_Ammo, Slot6);
DECLARE_COMMAND(m_Ammo, Slot7);
DECLARE_COMMAND(m_Ammo, Slot8);
DECLARE_COMMAND(m_Ammo, Slot9);
DECLARE_COMMAND(m_Ammo, Slot10);
DECLARE_COMMAND(m_Ammo, Close);
DECLARE_COMMAND(m_Ammo, NextWeapon);
DECLARE_COMMAND(m_Ammo, PrevWeapon);

// width of ammo fonts
#define AMMO_SMALL_WIDTH 10
#define AMMO_LARGE_WIDTH 20

#define HISTORY_DRAW_TIME	"5"

bool CHudAmmo::Init()
{
	gHUD.AddHudElem(this);

	HOOK_MESSAGE(CurWeapon);
	HOOK_MESSAGE(AmmoPickup);
	HOOK_MESSAGE(WeapPickup);
	HOOK_MESSAGE(ItemPickup);
	HOOK_MESSAGE(HideWeapon);
	HOOK_MESSAGE(AmmoX);

	HOOK_COMMAND("slot1", Slot1);
	HOOK_COMMAND("slot2", Slot2);
	HOOK_COMMAND("slot3", Slot3);
	HOOK_COMMAND("slot4", Slot4);
	HOOK_COMMAND("slot5", Slot5);
	HOOK_COMMAND("slot6", Slot6);
	HOOK_COMMAND("slot7", Slot7);
	HOOK_COMMAND("slot8", Slot8);
	HOOK_COMMAND("slot9", Slot9);
	HOOK_COMMAND("slot10", Slot10);
	HOOK_COMMAND("cancelselect", Close);
	HOOK_COMMAND("invnext", NextWeapon);
	HOOK_COMMAND("invprev", PrevWeapon);

	Reset();

	CVAR_CREATE( "hud_drawhistory_time", HISTORY_DRAW_TIME, 0 );
	CVAR_CREATE( "hud_fastswitch", "0", FCVAR_ARCHIVE );		// controls whether or not weapons can be selected in one keypress

	m_pCrosshair = gEngfuncs.pfnGetCvarPointer( "crosshair" );
	m_pCrosshairMode = CVAR_CREATE( "crosshair_mode", "0", FCVAR_ARCHIVE );
	m_pCrosshairScale = CVAR_CREATE( "crosshair_scale", "1", FCVAR_ARCHIVE );

	m_iFlags |= HUD_ACTIVE; //!!!

	gWR.Init();
	gHR.Init();

	return true;
};

void CHudAmmo::Reset()
{
	m_fFade = 0;
	m_iFlags |= HUD_ACTIVE; //!!!

	gpActiveSel = NULL;
	gHUD.m_iHideHUDDisplay = 0;

	gWR.Reset();
	gHR.Reset();
}

bool CHudAmmo::VidInit()
{
	gWR.VidInit();

	// Load sprites for buckets (top row of weapon menu)
	m_HUD_bucket0 = gHUD.GetSpriteIndex( "bucket1" );
	m_HUD_selection = gHUD.GetSpriteIndex( "selection" );

	ghsprBuckets = gHUD.GetSprite(m_HUD_bucket0);
	giBucketWidth = gHUD.GetSpriteRect(m_HUD_bucket0).right - gHUD.GetSpriteRect(m_HUD_bucket0).left;
	giBucketHeight = gHUD.GetSpriteRect(m_HUD_bucket0).bottom - gHUD.GetSpriteRect(m_HUD_bucket0).top;

	gHR.iHistoryGap = max( gHR.iHistoryGap, gHUD.GetSpriteRect(m_HUD_bucket0).bottom - gHUD.GetSpriteRect(m_HUD_bucket0).top);

	if (ScreenWidth >= 640)
	{
		giABWidth = 20;
		giABHeight = 4;
	}
	else
	{
		giABWidth = 10;
		giABHeight = 2;
	}

	return true;
}

//
// Think:
//  Used for selection of weapon menu item.
//
void CHudAmmo::Think()
{
	if( m_bNeedsLocalUpdate )
	{
		m_bNeedsLocalUpdate = false;
		UpdateWeaponHUD( m_pWeapon, m_bOnTarget );
	}

	if ( gHUD.m_bPlayerDead )
		return;

	if ( gHUD.m_iWeaponBits != gWR.iOldWeaponBits )
	{
		gWR.iOldWeaponBits = gHUD.m_iWeaponBits;

		for (int i = MAX_WEAPONS-1; i > 0; i-- )
		{
			WEAPON *p = gWR.GetWeapon(i);

			if ( p && p->pInfo )
			{
				if ( gHUD.m_iWeaponBits & ( 1 << p->pInfo->GetID() ) )
					gWR.PickupWeapon( p );
				else
					gWR.DropWeapon( p );
			}
		}
	}

	if (!gpActiveSel)
		return;

	// has the player selected one?
	if (gHUD.m_iKeyBits & IN_ATTACK)
	{
		if (gpActiveSel != (WEAPON *)1)
		{
			ServerCmd(gpActiveSel->pInfo->GetWeaponName());
			g_weaponselect = gpActiveSel->pInfo->GetID();
		}

		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		gHUD.m_iKeyBits &= ~IN_ATTACK;

		PlaySound("common/wpn_select.wav", 1);
	}

}

//
// Helper function to return a Ammo pointer from id
//
const WeaponHUDSprite* WeaponsResource :: GetAmmoPicFromWeapon( int iAmmoId ) const
{
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if( rgWeapons[ i ].pInfo )
		{
			if ( rgWeapons[i].pInfo->GetPrimaryAmmo() && rgWeapons[ i ].pInfo->GetPrimaryAmmo()->GetID() == iAmmoId )
			{
				return &rgWeapons[ i ].pInfo->GetHUDInfo()->GetPrimaryAmmo();
			}
			else if ( rgWeapons[i].pInfo->GetSecondaryAmmo() && rgWeapons[ i ].pInfo->GetSecondaryAmmo()->GetID() == iAmmoId )
			{
				return &rgWeapons[ i ].pInfo->GetHUDInfo()->GetSecondaryAmmo();
			}
		}
	}

	return nullptr;
}


// Menu Selection Code

void WeaponsResource :: SelectSlot( int iSlot, const bool fAdvance, int iDirection )
{
	if ( gHUD.m_Menu.m_fMenuDisplayed && !fAdvance && (iDirection == 1) )	
	{ // menu is overriding slot use commands
		gHUD.m_Menu.SelectMenuItem( iSlot + 1 );  // slots are one off the key numbers
		return;
	}

	if ( iSlot > MAX_WEAPON_SLOTS )
		return;

	if ( gHUD.m_bPlayerDead || gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) )
		return;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return;

	if ( ! ( gHUD.m_iWeaponBits & ~(1<<(WEAPON_SUIT)) ))
		return;

	WEAPON *p = NULL;
	bool fastSwitch = CVAR_GET_FLOAT( "hud_fastswitch" ) != 0;

	if ( (gpActiveSel == NULL) || (gpActiveSel == (WEAPON *)1) || (iSlot != gpActiveSel->pInfo->GetBucket()) )
	{
		PlaySound( "common/wpn_hudon.wav", 1 );
		p = GetFirstPos( iSlot );

		if ( p && fastSwitch ) // check for fast weapon switch mode
		{
			// if fast weapon switch is on, then weapons can be selected in a single keypress
			// but only if there is only one item in the bucket
			WEAPON *p2 = GetNextActivePos( p->pInfo->GetBucket(), p->pInfo->GetPosition() );
			if ( !p2 )
			{	// only one active item in bucket, so change directly to weapon
				ServerCmd( p->pInfo->GetWeaponName() );
				g_weaponselect = p->pInfo->GetID();
				return;
			}
		}
	}
	else
	{
		PlaySound("common/wpn_moveselect.wav", 1);
		if ( gpActiveSel )
			p = GetNextActivePos( gpActiveSel->pInfo->GetBucket(), gpActiveSel->pInfo->GetPosition() );
		if ( !p )
			p = GetFirstPos( iSlot );
	}

	
	if ( !p )  // no selection found
	{
		// just display the weapon list, unless fastswitch is on just ignore it
		if ( !fastSwitch )
			gpActiveSel = (WEAPON *)1;
		else
			gpActiveSel = NULL;
	}
	else 
		gpActiveSel = p;
}

void WeaponsResource::SyncWithWeapons()
{
	//Only zero out the info, the clip might already be valid again. - Solokiller
	for( auto& weapon : rgWeapons )
	{
		weapon.pInfo = nullptr;
	}

	g_WeaponInfoCache.EnumInfos(
		[]( const CWeaponInfo& info, void* pUserData ) -> bool
	{
		WEAPON Weapon;

		memset( &Weapon, 0, sizeof( Weapon ) );

		Weapon.pInfo = &info;

		gWR.AddWeapon( &Weapon );

		return true;
	}
	);
}

//------------------------------------------------------------------------
// Message Handlers
//------------------------------------------------------------------------

//
// AmmoX  -- Update the count of a known type of ammo
// 
int CHudAmmo::MsgFunc_AmmoX(const char *pszName, int iSize, void *pbuf)
{
	CBufferReader reader( pbuf, iSize );

	int iIndex = reader.ReadByte();
	int iCount = reader.ReadByte();

	gWR.SetAmmo( iIndex, abs(iCount) );

	return 1;
}

int CHudAmmo::MsgFunc_AmmoPickup( const char *pszName, int iSize, void *pbuf )
{
	CBufferReader reader( pbuf, iSize );
	int iIndex = reader.ReadByte();
	int iCount = reader.ReadByte();

	// Add ammo to the history
	gHR.AddToHistory( HISTSLOT_AMMO, iIndex, abs(iCount) );

	return 1;
}

int CHudAmmo::MsgFunc_WeapPickup( const char *pszName, int iSize, void *pbuf )
{
	CBufferReader reader( pbuf, iSize );
	int iIndex = reader.ReadByte();

	// Add the weapon to the history
	gHR.AddToHistory( HISTSLOT_WEAP, iIndex );

	return 1;
}

int CHudAmmo::MsgFunc_ItemPickup( const char *pszName, int iSize, void *pbuf )
{
	CBufferReader reader( pbuf, iSize );
	const char *szName = reader.ReadString();

	// Add the weapon to the history
	gHR.AddToHistory( HISTSLOT_ITEM, szName );

	return 1;
}


int CHudAmmo::MsgFunc_HideWeapon( const char *pszName, int iSize, void *pbuf )
{
	CBufferReader reader( pbuf, iSize );
	
	gHUD.m_iHideHUDDisplay = reader.ReadByte();

	if (gEngfuncs.IsSpectateOnly())
		return 1;

	if ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) )
	{
		static wrect_t nullrc;
		gpActiveSel = NULL;
		SetCrosshair( 0, nullrc, 0, 0, 0 );
	}
	else
	{
		if ( m_pWeapon )
		{
			const auto& crosshair = m_pWeapon->pInfo->GetHUDInfo()->GetCrosshair();
			SetCrosshair( crosshair.hSprite, crosshair.rect, 255, 255, 255 );
		}
	}

	return 1;
}

// 
//  CurWeapon: Update hud state with the current weapon and clip count. Ammo
//  counts are updated with AmmoX. Server assures that the Weapon ammo type 
//  numbers match a real ammo type.
//
int CHudAmmo::MsgFunc_CurWeapon(const char *pszName, int iSize, void *pbuf )
{
	static wrect_t nullrc;

	CBufferReader reader( pbuf, iSize );

	const WpnOnTargetState state = static_cast<WpnOnTargetState>( reader.ReadByte() );
	int iId = reader.ReadChar();
	int iClip = reader.ReadChar();

	// detect if we're also on target
	bool fOnTarget = state == WpnOnTargetState::ACTIVE_IS_ONTARGET;

	if ( iId < 1 )
	{
		SetCrosshair(0, nullrc, 0, 0, 0);
		return 0;
	}

	if ( g_iUser1 != OBS_IN_EYE )
	{
		// Is player dead???
		if ((iId == -1) && (iClip == -1))
		{
			gHUD.m_bPlayerDead = true;
			gpActiveSel = NULL;
			return 1;
		}
		gHUD.m_bPlayerDead = false;
	}

	WEAPON *pWeapon = gWR.GetWeapon( iId );

	if ( !pWeapon )
		return 0;

	if ( iClip < -1 )
		pWeapon->iClip = abs(iClip);
	else
		pWeapon->iClip = iClip;


	if ( state == WpnOnTargetState::NOT_ACTIVE_WEAPON )	// we're not the current weapon, so update no more
		return 1;

	m_pWeapon = pWeapon;

	if( !m_pWeapon->pInfo )
	{
		m_bNeedsLocalUpdate = true;
		m_bOnTarget = fOnTarget;
		m_iFlags |= HUD_ACTIVE;
		return 1;
	}
	else if( m_bNeedsLocalUpdate )
		m_bNeedsLocalUpdate = false;

	UpdateWeaponHUD( m_pWeapon, fOnTarget );
	
	return 1;
}

void CHudAmmo::UpdateWeaponHUD( WEAPON* pWeapon, bool bOnTarget )
{
	auto pHUDInfo = pWeapon->pInfo->GetHUDInfo();

	if( gHUD.m_iFOV >= 90 )
	{ // normal crosshairs
		if( bOnTarget && pHUDInfo->GetAutoAim().hSprite )
			SetCrosshair( pHUDInfo->GetAutoAim().hSprite, pHUDInfo->GetAutoAim().rect, 255, 255, 255 );
		else
			SetCrosshair( pHUDInfo->GetCrosshair().hSprite, pHUDInfo->GetCrosshair().rect, 255, 255, 255 );
	}
	else
	{ // zoomed crosshairs
		if( bOnTarget && pHUDInfo->GetZoomedAutoAim().hSprite )
			SetCrosshair( pHUDInfo->GetZoomedAutoAim().hSprite, pHUDInfo->GetZoomedAutoAim().rect, 255, 255, 255 );
		else
			SetCrosshair( pHUDInfo->GetZoomedCrosshair().hSprite, pHUDInfo->GetZoomedCrosshair().rect, 255, 255, 255 );

	}

	m_fFade = 200.0f; //!!!
	m_iFlags |= HUD_ACTIVE;
}

//------------------------------------------------------------------------
// Command Handlers
//------------------------------------------------------------------------
// Slot button pressed
void CHudAmmo::SlotInput( int iSlot )
{
	if ( gViewPort && gViewPort->SlotInput( iSlot ) )
		return;

	gWR.SelectSlot(iSlot, false, 1);
}

void CHudAmmo::UserCmd_Slot1(void)
{
	SlotInput( 0 );
}

void CHudAmmo::UserCmd_Slot2(void)
{
	SlotInput( 1 );
}

void CHudAmmo::UserCmd_Slot3(void)
{
	SlotInput( 2 );
}

void CHudAmmo::UserCmd_Slot4(void)
{
	SlotInput( 3 );
}

void CHudAmmo::UserCmd_Slot5(void)
{
	SlotInput( 4 );
}

void CHudAmmo::UserCmd_Slot6(void)
{
	SlotInput( 5 );
}

void CHudAmmo::UserCmd_Slot7(void)
{
	SlotInput( 6 );
}

void CHudAmmo::UserCmd_Slot8(void)
{
	SlotInput( 7 );
}

void CHudAmmo::UserCmd_Slot9(void)
{
	SlotInput( 8 );
}

void CHudAmmo::UserCmd_Slot10(void)
{
	SlotInput( 9 );
}

void CHudAmmo::UserCmd_Close(void)
{
	if (gpActiveSel)
	{
		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		PlaySound("common/wpn_hudoff.wav", 1);
	}
	else
		EngineClientCmd("escape");
}


// Selects the next item in the weapon menu
void CHudAmmo::UserCmd_NextWeapon(void)
{
	if ( gHUD.m_bPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) )
		return;

	if ( !gpActiveSel || gpActiveSel == (WEAPON*)1 )
		gpActiveSel = m_pWeapon;

	int pos = 0;
	int slot = 0;
	if ( gpActiveSel )
	{
		pos = gpActiveSel->pInfo->GetPosition() + 1;
		slot = gpActiveSel->pInfo->GetBucket();
	}

	for ( int loop = 0; loop <= 1; loop++ )
	{
		for ( ; slot < MAX_WEAPON_SLOTS; slot++ )
		{
			for ( ; pos < MAX_WEAPON_POSITIONS; pos++ )
			{
				WEAPON *wsp = gWR.GetWeaponSlot( slot, pos );

				if ( wsp && gWR.HasAmmo(wsp) )
				{
					gpActiveSel = wsp;
					return;
				}
			}

			pos = 0;
		}

		slot = 0;  // start looking from the first slot again
	}

	gpActiveSel = NULL;
}

// Selects the previous item in the menu
void CHudAmmo::UserCmd_PrevWeapon(void)
{
	if ( gHUD.m_bPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) )
		return;

	if ( !gpActiveSel || gpActiveSel == (WEAPON*)1 )
		gpActiveSel = m_pWeapon;

	int pos = MAX_WEAPON_POSITIONS-1;
	int slot = MAX_WEAPON_SLOTS-1;
	if ( gpActiveSel )
	{
		pos = gpActiveSel->pInfo->GetPosition() - 1;
		slot = gpActiveSel->pInfo->GetBucket();
	}
	
	for ( int loop = 0; loop <= 1; loop++ )
	{
		for ( ; slot >= 0; slot-- )
		{
			for ( ; pos >= 0; pos-- )
			{
				WEAPON *wsp = gWR.GetWeaponSlot( slot, pos );

				if ( wsp && gWR.HasAmmo(wsp) )
				{
					gpActiveSel = wsp;
					return;
				}
			}

			pos = MAX_WEAPON_POSITIONS-1;
		}
		
		slot = MAX_WEAPON_SLOTS-1;
	}

	gpActiveSel = NULL;
}

#include "renderer/SpriteRenderUtils.h"

//-------------------------------------------------------------------------
// Drawing code
//-------------------------------------------------------------------------

bool CHudAmmo::Draw(float flTime)
{
	int a, x, y, r, g, b;
	int AmmoWidth;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return true;

	if ( (gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL )) )
		return true;

	// Draw Weapon Menu
	DrawWList(flTime);

	// Draw ammo pickup history
	gHR.DrawAmmoHistory( flTime );

	if (!(m_iFlags & HUD_ACTIVE))
		return false;

	if (!m_pWeapon)
		return false;

	WEAPON *pw = m_pWeapon; // shorthand

	// SPR_Draw Ammo
	if( !pw->pInfo->GetPrimaryAmmo() && !pw->pInfo->GetSecondaryAmmo() )
		return false;


	int iFlags = DHN_DRAWZERO; // draw 0 values

	AmmoWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).left;

	a = (int) max( static_cast<float>( MIN_ALPHA ), m_fFade );

	if (m_fFade > 0)
		m_fFade -= (gHUD.m_flTimeDelta * 20);

	gHUD.GetPrimaryColor().UnpackRGB(r,g,b);

	ScaleColors(r, g, b, a );

	// Does this weapon have a clip?
	y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight/2;

	if(  m_pCrosshair->value != 0 && m_hCrosshair != INVALID_HSPRITE )
	{
		float flScale;

		switch( static_cast<int>( m_pCrosshairMode->value ) )
		{
		default:
		case CROSS_NATIVESCALE:
			flScale = 1;
			break;

		case CROSS_RESSCALE:
			flScale = max( ( ScreenWidth / 640.0 ) * 0.75, 1.0 );
			break;

		case CROSS_USERSCALE:
			flScale = abs( m_pCrosshairScale->value );
			break;
		}

		class CCrosshairTriCallback : public ITriCoordFallback
		{
		public:
			void Calculate( HSPRITE hSprite, const int frame, const wrect_t& rect, const float flScale, int& x, int& y ) override
			{
				x = ( ScreenWidth - ( ( rect.right - rect.left ) ) ) / 2;
				y = ( ScreenHeight - ( ( rect.bottom - rect.top ) ) ) / 2;
			}
		};

		CCrosshairTriCallback callback;

		Tri_DrawScaledSprite( m_hCrosshair, 0, m_iR, m_iG, m_iB, 255, kRenderTransTexture, flScale, callback, &m_CrosshairRC );
	}

	// Does weapon have any ammo at all?
	if ( auto pAmmo = pw->pInfo->GetPrimaryAmmo() )
	{
		const auto& ammo = m_pWeapon->pInfo->GetHUDInfo()->GetPrimaryAmmo();

		int iIconWidth = ammo.rect.right - ammo.rect.left;
		
		if (pw->iClip >= 0)
		{
			// room for the number and the '|' and the current ammo
			
			x = ScreenWidth - (8 * AmmoWidth) - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, pw->iClip, r, g, b);

			wrect_t rc;
			rc.top = 0;
			rc.left = 0;
			rc.right = AmmoWidth;
			rc.bottom = 100;

			int iBarWidth =  AmmoWidth/10;

			x += AmmoWidth/2;

			gHUD.GetPrimaryColor().UnpackRGB(r,g,b);

			// draw the | bar
			FillRGBA(x, y, iBarWidth, gHUD.m_iFontHeight, r, g, b, a);

			x += iBarWidth + AmmoWidth/2;;

			// GL Seems to need this
			ScaleColors(r, g, b, a );
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo( pAmmo->GetID() ), r, g, b);


		}
		else
		{
			// SPR_Draw a bullets only line
			x = ScreenWidth - 4 * AmmoWidth - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo( pAmmo->GetID() ), r, g, b);
		}

		// Draw the ammo Icon
		int iOffset = ( ammo.rect.bottom - ammo.rect.top)/8;
		SPR_Set( ammo.hSprite, r, g, b);
		SPR_DrawAdditive(0, x, y - iOffset, &ammo.rect );
	}

	// Does weapon have seconday ammo?
	if ( auto pAmmo = pw->pInfo->GetSecondaryAmmo() ) 
	{
		const auto& ammo2 = m_pWeapon->pInfo->GetHUDInfo()->GetSecondaryAmmo();

		int iIconWidth = ammo2.rect.right - ammo2.rect.left;

		// Do we have secondary ammo?
		if ( gWR.CountAmmo( pAmmo->GetID() ) > 0 )
		{
			y -= gHUD.m_iFontHeight + gHUD.m_iFontHeight/4;
			x = ScreenWidth - 4 * AmmoWidth - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags|DHN_3DIGITS, gWR.CountAmmo( pAmmo->GetID() ), r, g, b);

			// Draw the ammo Icon
			SPR_Set( ammo2.hSprite, r, g, b);
			int iOffset = ( ammo2.rect.bottom - ammo2.rect.top)/8;
			SPR_DrawAdditive(0, x, y - iOffset, &ammo2.rect );
		}
	}
	return true;
}


//
// Draws the ammo bar on the hud
//
int DrawBar(int x, int y, int width, int height, float f)
{
	int r, g, b;

	if (f < 0)
		f = 0;
	if (f > 1)
		f = 1;

	if (f)
	{
		int w = f * width;

		// Always show at least one pixel if we have ammo.
		if (w <= 0)
			w = 1;
		const auto& barColor = gHUD.GetAmmoBarColor();

		FillRGBA(x, y, w, height, barColor.r, barColor.g, barColor.b, 255);
		x += w;
		width -= w;
	}

	gHUD.GetPrimaryColor().UnpackRGB(r, g, b);

	FillRGBA(x, y, width, height, r, g, b, 128);

	return (x + width);
}



void DrawAmmoBar(WEAPON *p, int x, int y, int width, int height)
{
	if ( !p )
		return;
	
	if ( auto pAmmo = p->pInfo->GetPrimaryAmmo() )
	{
		if( !gWR.CountAmmo( pAmmo->GetID() ) )
			return;

		float f = (float)gWR.CountAmmo( pAmmo->GetID() )/(float) pAmmo->GetMaxCarry();
		
		x = DrawBar(x, y, width, height, f);


		// Do we have secondary ammo too?

		if ( auto pAmmo2 = p->pInfo->GetSecondaryAmmo() )
		{
			f = (float)gWR.CountAmmo( pAmmo2->GetID() )/(float) pAmmo2->GetMaxCarry();

			x += 5; //!!!

			DrawBar(x, y, width, height, f);
		}
	}
}




//
// Draw Weapon Menu
//
int CHudAmmo::DrawWList(float flTime)
{
	int r,g,b,x,y,a,i;

	if ( !gpActiveSel )
		return 0;

	int iActiveSlot;

	if ( gpActiveSel == (WEAPON *)1 )
		iActiveSlot = -1;	// current slot has no weapons
	else 
		iActiveSlot = gpActiveSel->pInfo->GetBucket();

	x = 10; //!!!
	y = 10; //!!!
	

	// Ensure that there are available choices in the active slot
	if ( iActiveSlot > 0 )
	{
		if ( !gWR.GetFirstPos( iActiveSlot ) )
		{
			gpActiveSel = (WEAPON *)1;
			iActiveSlot = -1;
		}
	}
		
	// Draw top line
	for ( i = 0; i < MAX_WEAPON_SLOTS; i++ )
	{
		int iWidth;

		gHUD.GetPrimaryColor().UnpackRGB(r,g,b);
	
		if ( iActiveSlot == i )
			a = 255;
		else
			a = 192;

		ScaleColors(r, g, b, 255);
		SPR_Set(gHUD.GetSprite(m_HUD_bucket0 + i), r, g, b );

		// make active slot wide enough to accomodate gun pictures
		if ( i == iActiveSlot )
		{
			WEAPON *p = gWR.GetFirstPos(iActiveSlot);
			if ( p )
				iWidth = p->pInfo->GetHUDInfo()->GetActive().rect.right - p->pInfo->GetHUDInfo()->GetActive().rect.left;
			else
				iWidth = giBucketWidth;
		}
		else
			iWidth = giBucketWidth;

		SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_bucket0 + i));
		
		x += iWidth + 5;
	}


	a = 128; //!!!
	x = 10;

	// Draw all of the buckets
	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		y = giBucketHeight + 10;

		// If this is the active slot, draw the bigger pictures,
		// otherwise just draw boxes
		if ( i == iActiveSlot )
		{
			WEAPON *p = gWR.GetFirstPos( i );
			int iWidth = giBucketWidth;
			if ( p )
				iWidth = p->pInfo->GetHUDInfo()->GetActive().rect.right - p->pInfo->GetHUDInfo()->GetActive().rect.left;

			for ( int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++ )
			{
				p = gWR.GetWeaponSlot( i, iPos );

				if ( !p || !p->pInfo )
					continue;

				auto pHUDInfo = p->pInfo->GetHUDInfo();

				gHUD.GetPrimaryColor().UnpackRGB( r,g,b );
			
				// if active, then we must have ammo.

				if ( gpActiveSel == p )
				{
					SPR_Set( pHUDInfo->GetActive().hSprite, r, g, b );
					SPR_DrawAdditive(0, x, y, &pHUDInfo->GetActive().rect );

					SPR_Set(gHUD.GetSprite(m_HUD_selection), r, g, b );
					SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_selection));
				}
				else
				{
					// Draw Weapon if Red if no ammo

					if ( gWR.HasAmmo(p) )
						ScaleColors(r, g, b, 192);
					else
					{
						gHUD.GetEmptyItemColor().UnpackRGB(r,g,b);
						ScaleColors(r, g, b, 128);
					}

					SPR_Set( pHUDInfo->GetInactive().hSprite, r, g, b );
					SPR_DrawAdditive( 0, x, y, &pHUDInfo->GetInactive().rect );
				}

				// Draw Ammo Bar

				DrawAmmoBar(p, x + giABWidth/2, y, giABWidth, giABHeight);
				
				y += pHUDInfo->GetActive().rect.bottom - pHUDInfo->GetActive().rect.top + 5;
			}

			x += iWidth + 5;

		}
		else
		{
			// Draw Row of weapons.

			gHUD.GetPrimaryColor().UnpackRGB(r,g,b);

			for ( int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++ )
			{
				WEAPON *p = gWR.GetWeaponSlot( i, iPos );
				
				if ( !p || !p->pInfo )
					continue;

				if ( gWR.HasAmmo(p) )
				{
					gHUD.GetPrimaryColor().UnpackRGB(r,g,b);
					a = 128;
				}
				else
				{
					gHUD.GetEmptyItemColor().UnpackRGB(r,g,b);
					a = 96;
				}

				FillRGBA( x, y, giBucketWidth, giBucketHeight, r, g, b, a );

				y += giBucketHeight + 5;
			}

			x += giBucketWidth + 5;
		}
	}	

	return 1;

}

void CHudAmmo::SetCrosshair( HSPRITE hCrosshair, const wrect_t& rect, int r, int g, int b )
{
	m_hCrosshair = hCrosshair;
	m_CrosshairRC = rect;
	m_iR = r;
	m_iG = g;
	m_iB = b;
}

void SetCrosshair( HSPRITE hCrosshair, const wrect_t& crosshairRC, int r, int g, int b )
{
	//gEngfuncs.pfnSetCrosshair( hCrosshair, crosshairRC, r, g, b );
	gHUD.m_Ammo.SetCrosshair( hCrosshair, crosshairRC, r, g, b );
}
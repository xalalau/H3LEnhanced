/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
#ifndef GAME_CLIENT_UI_HUD_CHUDTRAIN_H
#define GAME_CLIENT_UI_HUD_CHUDTRAIN_H

#include "shared/hud/CHudElement.h"
#include "hud.h"

class CHudTrain : public CBaseHudElement<CHud>
{
public:
	DECLARE_CLASS( CHudTrain, CBaseHudElement<CHud> );

	CHudTrain( const char* const pszName, CHud& hud );

	void Init() override;
	void VidInit() override;
	bool Draw( float flTime ) override;
	int MsgFunc_Train( const char *pszName, int iSize, void *pbuf );

private:
	HSPRITE m_hSprite;
	int m_iPos;
};

#endif //GAME_CLIENT_UI_HUD_CHUDTRAIN_H
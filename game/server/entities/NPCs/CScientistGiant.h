/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
#ifndef GAME_SERVER_ENTITIES_NPCS_CSCIENTISTGIANT_H
#define GAME_SERVER_ENTITIES_NPCS_CSCIENTISTGIANT_H

class CScientistGiant : public CScientist
{
public:
	DECLARE_CLASS( CScientistGiant, CScientist );
	DECLARE_DATADESC();

	void Spawn(void);
	void Precache(void);

private:
	float m_painTime;
	float m_healTime;
	float m_fearTime;
};

#endif //GAME_SERVER_ENTITIES_NPCS_CSCIENTISTGIANT_H
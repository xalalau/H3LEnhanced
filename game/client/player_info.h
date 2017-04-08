/***
*
*	Copyright (c) 2003', Valve LLC. All rights reserved.
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

#ifndef GAME_CLIENT_PLAYER_INFO_H
#define GAME_CLIENT_PLAYER_INFO_H

/**
*	Player info from the engine
*/
extern hud_player_info_t	g_PlayerInfoList[MAX_PLAYERS+1];

/**
*	Additional player info sent directly to the client dll
*/
extern extra_player_info_t  g_PlayerExtraInfo[MAX_PLAYERS+1];
extern team_info_t			g_TeamInfo[MAX_TEAMS+1];
extern int					g_IsSpectator[MAX_PLAYERS+1];

#endif //GAME_CLIENT_PLAYER_INFO_H

/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "CCyclerSprite.h"

BEGIN_DATADESC( CCyclerSprite )
	DEFINE_FIELD( m_animate, FIELD_INTEGER ),
	DEFINE_FIELD( m_lastTime, FIELD_TIME ),
	DEFINE_FIELD( m_maxFrame, FIELD_FLOAT ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( cycler_sprite, CCyclerSprite );

void CCyclerSprite::Spawn( void )
{
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_YES;
	pev->effects = 0;

	pev->frame = 0;
	pev->nextthink = gpGlobals->time + 0.1;
	m_animate = 1;
	m_lastTime = gpGlobals->time;

	PRECACHE_MODEL( ( char * ) STRING( pev->model ) );
	SetModel( STRING( pev->model ) );

	m_maxFrame = ( float ) MODEL_FRAMES( GetModelIndex() ) - 1;
}

void CCyclerSprite::Think( void )
{
	if( ShouldAnimate() )
		Animate( pev->framerate * ( gpGlobals->time - m_lastTime ) );

	pev->nextthink = gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}

void CCyclerSprite::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_animate = !m_animate;
	ALERT( at_console, "Sprite: %s\n", STRING( pev->model ) );
}

void CCyclerSprite::OnTakeDamage( const CTakeDamageInfo& info )
{
	if( m_maxFrame > 1.0 )
	{
		Animate( 1.0 );
	}
}

void CCyclerSprite::Animate( float frames )
{
	pev->frame += frames;
	if( m_maxFrame > 0 )
		pev->frame = fmod( pev->frame, m_maxFrame );
}
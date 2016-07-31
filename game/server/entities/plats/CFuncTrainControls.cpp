#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "CFuncTrackTrain.h"

#include "CFuncTrainControls.h"

LINK_ENTITY_TO_CLASS( func_traincontrols, CFuncTrainControls );

void CFuncTrainControls::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SetModel( STRING( pev->model ) );

	SetSize( pev->mins, pev->maxs );
	SetAbsOrigin( GetAbsOrigin() );

	SetThink( &CFuncTrainControls::Find );
	pev->nextthink = gpGlobals->time;
}

void CFuncTrainControls::Find( void )
{
	CBaseEntity* pTarget = nullptr;

	do
	{
		pTarget = UTIL_FindEntityByTargetname( pTarget, GetTarget() );
	}
	while( !FNullEnt( pTarget ) && !pTarget->ClassnameIs( "func_tracktrain" ) );

	if( FNullEnt( pTarget ) )
	{
		ALERT( at_console, "No train %s\n", GetTarget() );
		return;
	}

	CFuncTrackTrain *ptrain = CFuncTrackTrain::Instance( pTarget );
	ptrain->SetControls( this );
	UTIL_Remove( this );
}
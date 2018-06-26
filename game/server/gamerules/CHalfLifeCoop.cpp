// ##############################
// HU3-LIFE REGRAS DO COOP
// ##############################
/*
Este arquivo controla o nosso modo cooperativo quase todo.

Aqui esta uma lista explicada de modificacoes que ficaram espalhadas pelo projeto:

game/server/entities/triggers/CChangeLevel.cpp:
-- Conectada com a funcao ChangeLevelCoop();
game/server/gamerules/CGameRules.*:
-- Uso vazio e generalizado do changelevel do modo coop.
game/server/gamerules/CHu3LifeCoop.*:
-- Guarda as variaveis do modo coop que precisam ser acessadas e modificadas em diversos arquivos.
game/server/gamerules/GameRules.*:
-- Selecao do modo coop modificada para suportar nossos codigos.
game/shared/entities/NPCs/CRoach.cpp:
-- Uma chamada de GetLightLevel() estava crashando o modo coop e foi bloqueada.
game/shared/entities/CWorld.cpp:
-- Precache do sprite do changelevel.
game/server/CGlobalState.cpp:
-- Desativei o keyvalue "globalname".
game/server/gamerules/CHalfLifeRules.cpp:
-- Adicionado comando remove_in_sp.


Possibilidades de uso de comandos em entidades (por dentro dos arquivos .cfg dos mapas):

1) remove_in_coop "entidadeA;EntidadeB;Assimpordiante" = Remove entidades no modo cooperativo
2) remove_in_sp "entidadeA;EntidadeB;Assimpordiante" = Remove entidades no singleplayer
3) disable_physics "entidadeA;EntidadeB;Assimpordiante" = Deixa as entidades com transparencia e efeitos fisicos (sem modificar o seu funcionamento)
4) mp_hu3_trainspawnpoint "nome_de_um_trem" = Faz o modo cooperativo procurar pelo dito func_tracktrain e spawnar os jogadores dentro dele


Ordem de (re)spawn dos jogadores:

1) Tenta dar (re)spawn dentro de algum trem que tenha sido ajeitado para o coop;
2) Tenta dar (re)spawn em um info_player_coop;
3) Tenta dar respawn em relacao a um landmark (so e possivel para jogadores pos changelevel);
4) Se nada estiver disponivel, pega o ponto de spawn padrao (info_player_start).
*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "CBasePlayer.h"
#include "Weapons.h"
#include "CHalfLifeCoop.h"

#include "Skill.h"
#include "Server.h"
#include "entities/items/CItem.h"
#include "entities/spawnpoints/CBaseSpawnPoint.h"
#include "voice_gamemgr.h"
#include "hltv.h"

#include "CWeaponInfoCache.h"

extern DLL_GLOBAL CGameRules	*g_pGameRules;

extern int g_teamplay;

extern cvar_t mp_chattime;

CVoiceGameMgr	g_VoiceGameMgr2;

class CMultiplayGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool CanPlayerHearPlayer(CBasePlayer *pListener, CBasePlayer *pTalker)
	{
		if (g_teamplay)
		{
			if (g_pGameRules->PlayerRelationship(pListener, pTalker) != GR_TEAMMATE)
			{
				return false;
			}
		}

		return true;
	}
};
static CMultiplayGameMgrHelper g_GameMgrHelper;

//*********************************************************
// Rules for the half-life multiplayer game.
//*********************************************************

CBaseHalfLifeCoop::CBaseHalfLifeCoop()
{
	g_VoiceGameMgr2.Init(&g_GameMgrHelper, gpGlobals->maxClients);
}

void CBaseHalfLifeCoop::OnCreate()
{
	CGameRules::OnCreate();

	// 11/8/98
	// Modified by YWB:  Server .cfg file is now a cvar, so that 
	//  server ops can run multiple game servers, with different server .cfg files,
	//  from a single installed directory.
	// Mapcyclefile is already a cvar.

	// 3/31/99
	// Added lservercfg file cvar, since listen and dedicated servers should not
	// share a single config file. (sjb)
	if (IS_DEDICATED_SERVER())
	{
		// this code has been moved into engine, to only run server.cfg once
	}
	else
	{
		// listen server
		char *lservercfgfile = (char *)CVAR_GET_STRING("lservercfgfile");

		if (lservercfgfile && lservercfgfile[0])
		{
			char szCommand[256];

			ALERT(at_console, "Executing listen server config file\n");
			sprintf(szCommand, "exec %s\n", lservercfgfile);
			SERVER_COMMAND(szCommand);
		}
	}
}

bool CBaseHalfLifeCoop::ClientCommand(CBasePlayer *pPlayer, const char *pcmd)
{
	if (g_VoiceGameMgr2.ClientCommand(pPlayer, pcmd))
		return true;

	return CGameRules::ClientCommand(pPlayer, pcmd);
}

cvar_t* CBaseHalfLifeCoop::GetSkillCvar(const skilldata_t& skillData, const char* pszSkillCvarName)
{
	//These cvars have overrides for multiplayer. - Solokiller
	if (strcmp(pszSkillCvarName, "sk_suitcharger") == 0 ||
		strcmp(pszSkillCvarName, "sk_plr_crowbar") == 0 ||
		strcmp(pszSkillCvarName, "sk_plr_9mm_bullet") == 0 ||
		strcmp(pszSkillCvarName, "sk_plr_357_bullet") == 0 ||
		strcmp(pszSkillCvarName, "sk_plr_9mmAR_bullet") == 0 ||
		strcmp(pszSkillCvarName, "sk_plr_9mmAR_grenade") == 0 ||
		strcmp(pszSkillCvarName, "sk_plr_buckshot") == 0 ||
		strcmp(pszSkillCvarName, "sk_plr_xbow_bolt_client") == 0 ||
		strcmp(pszSkillCvarName, "sk_plr_rpg") == 0 ||
		strcmp(pszSkillCvarName, "sk_plr_egon_wide") == 0 ||
		strcmp(pszSkillCvarName, "sk_plr_egon_narrow") == 0 ||
		strcmp(pszSkillCvarName, "sk_plr_hand_grenade") == 0 ||
		strcmp(pszSkillCvarName, "sk_plr_satchel") == 0 ||
		strcmp(pszSkillCvarName, "sk_plr_tripmine") == 0 ||
		strcmp(pszSkillCvarName, "sk_plr_hornet_dmg") == 0)
	{
		char szBuffer[64];

		//All overrides have _mp appended to their base name. - Solokiller
		const int Result = snprintf(szBuffer, sizeof(szBuffer), "%s_mp", pszSkillCvarName);

		return skilldata_t::GetSkillCvar(szBuffer, skillData.GetSkillLevel());
	}

	return CGameRules::GetSkillCvar(skillData, pszSkillCvarName);
}

//=========================================================
//=========================================================
void CBaseHalfLifeCoop::Think(void)
{
	g_VoiceGameMgr2.Update(gpGlobals->frametime);
}

//=========================================================
//=========================================================
bool CBaseHalfLifeCoop::IsMultiplayer() const
{
	return true;
}

//=========================================================
//=========================================================
bool CBaseHalfLifeCoop::IsDeathmatch() const
{
	return false;
}

//=========================================================
//=========================================================
bool CBaseHalfLifeCoop::IsCoOp() const
{
	return true;
}

//=========================================================
//=========================================================
bool CBaseHalfLifeCoop::FShouldSwitchWeapon(CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon)
{
	if (!pPlayer->m_pActiveItem)
	{
		// player doesn't have an active item!
		return true;
	}

	if (!pPlayer->m_pActiveItem->CanHolster())
	{
		// can't put away the active item.
		return false;
	}

	return true;
}

bool CBaseHalfLifeCoop::GetNextBestWeapon(CBasePlayer *pPlayer, CBasePlayerWeapon *pCurrentWeapon)
{
	CBasePlayerWeapon *pCheck;
	CBasePlayerWeapon *pBest;// this will be used in the event that we don't find a weapon in the same category.
	int iBestWeight;
	int i;

	iBestWeight = -1;// no weapon lower than -1 can be autoswitched to
	pBest = NULL;

	if (!pCurrentWeapon->CanHolster())
	{
		// can't put this gun away right now, so can't switch.
		return false;
	}

	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		pCheck = pPlayer->m_rgpPlayerItems[i];

		while (pCheck)
		{
			if (pCheck->iWeight() > -1 && pCheck->iWeight() == pCurrentWeapon->iWeight() && pCheck != pCurrentWeapon)
			{
				// this weapon is from the same category. 
				if (pCheck->CanDeploy())
				{
					if (pPlayer->SwitchWeapon(pCheck))
					{
						return true;
					}
				}
			}
			else if (pCheck->iWeight() > iBestWeight && pCheck != pCurrentWeapon)// don't reselect the weapon we're trying to get rid of
			{
				//ALERT ( at_console, "Considering %s\n", pCheck->GetClassname() );
				// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
				// that the player was using. This will end up leaving the player with his heaviest-weighted 
				// weapon. 
				if (pCheck->CanDeploy())
				{
					// if this weapon is useable, flag it as the best
					iBestWeight = pCheck->iWeight();
					pBest = pCheck;
				}
			}

			pCheck = pCheck->m_pNext;
		}
	}

	// if we make it here, we've checked all the weapons and found no useable 
	// weapon in the same catagory as the current weapon. 

	// if pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always 
	// at least get the crowbar, but ya never know.
	if (!pBest)
	{
		return false;
	}

	pPlayer->SwitchWeapon(pBest);

	return true;
}

//=========================================================
//=========================================================
bool CBaseHalfLifeCoop::ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[CCONNECT_REJECT_REASON_SIZE])
{
	g_VoiceGameMgr2.ClientConnected(pEntity);
	
	return true;
}

void CBaseHalfLifeCoop::UpdateGameMode(CBasePlayer *pPlayer)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgGameMode, NULL, pPlayer);
	WRITE_BYTE(0);  // game mode none
	MESSAGE_END();

	// Adiciono um sprite em cada landmark
	CBaseEntity* pLandmark = nullptr;

	while ((pLandmark = UTIL_FindEntityByClassname(pLandmark, "info_landmark")) != nullptr)
	{
		CSprite* pSprite1 = CSprite::SpriteCreate("sprites/changelevel.spr", pLandmark->GetAbsOrigin(), false);
		pSprite1->SetTransparency(kRenderTransAdd, 255, 255, 255, 150, kRenderFxFadeFast);
	}

	// Executo estados especiais nas entidades
	edict_t		*pEdict = g_engfuncs.pfnPEntityOfEntIndex(0);
	CBaseEntity *pEntity;
	char * remove_in_coop = (char*)CVAR_GET_STRING("remove_in_coop");
	char * nophysics_in_coop = (char*)CVAR_GET_STRING("nophysics_in_coop");
	char* tok1 = strtok(remove_in_coop, ";");
	char* tok2 = strtok(nophysics_in_coop, ";");
	int j, count_remove_in_coop = 0, count_nophysics_in_coop = 0;

	// Contar a quantidade de entidades a remover
	while (tok1 != NULL) {
		count_remove_in_coop++;
		tok1 = strtok(NULL, ";");
	}

	// Contar a quantidade de entidades a desativar a fisica
	while (tok2 != NULL) {
		count_nophysics_in_coop++;
		tok2 = strtok(NULL, ";");
	}

	if (count_remove_in_coop > 0 || count_nophysics_in_coop > 0)
	{
		for (int i = 1; i < gpGlobals->maxEntities; i++, pEdict++)
		{
			if (!pEdict)
				break;

			pEntity = CBaseEntity::Instance(pEdict);
			if (!pEntity)
				continue; // Essa verificacao em Util.cpp dentro de UTIL_MonstersInSphere() usa continue ao inves de break

			bool go_on = true;

			// Remover a entidade se ela estiver marcada como nao apropriada para o coop
			tok1 = remove_in_coop;
			for (j = 0; j < count_remove_in_coop; ++j) {
				if (strcmp(pEntity->GetTargetname(), tok1) == 0)
				{
					pEntity->SUB_Remove();
					go_on = false;

					break;
				}
				tok1 += strlen(tok1) + 1;
				tok1 += strspn(tok1, ";");
			}

			//  Deixar a entidade com transparencia e sem efeitos fisicos (sendo que ela continua a funcionar)
			tok2 = nophysics_in_coop;
			for (j = 0; j < count_nophysics_in_coop; ++j) {
				if (strcmp(pEntity->GetTargetname(), tok2) == 0)
				{
					DisablePhysics(pEntity);

					break;
				}
				tok2 += strlen(tok2) + 1;
				tok2 += strspn(tok2, ";");
			}
		}
	}
}

void CBaseHalfLifeCoop::InitHUD(CBasePlayer *pl)
{
	// notify other clients of player joining the game
	UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("%s se juntou a esta bosta\n",
		pl->HasNetName() ? pl->GetNetName() : "desconectado"));

	// team match?
	if (g_teamplay)
	{
		UTIL_LogPrintf("\"%s<%i><%s><%s>\" entrou na zona\n",
			pl->GetNetName(),
			UTIL_GetPlayerUserId(pl),
			UTIL_GetPlayerAuthId(pl),
			g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pl->edict()), "model"));
	}
	else
	{
		UTIL_LogPrintf("\"%s<%i><%s><%i>\" entrou com tudo!\n",
			pl->GetNetName(),
			UTIL_GetPlayerUserId(pl),
			UTIL_GetPlayerAuthId(pl),
			UTIL_GetPlayerUserId(pl));
	}

	UpdateGameMode(pl);

	// sending just one score makes the hud scoreboard active;  otherwise
	// it is just disabled for single play
	MESSAGE_BEGIN(MSG_ONE, gmsgScoreInfo, NULL, pl);
	WRITE_BYTE(pl->entindex());
	WRITE_SHORT(0);
	WRITE_SHORT(0);
	WRITE_SHORT(0);
	WRITE_SHORT(0);
	MESSAGE_END();

	SendMOTDToClient(pl);

	// loop through all active players and send their score info to the new client
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		// FIXME:  Probably don't need to cast this just to read m_iDeaths
		CBasePlayer *plr = UTIL_PlayerByIndex(i);

		if (plr)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgScoreInfo, NULL, pl);
			WRITE_BYTE(i);	// client number
			WRITE_SHORT(plr->GetFrags());
			WRITE_SHORT(plr->m_iDeaths);
			WRITE_SHORT(0);
			WRITE_SHORT(GetTeamIndex(plr->m_szTeamName) + 1);
			MESSAGE_END();
		}
	}

	// Rodo um comando de troca de nome caso seja necessario
	if (hu3ChangeNetName)
	{
		CBaseEntity *hu3Player = CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex(hu3CoopPlyIndex));
		char comando[35] = "name ";
		strcat(strcat(comando, hu3NetNewName), "\n");
		CLIENT_COMMAND(ENT(hu3Player), comando);
		hu3ChangeNetName = false;
	}
}

//=========================================================
//=========================================================
void CBaseHalfLifeCoop::ClientDisconnected(edict_t *pClient)
{
	if (pClient)
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pClient);

		if (pPlayer)
		{
			FireTargets("game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0);

			// team match?
			if (g_teamplay)
			{
				UTIL_LogPrintf("\"%s<%i><%s><%s>\" morreu de vez\n",
					pPlayer->GetNetName(),
					UTIL_GetPlayerUserId(pPlayer),
					UTIL_GetPlayerAuthId(pPlayer),
					g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model"));
			}
			else
			{
				UTIL_LogPrintf("\"%s<%i><%s><%i>\" foi roubado e fugiu\n",
					pPlayer->GetNetName(),
					UTIL_GetPlayerUserId(pPlayer),
					UTIL_GetPlayerAuthId(pPlayer),
					UTIL_GetPlayerUserId(pPlayer));
			}

			pPlayer->RemoveAllItems(true);// destroy all of the players weapons and items
		}
	}
}

//=========================================================
//=========================================================
float CBaseHalfLifeCoop::FlPlayerFallDamage(CBasePlayer *pPlayer)
{
	int iFallDamage = (int)falldamage.value;

	switch (iFallDamage)
	{
	case 1://progressive
		pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
		return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
		break;
	default:
	case 0:// fixed
		return 10;
		break;
	}
}

//=========================================================
//=========================================================
bool CBaseHalfLifeCoop::FPlayerCanTakeDamage(CBasePlayer *pPlayer, const CTakeDamageInfo& info)
{
	// Nao deixo matarem jogadores que estao aguardando o changelevel
	if (CoopPlyData[pPlayer->entindex()].waitingforchangelevel)
		return false;

	// Nao deixo morrerem jogadores que acabem dentro de players dando (re)spawn
	// OBS: codigo feio! Nao consegui de jeito nenhum usar UTIL_FindEntityInSphere para ver os arredores de players
	//      que acabam de nascer, simplesmente nao da certo.
	if (info.GetDamageTypes() == 0)
	{
		char* hu3Train = (char*)CVAR_GET_STRING("mp_hu3_trainspawnpoint");
		
		// Em mapas com func_tracktrain configurados, ativo a defesa contra dano generico automaticamente
		if (strcmp(hu3Train, "0") != 0)
		{
			DisablePhysics(pPlayer);

			return false;
		}
		else
		{
			// Vejo se tem spawnpoints por perto
			CBaseEntity *ent = CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex(0));

			while ((ent = UTIL_FindEntityInSphere(ent, pPlayer->GetAbsOrigin(), 128)) != nullptr)
			{
				if (strcmp(ent->GetClassname(), "info_player_start") == 0 || strcmp(ent->GetClassname(), "info_coop_start") == 0)
				{
					DisablePhysics(pPlayer);

					return false;
				}
			}
		}
	}

	/*
	// Se bem proximo desse player tiverem outros players mas em estado de spawn coop, protejo ele
	// Nota: UTIL_FindEntityInSphere nao consegue encontrar players no momento do primeiro spawn deles

	CBaseEntity *ent = CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex(0));
	
	while ((ent = UTIL_FindEntityInSphere(ent, pPlayer->GetAbsOrigin(), 128)) != nullptr)
	{
		if (strcmp(ent->GetClassname(), "player") == 0)
		{
			if (CoopPlyData[ent->entindex()].notSolidWait > gpGlobals->time)
			{
				DisablePhysics(pPlayer);

				return false;
			}
		}
	}
	*/

	return true;
}

//=========================================================
//=========================================================
void CBaseHalfLifeCoop::DisablePhysics(CBaseEntity * pEntity)
{
	if (pEntity->IsPlayer())
		CoopPlyData[pEntity->entindex()].notSolidWait = gpGlobals->time + SPAWNPROTECTIONTIME;

	pEntity->SetSolidType(SOLID_NOT);
	pEntity->SetTakeDamageMode(DAMAGE_NO);
	pEntity->SetRenderMode(kRenderTransAdd);
	pEntity->SetRenderAmount(230);
	const Vector vecColor = { 0.6f, 0.8f, 1.0f };
	pEntity->SetRenderColor(vecColor);
	pEntity->SetRenderFX(kRenderFxFadeFast);
}

//=========================================================
//=========================================================
void CBaseHalfLifeCoop::PlayerThink(CBasePlayer *pPlayer)
{
	// Apos respawn e de certo tempo, restaurar solidez de pessoas que nao estejam em area de changelevel
	if (!CoopPlyData[pPlayer->entindex()].waitingforchangelevel)
	{
		if (pPlayer->GetSolidType() == SOLID_NOT)
		{
			if (CoopPlyData[pPlayer->entindex()].notSolidWait < gpGlobals->time)
			{
				CBaseEntity *ent = CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex(0));

				while ((ent = UTIL_FindEntityInSphere(ent, pPlayer->GetAbsOrigin(), 64)) != nullptr)
				{
					//if (ent->IsPlayer()) // Sempre retorna falso
					if (strcmp(ent->GetClassname(), "player") == 0 && ent != pPlayer)
					{
						CoopPlyData[pPlayer->entindex()].notSolidWait = gpGlobals->time + SPAWNPROTECTIONTIME;

						return;
					}
				}

				pPlayer->SetSolidType(SOLID_SLIDEBOX);
				pPlayer->SetTakeDamageMode(DAMAGE_YES);
				pPlayer->SetRenderMode(kRenderNormal);
				
				return;
			}
		}
	}
}

//=========================================================
//=========================================================
void CBaseHalfLifeCoop::FixPlayerCrouchStuck(edict_t *pPlayer)
{
	TraceResult trace;

	// Mover ate 18 pixels para cima se o jogador estiver preso no chao
	for (int i = 0; i < 18; i++)
	{
		UTIL_TraceHull(pPlayer->v.origin, pPlayer->v.origin, dont_ignore_monsters, Hull::HEAD, pPlayer, &trace);
		if (trace.fStartSolid)
			pPlayer->v.origin.z++;
		else
			break;
	}
}

//=========================================================
//=========================================================
void CBaseHalfLifeCoop::PlayerSpawn(CBasePlayer *pPlayer)
{
	// Pego uma classe acima o objeto jogador
	CBaseEntity *pPlayer2 = NULL;
	pPlayer2 = (CBaseEntity *)pPlayer;

	// Pego o indice do jogador
	int i = pPlayer2->entindex();

	// Configuramos o nome do jogador
	// O retorno dessa funcao eh o indice desse na nossa tabela coop
	if (!SetPlayerName(pPlayer2))
		CoopPlyData[i].newplayer = true;

	// Deixar o jogador passavel e com efeitos durante algum tempo
	DisablePhysics(pPlayer2);

	// O ponto de spawn
	CBaseEntity* spawnPoint = nullptr;

	// A posicao correta do jogador no mundo
	Vector absPos(0,0,0);

	// Tenta pegar o ponto de spawn em relacao a algum trem com spawn consertado para coop
	char* hu3Train = (char*)CVAR_GET_STRING("mp_hu3_trainspawnpoint");
	if (strcmp(hu3Train, "0") != 0)
	{
		CBaseEntity* temp = UTIL_FindEntityByString(NULL, "targetname", hu3Train);
		if (temp)
		{
			absPos = Vector (temp->GetAbsOrigin().x, temp->GetAbsOrigin().y, temp->GetAbsOrigin().z + 75);
		}
	}

	// Tenta pegar o ponto de spawn em relacao a algum info_player_coop disponivel
	if (absPos == Vector(0,0,0))
	{
		while ((spawnPoint = UTIL_FindEntityByClassname(spawnPoint, "info_player_coop")) != nullptr)
		{
			absPos = spawnPoint->GetAbsOrigin() + Vector(CoopPlyData[i].relPos.x, CoopPlyData[i].relPos.y, 0);
			break;
		}
	}

	// Configuramos o jogador no caso dele NAO ser novo no server (ja ter passado por changelevel)
	if (!CoopPlyData[i].newplayer)
	{
		// Tentar encontrar o local de spawn em relacao ao landmark
		if (absPos == Vector(0, 0, 0))
		{
			while ((spawnPoint = UTIL_FindEntityByTargetname(spawnPoint, hu3LandmarkName)) != nullptr)
			{
				if (spawnPoint->ClassnameIs("info_landmark"))
				{
					absPos = spawnPoint->GetAbsOrigin() + CoopPlyData[i].relPos;
					break;
				}
			}
		}

		// Carregar as informacoes
		if (spawnPoint)
		{
			// Propriedades de posicionamento
			pPlayer2->pev->v_angle = CoopPlyData[i].v_angle;
			pPlayer2->pev->velocity = CoopPlyData[i].velocity;
			pPlayer2->pev->angles = CoopPlyData[i].angles;
			pPlayer2->pev->punchangle = CoopPlyData[i].punchangle;
			pPlayer2->pev->fixangle = CoopPlyData[i].fixangle;
			pPlayer2->pev->flFallVelocity = CoopPlyData[i].flFallVelocity;
			if (CoopPlyData[i].bInDuck) // Agachamento. Meio bugado mas funciona...
			{
				pPlayer2->pev->flags = FL_DUCKING;
				pPlayer2->pev->button = IN_DUCK;
				FixPlayerCrouchStuck(pPlayer2->edict());
				pPlayer2->pev->view_ofs[2] = 3;
			}

			// Reparacao de estados gerais
			pPlayer2->pev->deadflag = CoopPlyData[i].deadflag;
			pPlayer2->pev->health = CoopPlyData[i].health;
			pPlayer2->pev->armorvalue = CoopPlyData[i].armorvalue;
			pPlayer2->pev->team = CoopPlyData[i].team;
			pPlayer2->pev->frags = CoopPlyData[i].frags;
			pPlayer2->pev->weapons = CoopPlyData[i].weapons;
			if (CoopPlyData[i].flashlight)
				pPlayer->FlashlightTurnOn();

			// Restaurar godmode e notarget
			if (CoopPlyData[i].godmode)
				CoopPlyData[i].godmode ? pPlayer2->pev->flags |= FL_GODMODE : pPlayer2->pev->flags &= ~FL_GODMODE;
			if (CoopPlyData[i].notarget)
				CoopPlyData[i].notarget ? pPlayer2->pev->flags |= FL_NOTARGET : pPlayer2->pev->flags &= ~FL_NOTARGET;

			// Restaurar noclip
			if (CoopPlyData[i].noclip)
					pPlayer2->SetMoveType(MOVETYPE_NOCLIP);

			// Carregar armas e municoes
			LoadPlayerItems(pPlayer, &CoopPlyData[i]);
		}
	}

	// Liberar a checagem de changelevel 
	if (CoopPlyData[i].waitingforchangelevel)
		CoopPlyData[i].waitingforchangelevel = false;

	// No ultimo dos casos, pega o ponto de spawn em relacao ao info_player_start
	if (absPos == Vector(0, 0, 0))
	{
		while ((spawnPoint = UTIL_FindEntityByClassname(spawnPoint, "info_player_start")) != nullptr)
		{
			absPos = spawnPoint->GetAbsOrigin();
			break;
		}
	}

	// Posicionar jogador no mundo
	pPlayer2->pev->origin = absPos;
}

//=========================================================
//=========================================================
bool CBaseHalfLifeCoop::SetPlayerName(CBaseEntity *pPlayer)
{
	// Vamos validar o nome do jogador. Nao pode ser "Player" e tem que ser unico
	// retorno: true = nome encontrado na tabela coop; false = nao encontrado
	// Obs: nunca existem nomes repetidos na nossa tabela coop

	char *name, *aux;
	char newName[30] = "";
	bool needsToGenerateNewName = false;
	const int TOTALNAMES = 125;
	int i = 1;

	// Pego o nome atual
	name = (char*) STRING(pPlayer->pev->netname);

	// Comeco um loop ate que o nome esteja aprovado
	while (true)
	{
		// O nome do jogador nao pode ser "Player"
		if (strcmp(name, "Player") == 0 && strcmp(newName, "") == 0)
		{
			needsToGenerateNewName = true;
		}
		else
		{
			// Seleciono o nome correto a ser verificado na funcao
			if (strcmp(newName, "") != 0)
				aux = newName; // Caso haja um novo nome proposto
			else
				aux = name; // Caso ainda estejamos verificando a validade do nome registrado

			// Inicio um loop pelos nomes da tabela coop
			while (CoopPlyData[i].pName)
			{
				// Verifico se o nome em questao eh igual ao nome que queremos validar
				if (strcmp(aux, CoopPlyData[i].pName) == 0)
				{
					// Se ainda nao houver um novo nome proposto, entao o nome registrado eh valido. Vou retornar o indice
					if (strcmp(newName, "") == 0)
					{
						return true;
					}
					// Caso contrario o jogador encontrado eh um jogador qualquer e precisamos gerar outro novo nome
					else
					{
						needsToGenerateNewName = true;
					}

					break;
				}
				i++;
			}
		}

		// Geracao de novos nomes...
		if (needsToGenerateNewName)
		{
			char names[TOTALNAMES][25] = {
				"Demente", // 0

				"Jamelao", "Jararaca", "Wallita", "Pereba", "Gandalf",
				"Hipoglos", "Gonorreia", "Gordon", "UNO1.0", "Jabolane",  // 10

				"GMBR", "Estupido", "Jirafa", "Gayzing", "Ceboso",
				"Cajado", "Galudo", "Bebum", "Jiromba", "Comulixo", // 20

				"Lalau", "Mesoclise", "Fuscazul", "FiDiKenga", "Birinbau",
				"Lampiao", "Danoninho", "Sapatao", "Gugu", "Platao", // 30

				"Amenopausa", "Caganeira", "Diarreia", "Televisao", "Amendoim",
				"Ovocito", "Polichop", "Gulosa", "Suvaco", "Bilau", // 40

				"Mindingu", "Gatuno", "Loirinha18", "Bostolhozo", "Fezaldo",
				"Trivago", "Resto", "Ganancia", "Niobio", "Coco", // 50

				"Arrombado", "Piperodaptiloh", "Pistolao", "Papaco", "Teocu",
				"Teopai", "Tuamae", "Cagapau", "Bizonho", "Pipistrelo", // 60

				"Cegueta", "Metiolate", "Noob", "XXXX", "Ratazana",
				"Xoraum", "Isk8tero", "Acreman", "Sr.Peitos", "Sr.Mijo", // 70

				"Verruga", "Mercedes", "Otahrio", "Gastrite", "MC_Menor",
				"Zigoto", "Pesadao", "Bozo", "Cretino", "Folgado", // 80

				"CuDeFoca", "Cotoco", "Sem_pau", "Vigario", "1000otwo",
				"Gargarejo", "Kitinet", "Ser.Veja", "Dollynetx", "Cafetao", // 90

				"Entalado", "Baiano", "Bundao", "Hostil", "Maluko",
				"Leitinho","QuerAlho", "Castigador", "CHUTEME", "Bucetaldx", // 100

				"Monosselha", "Zarolha", "Maritmo", "Sr.dos_anais", "Poderaldo",
				"Inutil", "Virjao", "Aleijadinho", "Parideira", "Motumbo", // 110

				"Xalalau", "NickMBR", "Jonks", "M4n0Cr4zy", "Pepeu",
				"R4t0", "DarteVerder", "Zuzu", "Pavomba", "Gaben" // 120
			};

			// Pego um nome aleatoriamente
			strcpy(newName, names[RANDOM_LONG(0, TOTALNAMES)]);

			// Reinicio as variaveis
			needsToGenerateNewName = false;
			i = 1;

			// Volto para o loop
		}
		// Finalizando a execucao, o nome eh valido!
		// Obs: o codigo so chega aqui caso haja um nome novo a ser aplicado ou caso nao exista referencias
		// desse nome na tabela coop (jogador novo com nome valido).
		else
		{
			// Pego o indice do jogador
			i = pPlayer->entindex();

			// Se houver um novo nome a ser aceito, temos que passar isso adiante
			if (strcmp(newName, "") != 0)
			{
				// Marco as variaveis especiais do CHu3LifeCoop.cpp para rodar no CBasePlayer.server.game.cpp 
				hu3CoopPlyIndex = i;
				strcpy(hu3NetNewName, newName);
				hu3ChangeNetName = true;
			}

			// Adiciono o jogador na nossa tabela coop
			CoopPlyData[i].pName = newName;

			return false;
		}
	}
}

//=========================================================
//=========================================================
void CBaseHalfLifeCoop::LoadPlayerItems(CBasePlayer *pPlayer, playerCoopSaveRestore* CoopPlyDataIn)
{
	// Restauracao das armas e municoes

	int i = 0;

	// As armas salvas serao verificadas uma a uma
	while (strcmp((*CoopPlyDataIn).keepweapons[i].name, "-1") != 0)
	{
		// Readiciono a quantidade da municao 1 no jogador
		if (strcmp((*CoopPlyDataIn).keepweapons[i].type1, "-1") != 0)
			// jogador deve estar zerado dessa municao, caso contrario ela ja foi restaurada por outra arma
			if (pPlayer->m_rgAmmo[pPlayer->GetAmmoIndex((*CoopPlyDataIn).keepweapons[i].type1)] == 0)
			{
				// HACK!!!! AJUSTE DA MUNICAO DURANTE O MOMENTO DO CHANGELEVEL
				// Estao ocorrendo somas muito loucas na recarga aqui, nao sei de onde elas vem...
				// Definitavamente eu nao sou o culpado disso, estou resolvendo na mao! Funciona...
				if ((*CoopPlyDataIn).changinglevel)
				{
					if (strcmp((*CoopPlyDataIn).keepweapons[i].type1, "buckshot") == 0)
						(*CoopPlyDataIn).keepweapons[i].amountammo1 = (*CoopPlyDataIn).keepweapons[i].amountammo1 - 4;
					else if (strcmp((*CoopPlyDataIn).keepweapons[i].type1, "rockets") == 0)
						(*CoopPlyDataIn).keepweapons[i].amountammo1 = (*CoopPlyDataIn).keepweapons[i].amountammo1 - 1;
					else if (strcmp((*CoopPlyDataIn).keepweapons[i].type1, "uranium") == 0)
						(*CoopPlyDataIn).keepweapons[i].amountammo1 = (*CoopPlyDataIn).keepweapons[i].amountammo1 - 80;
					else if (strcmp((*CoopPlyDataIn).keepweapons[i].type1, "Hand Grenade") == 0)
						(*CoopPlyDataIn).keepweapons[i].amountammo1 = (*CoopPlyDataIn).keepweapons[i].amountammo1 - 5;
					else if (strcmp((*CoopPlyDataIn).keepweapons[i].type1, "Trip Mine") == 0)
						(*CoopPlyDataIn).keepweapons[i].amountammo1 = (*CoopPlyDataIn).keepweapons[i].amountammo1 - 1;
					else if (strcmp((*CoopPlyDataIn).keepweapons[i].type1, "Satchel Charge") == 0)
						(*CoopPlyDataIn).keepweapons[i].amountammo1 = (*CoopPlyDataIn).keepweapons[i].amountammo1 - 1;
					else if (strcmp((*CoopPlyDataIn).keepweapons[i].type1, "Snarks") == 0)
						(*CoopPlyDataIn).keepweapons[i].amountammo1 = (*CoopPlyDataIn).keepweapons[i].amountammo1 - 5;
				}
				//--

				pPlayer->GiveAmmo((*CoopPlyDataIn).keepweapons[i].amountammo1, (*CoopPlyDataIn).keepweapons[i].type1);
			}

		//  Readiciono a quantidade da municao 2 no jogador
		if (strcmp((*CoopPlyDataIn).keepweapons[i].type2, "-1") != 0)
			// jogador deve estar zerado dessa municao, caso contrario ela ja foi restaurada por outra arma
			if (pPlayer->m_rgAmmo[pPlayer->GetAmmoIndex((*CoopPlyDataIn).keepweapons[i].type2)] == 0)
				pPlayer->GiveAmmo((*CoopPlyDataIn).keepweapons[i].amountammo2, (*CoopPlyDataIn).keepweapons[i].type2);

		// Restauro a arma
		CBaseEntity* pItem = pPlayer->GiveNamedItem((*CoopPlyDataIn).keepweapons[i].name);

		// Readiciono a municao ativa da arma
		CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon*)pItem;
		pWeapon->m_iClip = (*CoopPlyDataIn).keepweapons[i].currentammo;

		// Print maroto de testes
		//ALERT(at_console, "%s: %s >> %d ||| %s >> %d\n", (*CoopPlyDataIn).keepweapons[i].name, (*CoopPlyDataIn).keepweapons[i].type1, (*CoopPlyDataIn).keepweapons[i].amountammo1, (*CoopPlyDataIn).keepweapons[i].type2, (*CoopPlyDataIn).keepweapons[i].amountammo2);

		i++;
	}

	// Desligo o hack logo acima com essa variavel, ele so vale no changelevel!
	if ((*CoopPlyDataIn).changinglevel)
		(*CoopPlyDataIn).changinglevel = false;
}

//=========================================================
//=========================================================
void CBaseHalfLifeCoop::SavePlayerItems(CBasePlayer *pPlayer, playerCoopSaveRestore* CoopPlyDataIn)
{
	// Salvar armas e municao antes de um changelevel
	// Funcao auxiliar

	CBasePlayerWeapon *pWeapon;
	int i, j = 0;

	// Fazer uma lista vazia caso nao exista uma arma carregada
	if (!pPlayer->m_pActiveItem)
	{
		strcpy((*CoopPlyDataIn).keepweapons[0].name, "-1");
		return;
	}

	// Vamos salvar as armas. O limite eh 64.
	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		// Tenta pegar a primeira arma do slot (10 slots)
		pWeapon = pPlayer->m_rgpPlayerItems[i];
		// Se existir a primeira arma, vamos salva-la e seguir fazendo o mesmo para as proximas caso estas existam
		while (pWeapon)
		{
			// Se a arma em questao estiver ativa nos devemos pula-la (ela precisa entrar por ultimo)
			if (pPlayer->m_pActiveItem == pWeapon)
			{
				pWeapon = pWeapon->m_pNext;
				continue;
			}

			// Salva as infos da arma
			SavePlayerItemsAux(pPlayer, CoopPlyDataIn, pWeapon, j);

			// Print maroto de testes
			//ALERT(at_console, "%s: %s >> %d ||| %s >> %d\n", (*CoopPlyDataIn).keepweapons[j].name, (*CoopPlyDataIn).keepweapons[j].type1, (*CoopPlyDataIn).keepweapons[j].amountammo1, (*CoopPlyDataIn).keepweapons[j].type2, (*CoopPlyDataIn).keepweapons[j].amountammo2);

			// J eh importante pois pulamos posicoes. Nao usar i
			j++;

			// pWeapon eh apontado para a proxima arma
			pWeapon = pWeapon->m_pNext;
		}
	}

	// Adicionamos a arma ativa no ultimo slot da lista (vai fazer ela carregar corretamente apos o changelevel)
	SavePlayerItemsAux(pPlayer, CoopPlyDataIn, pPlayer->m_pActiveItem, j);

	// Lista termina no "-1"
	strcpy((*CoopPlyDataIn).keepweapons[j + 1].name, "-1");
}

//=========================================================
//=========================================================
// Salva as infos da arma
void CBaseHalfLifeCoop::SavePlayerItemsAux(CBasePlayer *pPlayer, playerCoopSaveRestore* CoopPlyDataIn, CBasePlayerWeapon *pWeapon, int j)
{
	// Funcao auxiliar da funcao auxiliar

	// Nome da arma
	strcpy((*CoopPlyDataIn).keepweapons[j].name, pWeapon->pszName());
	// Quantidade da municao 1 em uso
	(*CoopPlyDataIn).keepweapons[j].currentammo = pWeapon->m_iClip;
	// Se tiver municao 1 em cartucho
	if (pWeapon->pszAmmo1())
	{
		// Tipo da municao 1
		strcpy((*CoopPlyDataIn).keepweapons[j].type1, pWeapon->pszAmmo1());
		// Quantidade da municao 1 em cartucho
		(*CoopPlyDataIn).keepweapons[j].amountammo1 = pPlayer->m_rgAmmo[pPlayer->GetAmmoIndex(pWeapon->pszAmmo1())];
	}
	else
	{
		// Caso contrario marco a quantidade da municao 1 como -1
		strcpy((*CoopPlyDataIn).keepweapons[j].type1, "-1");
		(*CoopPlyDataIn).keepweapons[j].amountammo1 = -1;
	}
	// Se tiver municao 2 em cartucho
	if (pWeapon->pszAmmo2())
	{
		// Tipo da municao 2
		strcpy((*CoopPlyDataIn).keepweapons[j].type2, pWeapon->pszAmmo2());
		// Quantidade da municao 2 em cartucho
		(*CoopPlyDataIn).keepweapons[j].amountammo2 = pPlayer->m_rgAmmo[pPlayer->GetAmmoIndex(pWeapon->pszAmmo2())];
	}
	else
	{
		// Caso contrario marco a quantidade da municao 2 como -1
		strcpy((*CoopPlyDataIn).keepweapons[j].type2, "-1");
		(*CoopPlyDataIn).keepweapons[j].amountammo2 = -1;
	}
}

//=========================================================
//=========================================================
void CBaseHalfLifeCoop::ChangeLevelCoop(CBaseEntity* pLandmark, char* m_szLandmarkName, char* st_szNextMap)
{
	CBaseEntity *hu3Player;
	int i = 0;

	// Verifica se todos os jogadores esta dentro de um mesmo changelevel e trava eles
	bool releaseChangelevel = true;

	while ((hu3Player = CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex(i))) != nullptr)
	{
		if (InTransitionVolume(hu3Player, m_szLandmarkName))
		{
			// Prendo os jogadores que ja estao em posicao valida, os deixo passaveis e taco efeitos
			if (!CoopPlyData[hu3Player->entindex()].waitingforchangelevel)
			{
				CBasePlayer* hu3Player2 = (CBasePlayer*)hu3Player;

				if (!hu3Player2->GetFlags().Any(FL_FROZEN))
					hu3Player2->EnableControl(false);

				DisablePhysics(hu3Player2);

				CoopPlyData[hu3Player->entindex()].waitingforchangelevel = true;
			}
		}
		else
		{
			releaseChangelevel = false;
		}

		i++;
	}

	if (!releaseChangelevel)
		return;

	// Reseto a deteccao do trigger_changelevel
	hu3CorrectChangelevelTrigger = 0;

	// Reseto o comando mp_hu3_trainspawnpoint
	char* hTarget = (char*)CVAR_GET_STRING("mp_hu3_trainspawnpoint");
	if (strcmp(hTarget, "0") != 0)
		CVAR_SET_STRING("mp_hu3_trainspawnpoint", "0");

	// Invalido a tabela de players coop atual atribuindo aos campos de nome usados um nome impossivel de existir
	// No client MAX_PLAYERS eh 64...
	for (i; i <= 64; i++)
		if (CoopPlyData[i].pName)
			CoopPlyData[i].pName = "Player";

	// Salvo o nome do landmark (sera usado no proximo mapa)
	strcpy(hu3LandmarkName, m_szLandmarkName);

	// Preencho a tabela de infos dos players novamente
	i = 1; // Tem que ser 1 para funcionar na funcao abaixo!
	while ((hu3Player = CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex(i))) != nullptr)
	{
		// Deixo o hu3Player pronto como hu3Player2 em CBasePlayer, uma classe abaixo
		CBasePlayer *hu3Player2 = NULL;
		hu3Player2 = (CBasePlayer *)hu3Player;

		// Calculo a distancia do player ate o landmark
		Vector absPos = pLandmark->GetAbsOrigin();
		Vector plyPos = hu3Player->pev->origin;
		Vector relPos;

		relPos = plyPos - absPos;

		// Vejo se o jogador esta abaixado
		bool inDuck = false;
		if (hu3Player->pev->flags & FL_DUCKING)
			inDuck = true;

		// Vejo se a lanterna esta ligada
		bool flashlightState = false;
		if (hu3Player2->FlashlightIsOn())
			flashlightState = true;

		// Verifico o godmode
		bool godmodeState = false;
		if (hu3Player->pev->flags & FL_GODMODE)
			godmodeState = true;

		// Verifico o notarget
		bool notargetState = false;
		if (hu3Player->pev->flags & FL_NOTARGET)
			notargetState = true;

		// Verifico o noclip
		bool noclipState = false;
		if (hu3Player->GetMoveType() == MOVETYPE_NOCLIP)
			noclipState = true;

		// Salvo as infos gerais
		CoopPlyData[i].pName = (char*)hu3Player->GetNetName();
		CoopPlyData[i].relPos = relPos;
		CoopPlyData[i].v_angle = hu3Player->pev->v_angle;
		CoopPlyData[i].velocity = hu3Player->pev->velocity;
		CoopPlyData[i].angles = hu3Player->pev->angles;
		CoopPlyData[i].punchangle = hu3Player->pev->punchangle;
		CoopPlyData[i].deadflag = hu3Player->pev->deadflag;
		CoopPlyData[i].fixangle = hu3Player->pev->fixangle;
		CoopPlyData[i].flFallVelocity = hu3Player->pev->flFallVelocity;
		CoopPlyData[i].bInDuck = inDuck;
		CoopPlyData[i].flashlight = flashlightState;
		CoopPlyData[i].team = hu3Player->pev->team;
		CoopPlyData[i].frags = hu3Player->pev->frags;
		CoopPlyData[i].health = hu3Player->pev->health;
		CoopPlyData[i].armorvalue = hu3Player->pev->armorvalue;
		CoopPlyData[i].weapons = hu3Player->pev->weapons;
		CoopPlyData[i].newplayer = false;
		CoopPlyData[i].changinglevel = true;
		CoopPlyData[i].godmode = godmodeState;
		CoopPlyData[i].notarget = notargetState;
		CoopPlyData[i].noclip = noclipState;
		CoopPlyData[i].respawncommands = true;
		CoopPlyData[i].waitingforchangelevel = true;

		// Salvo as infos de municao e armas
		SavePlayerItems(hu3Player2, &CoopPlyData[i]);

		// Proximo jogador...
		i++;
	}

	// Agora comeco o processo de troca de mapa
	char comando[30] = "changelevel ";

	strcat(strcat(comando, st_szNextMap), ";");
	SERVER_COMMAND(comando);
}

//=========================================================
//=========================================================
// Verificar se jogador esta dentro da area do trigger_changelevel
bool CBaseHalfLifeCoop::InTransitionVolume(CBaseEntity *pEntity, char *pVolumeName)
{
	CBaseEntity* pChangelevel = nullptr;
	// Temos os valores 0, 1 ou 2 para j:
	// 1 = o primeiro trigger_changelevel encontrado no mapa;
	// 2 = o segundo trigger_changelevel encontrado no mapa;
	// 0 = ainda nao foi feita nenhuma verificacao nesse mapa.
	int j = 1;

	while ((pChangelevel = UTIL_FindEntityByClassname(pChangelevel, "trigger_changelevel")) != nullptr)
	{
		// Posicao DENTRO de uma area de troca
		if (pChangelevel->Intersects(pEntity))
		{
			if (hu3CorrectChangelevelTrigger != 0)
			{
				// Area DIFERENTE a do primeiro jogador analisado
				if (hu3CorrectChangelevelTrigger != j)
					return false;
			}
			else
			{
				hu3CorrectChangelevelTrigger = j;
			}
			// Area IGUAL a do primeiro jogador analisado
			return true;
		}
		j++;
	}

	// Posicao FORA das duas areas de troca
	return false;
}

//=========================================================
//=========================================================
bool CBaseHalfLifeCoop::FPlayerCanRespawn(CBasePlayer *pPlayer)
{
	return true;
}

//=========================================================
//=========================================================
float CBaseHalfLifeCoop::FlPlayerSpawnTime(CBasePlayer *pPlayer)
{
	return gpGlobals->time; // Imediatamente
}

bool CBaseHalfLifeCoop::AllowAutoTargetCrosshair()
{
	return (aimcrosshair.value != 0);
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CBaseHalfLifeCoop::IPointsForKill(CBasePlayer *pAttacker, CBasePlayer *pKilled)
{
	return 1;
}

//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CBaseHalfLifeCoop::PlayerKilled(CBasePlayer* pVictim, const CTakeDamageInfo& info)
{
	auto pKiller = info.GetAttacker();
	//auto pInflictor = info.GetInflictor();

	ASSERT(pKiller);
	//ASSERT(pInflictor);

	DeathNotice(pVictim, info);

	pVictim->m_iDeaths += 1;

	FireTargets("game_playerdie", pVictim, pVictim, USE_TOGGLE, 0);

	CBasePlayer* const peKiller = pKiller && pKiller->IsPlayer() ? (CBasePlayer*)pKiller : nullptr;

	if (pVictim == pKiller)
	{
		// killed self
		pKiller->SetFrags(pKiller->GetFrags() - 1);
	}
	else if (peKiller)
	{
		// if a player dies in a deathmatch game and the killer is a client, award the killer some points
		peKiller->SetFrags(peKiller->GetFrags() + IPointsForKill(peKiller, pVictim));

		FireTargets("game_playerkill", peKiller, peKiller, USE_TOGGLE, 0);
	}
	else
	{
		// killed by the world
		pKiller->SetFrags(pKiller->GetFrags() - 1);
	}

	// update the scores
	// killed scores
	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
	WRITE_BYTE(pVictim->entindex());
	WRITE_SHORT(pVictim->GetFrags());
	WRITE_SHORT(pVictim->m_iDeaths);
	WRITE_SHORT(0);
	WRITE_SHORT(GetTeamIndex(pVictim->m_szTeamName) + 1);
	MESSAGE_END();

	// killers score, if it's a player
	if (peKiller)
	{
		MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
		WRITE_BYTE(peKiller->entindex());
		WRITE_SHORT(peKiller->GetFrags());
		WRITE_SHORT(peKiller->m_iDeaths);
		WRITE_SHORT(0);
		WRITE_SHORT(GetTeamIndex(peKiller->m_szTeamName) + 1);
		MESSAGE_END();

		// let the killer paint another decal as soon as he'd like.
		peKiller->m_flNextDecalTime = gpGlobals->time;
	}

	if (pVictim->HasNamedPlayerItem("weapon_satchel"))
	{
		DeactivateSatchels(pVictim, SatchelAction::RELEASE);
	}
}

//=========================================================
// Deathnotice. 
//=========================================================
void CBaseHalfLifeCoop::DeathNotice(CBasePlayer* pVictim, const CTakeDamageInfo& info)
{
	auto pKiller = info.GetAttacker();
	auto pInflictor = info.GetInflictor();

	// Work out what killed the player, and send a message to all clients about it
	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_index = 0;

	//TODO clean this up - Solokiller

	// Hack to fix name change
	const char* const tau = "tau_cannon";
	const char* const gluon = "gluon gun";

	if (pKiller->GetFlags().Any(FL_CLIENT))
	{
		killer_index = pKiller->entindex();

		if (pInflictor)
		{
			if (pInflictor == pKiller)
			{
				// If the inflictor is the killer,  then it must be their current weapon doing the damage
				CBasePlayer *pPlayer = (CBasePlayer*)pKiller;

				if (pPlayer->m_pActiveItem)
				{
					killer_weapon_name = pPlayer->m_pActiveItem->pszName();
				}
			}
			else
			{
				killer_weapon_name = pInflictor->GetClassname();  // it's just that easy
			}
		}
	}
	else
	{
		killer_weapon_name = pInflictor->GetClassname();
	}

	// strip the monster_* or weapon_* from the inflictor's classname
	if (strncmp(killer_weapon_name, "weapon_", 7) == 0)
		killer_weapon_name += 7;
	else if (strncmp(killer_weapon_name, "monster_", 8) == 0)
		killer_weapon_name += 8;
	else if (strncmp(killer_weapon_name, "func_", 5) == 0)
		killer_weapon_name += 5;

	MESSAGE_BEGIN(MSG_ALL, gmsgDeathMsg);
	WRITE_BYTE(killer_index);						// the killer
	WRITE_BYTE(pVictim->entindex());		// the victim
	WRITE_STRING(killer_weapon_name);		// what they were killed by (should this be a string?)
	MESSAGE_END();

	// replace the code names with the 'real' names
	if (!strcmp(killer_weapon_name, "egon"))
		killer_weapon_name = gluon;
	else if (!strcmp(killer_weapon_name, "gauss"))
		killer_weapon_name = tau;

	if (pVictim == pKiller)
	{
		// killed self

		// team match?
		if (g_teamplay)
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" cometeu suicidio com um(a) \"%s\"\n",
				pVictim->GetNetName(),
				UTIL_GetPlayerUserId(pVictim),
				UTIL_GetPlayerAuthId(pVictim),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pVictim->edict()), "model"),
				killer_weapon_name);
		}
		else
		{
			UTIL_LogPrintf("\"%s<%i><%s><%i>\" cometeu suicidio com um(a) \"%s\"\n",
				pVictim->GetNetName(),
				UTIL_GetPlayerUserId(pVictim),
				UTIL_GetPlayerAuthId(pVictim),
				UTIL_GetPlayerUserId(pVictim),
				killer_weapon_name);
		}
	}
	else if (pKiller->GetFlags().Any(FL_CLIENT))
	{
		// team match?
		if (g_teamplay)
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" matou \"%s<%i><%s><%s>\" com um(a) \"%s\"\n",
				pKiller->GetNetName(),
				UTIL_GetPlayerUserId(pKiller),
				UTIL_GetPlayerAuthId(pKiller),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(ENT(pKiller)), "model"),
				pVictim->GetNetName(),
				UTIL_GetPlayerUserId(pVictim),
				UTIL_GetPlayerAuthId(pVictim),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pVictim->edict()), "model"),
				killer_weapon_name);
		}
		else
		{
			UTIL_LogPrintf("\"%s<%i><%s><%i>\" matou \"%s<%i><%s><%i>\" com um(a) \"%s\"\n",
				pKiller->GetNetName(),
				UTIL_GetPlayerUserId(pKiller),
				UTIL_GetPlayerAuthId(pKiller),
				UTIL_GetPlayerUserId(pKiller),
				pVictim->GetNetName(),
				UTIL_GetPlayerUserId(pVictim),
				UTIL_GetPlayerAuthId(pVictim),
				UTIL_GetPlayerUserId(pVictim),
				killer_weapon_name);
		}
	}
	else
	{
		// killed by the world

		// team match?
		if (g_teamplay)
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" cometeu suicidio com um(a) \"%s\" (world)\n",
				pVictim->GetNetName(),
				UTIL_GetPlayerUserId(pVictim),
				UTIL_GetPlayerAuthId(pVictim),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pVictim->edict()), "model"),
				killer_weapon_name);
		}
		else
		{
			UTIL_LogPrintf("\"%s<%i><%s><%i>\" cometeu suicidio com um(a) \"%s\" (world)\n",
				pVictim->GetNetName(),
				UTIL_GetPlayerUserId(pVictim),
				UTIL_GetPlayerAuthId(pVictim),
				UTIL_GetPlayerUserId(pVictim),
				killer_weapon_name);
		}
	}

	MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
	WRITE_BYTE(9);	// command length in bytes
	WRITE_BYTE(DRC_CMD_EVENT);	// player killed
	WRITE_SHORT(pVictim->entindex());	// index number of primary entity
	if (pInflictor)
		WRITE_SHORT(pInflictor->entindex());	// index number of secondary entity
	else
		WRITE_SHORT(pKiller->entindex());	// index number of secondary entity
	WRITE_LONG(7 | DRC_FLAG_DRAMATIC);   // eventflags (priority and flags)
	MESSAGE_END();

	// Print a standard message

	//TODO: add a cvar to control this? - Solokiller

	char szBuffer[512];

	if (pKiller->GetFlags().Any(FL_MONSTER))
	{
		// killed by a monster
		snprintf(szBuffer, sizeof(szBuffer), "%s foi morto por um monstro.\n", pVictim->GetNetName());
	}
	else if (pKiller == pVictim)
	{
		snprintf(szBuffer, sizeof(szBuffer), "%s cometeu suicidio.\n", pVictim->GetNetName());
	}
	else if (pKiller->GetFlags().Any(FL_CLIENT))
	{
		snprintf(szBuffer, sizeof(szBuffer), "%s : %s : %s\n", pKiller->GetNetName(), killer_weapon_name, pVictim->GetNetName());
	}
	else if (pKiller->ClassnameIs("worldspawn"))
	{
		snprintf(szBuffer, sizeof(szBuffer), "%s caiu ou se afogou ou qualquer coisa.\n", pVictim->GetNetName());
	}
	else if (pKiller->GetSolidType() == SOLID_BSP)
	{
		snprintf(szBuffer, sizeof(szBuffer), "%s foi assaltado.\n", pVictim->GetNetName());
	}
	else
	{
		snprintf(szBuffer, sizeof(szBuffer), "%s morreu misteriosamente.\n", pVictim->GetNetName());
	}

	UTIL_ClientPrintAll(HUD_PRINTNOTIFY, szBuffer);
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CBaseHalfLifeCoop::PlayerGotWeapon(CBasePlayer *pPlayer, CBasePlayerWeapon *pWeapon)
{
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CBaseHalfLifeCoop::FlWeaponRespawnTime(CBasePlayerWeapon *pWeapon)
{
	return gpGlobals->time; // Imediatamente
}

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================

// AJUSTAR ISSO AQUI:

float CBaseHalfLifeCoop::FlWeaponTryRespawn(CBasePlayerWeapon *pWeapon)
{
	if (pWeapon && pWeapon->m_iId && (pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD))
	{
		if (NUMBER_OF_ENTITIES() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE))
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime(pWeapon);
	}

	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CBaseHalfLifeCoop::VecWeaponRespawnSpot(CBasePlayerWeapon *pWeapon)
{
	return pWeapon->GetAbsOrigin();
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CBaseHalfLifeCoop::WeaponShouldRespawn(CBasePlayerWeapon *pWeapon)
{
	if (pWeapon->GetSpawnFlags().Any(SF_NORESPAWN))
	{
		return GR_WEAPON_RESPAWN_NO;
	}

	return GR_WEAPON_RESPAWN_YES;
}

//=========================================================
// CanHaveWeapon - returns false if the player is not allowed
// to pick up this weapon
//=========================================================
bool CBaseHalfLifeCoop::CanHavePlayerItem(CBasePlayer *pPlayer, CBasePlayerWeapon *pItem)
{
	if (weaponstay.value > 0)
	{
		if (pItem->iFlags() & ITEM_FLAG_LIMITINWORLD)
			return CGameRules::CanHavePlayerItem(pPlayer, pItem);

		// check if the player already has this weapon
		for (int i = 0; i < MAX_WEAPON_SLOTS; i++)
		{
			CBasePlayerWeapon *it = pPlayer->m_rgpPlayerItems[i];

			while (it != NULL)
			{
				if (it->m_iId == pItem->m_iId)
				{
					return false;
				}

				it = it->m_pNext;
			}
		}
	}

	return CGameRules::CanHavePlayerItem(pPlayer, pItem);
}

//=========================================================
//=========================================================
bool CBaseHalfLifeCoop::CanHaveItem(CBasePlayer *pPlayer, CItem *pItem)
{
	return true;
}

//=========================================================
//=========================================================
void CBaseHalfLifeCoop::PlayerGotItem(CBasePlayer *pPlayer, CItem *pItem)
{
}

//=========================================================
//=========================================================
int CBaseHalfLifeCoop::ItemShouldRespawn(CItem *pItem)
{
	if (pItem->GetSpawnFlags().Any(SF_NORESPAWN))
	{
		return GR_ITEM_RESPAWN_NO;
	}

	return GR_ITEM_RESPAWN_YES;
}

//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CBaseHalfLifeCoop::FlItemRespawnTime(CItem *pItem)
{
	CBaseEntity *pItem2 = (CBaseEntity *)pItem;

	if ((strcmp(pItem2->GetClassname(), "item_healthkit") == 0) || (strcmp(pItem2->GetClassname(), "item_battery") == 0))
	{
		return gpGlobals->time + 5; // Vida e escudo demoram 5 segundos
	}

	return gpGlobals->time; // O resto vem de imediato
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CBaseHalfLifeCoop::VecItemRespawnSpot(CItem *pItem)
{
	return pItem->GetAbsOrigin();
}

//=========================================================
//=========================================================
void CBaseHalfLifeCoop::PlayerGotAmmo(CBasePlayer *pPlayer, char *szName, int iCount)
{
}

//=========================================================
//=========================================================
bool CBaseHalfLifeCoop::IsAllowedToSpawn(CBaseEntity *pEntity)
{
	return true;
}

//=========================================================
//=========================================================
int CBaseHalfLifeCoop::AmmoShouldRespawn(CBasePlayerAmmo *pAmmo)
{
	if (pAmmo->GetSpawnFlags().Any(SF_NORESPAWN))
	{
		return GR_AMMO_RESPAWN_NO;
	}

	return GR_AMMO_RESPAWN_YES;
}

//=========================================================
//=========================================================
float CBaseHalfLifeCoop::FlAmmoRespawnTime(CBasePlayerAmmo *pAmmo)
{
	return gpGlobals->time + 2; // 2 Segundos para a municao voltar
}

//=========================================================
//=========================================================
Vector CBaseHalfLifeCoop::VecAmmoRespawnSpot(CBasePlayerAmmo *pAmmo)
{
	return pAmmo->GetAbsOrigin();
}

//=========================================================
//=========================================================
float CBaseHalfLifeCoop::FlHealthChargerRechargeTime(void)
{
	return 60;
}

float CBaseHalfLifeCoop::FlHEVChargerRechargeTime(void)
{
	return 30;
}

//=========================================================
//=========================================================
int CBaseHalfLifeCoop::DeadPlayerWeapons(CBasePlayer *pPlayer)
{
	return GR_PLR_DROP_GUN_ACTIVE;
}

//=========================================================
//=========================================================
int CBaseHalfLifeCoop::DeadPlayerAmmo(CBasePlayer *pPlayer)
{
	return GR_PLR_DROP_AMMO_ACTIVE;
}

CBaseEntity* CBaseHalfLifeCoop::GetPlayerSpawnSpot(CBasePlayer* pPlayer)
{
	CBaseEntity* pSpawnSpot = CGameRules::GetPlayerSpawnSpot(pPlayer);

	if (auto pSpawnPoint = dynamic_cast<CBaseSpawnPoint*>(pSpawnSpot))
	{
		pSpawnPoint->PlayerSpawned(pPlayer);
	}

	return pSpawnSpot;
}

//=========================================================
//=========================================================
int CBaseHalfLifeCoop::PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget)
{
	// Inimigos para todos os lados
	return GR_NOTTEAMMATE;
}

bool CBaseHalfLifeCoop::PlayFootstepSounds(CBasePlayer *pl, float fvol)
{
	if (g_footsteps && g_footsteps->value == 0)
		return false;

	if (pl->IsOnLadder() || pl->GetAbsVelocity().Length2D() > PLAYER_STEP_SOUND_SPEED)
		return true;  // only make step sounds in multiplayer if the player is moving fast enough

	return false;
}

bool CBaseHalfLifeCoop::FAllowFlashlight() const
{
	return flashlight.value != 0;
}

//=========================================================
//=========================================================
bool CBaseHalfLifeCoop::FAllowMonsters() const
{
	return (allowmonsters.value != 0);
}

void CBaseHalfLifeCoop::SendMOTDToClient(CBasePlayer* pPlayer)
{
	if (CoopPlyData[pPlayer->entindex()].newplayer)
	{
		// read from the MOTD.txt file
		int length, char_count = 0;
		char *pFileList;
		char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME((char *)CVAR_GET_STRING("motdfile"), &length);

		// send the server name
		MESSAGE_BEGIN(MSG_ONE, gmsgServerName, NULL, pPlayer);
		WRITE_STRING(CVAR_GET_STRING("hostname"));
		MESSAGE_END();

		// Send the message of the day
		// read it chunk-by-chunk,  and send it in parts

		while (pFileList && *pFileList && char_count < MAX_MOTD_LENGTH)
		{
			char chunk[MAX_MOTD_CHUNK + 1];

			if (strlen(pFileList) < MAX_MOTD_CHUNK)
			{
				strcpy(chunk, pFileList);
			}
			else
			{
				strncpy(chunk, pFileList, MAX_MOTD_CHUNK);
				chunk[MAX_MOTD_CHUNK] = 0;		// strncpy doesn't always append the null terminator
			}

			char_count += strlen(chunk);
			if (char_count < MAX_MOTD_LENGTH)
				pFileList = aFileList + char_count;
			else
				*pFileList = 0;

			MESSAGE_BEGIN(MSG_ONE, gmsgMOTD, NULL, pPlayer);
			WRITE_BYTE(*pFileList ? 0 : 1);	// 0 means there is still more message to come
			WRITE_STRING(chunk);
			MESSAGE_END();
		}

		FREE_FILE(aFileList);
	}
}

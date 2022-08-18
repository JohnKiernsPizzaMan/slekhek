#include "legitbot.h"
#include <Windows.h>
#include <iostream>

void LegitBot::TriggerBot(UserCmd* cmd)
{
	if (!v::legitbot.triggerbot)
		return;
	//if (!GetAsyncKeyState(VK_XBUTTON2))
	//	return;

	if (!g::localPlayer || !g::localPlayer->IsAlive())
		return;

	Vector localEyePosition;
	g::localPlayer->GetEyePosition(localEyePosition);

	Vector aimPunch = g::localPlayer->GetAimPunch();

	const Vector dst = localEyePosition + Vector{ cmd->viewPoint + aimPunch } * 1000.f;

	Trace trace; 
	i::trace->TraceRay({ localEyePosition, dst }, MASK_SHOT, g::localPlayer, trace);

	if (!trace.entity || !trace.entity->IsPlayer())
		return;

	if (!trace.entity->IsAlive() || trace.entity->GetTeam() == g::localPlayer->GetTeam())
		return;

	cmd->buttons |= UserCmd::IN_ATTACK;
}

void LegitBot::AimBot(UserCmd* cmd) {
	if (!v::legitbot.aimbot)
		return;
	/*if (!(cmd->buttons & UserCmd::IN_ATTACK))
		return;*/
	if (g::localPlayer->IsDefusing())
		return;

	BaseAttributableItem* activeWeapon = g::localPlayer->GetActiveWeapon();

	if (!activeWeapon)
		return;

	
	const int weaponType = activeWeapon->GetWeaponType();

	switch (weaponType)
	{
	case WEAPONTYPE_MACHINEGUN:
	case WEAPONTYPE_PISTOL:
	case WEAPONTYPE_SHOTGUN:
	case WEAPONTYPE_SNIPER:
	case WEAPONTYPE_RIFLE:
	{
		if (!activeWeapon->GetClip())
			return;
		if (weaponType == WEAPONTYPE_SNIPER)
			if (!g::localPlayer->IsScoped())
				return;
		break;
	}
	default:
		return;	
		
	}

	Vector bestAngle{ };

	float bestFov{ v::legitbot.fov };
	for (int i = 0; i <= i::globals->maxClients; i++)
	{
		BaseEntity* player = (BaseEntity*) i::entity->GetClientEntity(i);

		if (!player)
			continue;
		if (player->IsDormant() || !player->IsAlive())
			continue;
		if (player->GetTeam() == g::localPlayer->GetTeam())
			continue;
		if (((CSPlayer*)player)->HasGunGameImmunity())
			continue;
					
		Matrix3x4 bones[128];

		if (!player->SetupBones(bones, 128, 256, i::globals->currentTime))
			continue;

		Vector localEyePosition;
		g::localPlayer->GetEyePosition(localEyePosition);

		Vector aimPunch{};

		switch (weaponType)
		{
		case WEAPONTYPE_RIFLE:
		case WEAPONTYPE_SUBMACHINEGUN:
		case WEAPONTYPE_MACHINEGUN:
			aimPunch = g::localPlayer->GetAimPunch();
		}
		
		Trace trace;
		i::trace->TraceRay(
			Ray{ localEyePosition, bones[8].Origin() },
			MASK_SHOT,
			TraceFilter{ g::localPlayer },
			trace
		);

		if (trace.entity)
			i::cvar->ConsolePrintf("%f\n", trace.fraction);

		if (!trace.entity /* || trace.fraction < 0.70f*/) // add to remove aimming through walls
			return;

		Vector enemyAngle{
			(bones[8].Origin() - localEyePosition).ToAngle() - (cmd->viewPoint + aimPunch)
		};

		if (const float fov = std::hypot(enemyAngle.x, enemyAngle.y); fov < bestFov)
		{
			bestFov = fov;
			bestAngle = enemyAngle;
		}
	}
	
	cmd->viewPoint = cmd->viewPoint + bestAngle.Scale(v::legitbot.smoothing);
	if (v::legitbot.silent)
		return;
	i::engine->SetViewAngles(cmd->viewPoint);

}

void LegitBot::RecoilControl(UserCmd* cmd)
{
	if (!v::legitbot.rcs)
		return;
	if (!g::localPlayer || !g::localPlayer->IsAlive())
		return;
	static Vector oldPunch{};
	if (cmd->buttons & UserCmd::IN_ATTACK)
	{
		
		Vector currentPunch{};
		switch (g::localPlayer->GetActiveWeapon()->GetWeaponType())
		{
		case WEAPONTYPE_RIFLE:
		case WEAPONTYPE_SUBMACHINEGUN:
		case WEAPONTYPE_MACHINEGUN:
			currentPunch = g::localPlayer->GetAimPunch();
		}

		Vector newAngles = Vector
		{
			cmd->viewPoint.x + oldPunch.x - currentPunch.x * 2.f,
			cmd->viewPoint.y + oldPunch.y - currentPunch.y * 2.f
		};

		oldPunch = currentPunch * 2.f;

		if (newAngles.x > 89.f)
			newAngles.x = 89.f;

		if (newAngles.x < -89.f)
			newAngles.x = -89.f;

		while (newAngles.y > 180.f)
			newAngles.y -= 360.f;

		while (newAngles.y < -180.f)
			newAngles.y += 360.f;

		cmd->viewPoint = newAngles;
		i::engine->SetViewAngles(cmd->viewPoint);
	}
	else {
		oldPunch.x = oldPunch.y = 0.f;
	}
	
}
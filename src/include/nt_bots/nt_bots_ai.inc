#if defined(_NT_BOTS_AI_INC_)
#endinput
#endif
#define _NT_BOTS_AI_INC_

enum StuckDir {
	STUCK_DIR_NONE,
	STUCK_DIR_NORTH,
	STUCK_DIR_EAST,
	STUCK_DIR_SOUTH,
	STUCK_DIR_WEST
};

void NoticeEnemy(int bot_client, int other_client, const float[3] enemy_pos)
{
	_enemy_positions[bot_client][other_client] = enemy_pos;
	_enemy_last_spotted_time_single[bot_client] = GetGameTime();
	_enemy_last_spotted_time[bot_client][other_client] = _enemy_last_spotted_time_single[bot_client];
}

public bool Cb_TraceEnumerator(int hit_entity, int other_client)
{
	if (hit_entity != other_client) {
		TR_ClipCurrentRayToEntity(MASK_OPAQUE, hit_entity);
		return true;
	}
	return false;
}

public void ThinkHook(int client)
{	
	float eye_pos[3];
	float other_client_eye_pos[3];
	float eye_dirs[3];
	float eye_ang[3];
	
	GetClientEyePosition(client, eye_pos);
	
	float normalized_ang_fwd[3];
	float actual_eye_ang[3]; float actual_eye_fwd[3];
	GetClientEyeAngles(client, actual_eye_ang);
	GetAngleVectors(actual_eye_ang, actual_eye_fwd, NULL_VECTOR, NULL_VECTOR);
	NormalizeVector(actual_eye_fwd, actual_eye_fwd);
	
	if (!g_cNoTarget.BoolValue) {
		for (int other_client = 1; other_client <= MaxClients; ++other_client) {
			if (!IsClientInGame(other_client) || !IsPlayerAlive(other_client)) {
				continue;
			}
			else if (GetClientTeam(client) == GetClientTeam(other_client)) {
				continue;
			}
			
			GetClientEyePosition(other_client, other_client_eye_pos);
			
			MakeVectorFromPoints(eye_pos, other_client_eye_pos, eye_dirs);
			GetVectorAngles(eye_dirs, eye_ang);
			
			// Unless we just got attacked by this player...
			if (bot_last_took_dmg_from[client] != other_client) {
				// ...don't spot enemies behind the bot
				GetAngleVectors(eye_ang, normalized_ang_fwd, NULL_VECTOR, NULL_VECTOR);
				NormalizeVector(normalized_ang_fwd, normalized_ang_fwd);
				float arccos = ArcCosine(GetVectorDotProduct(actual_eye_fwd, normalized_ang_fwd));
				if (arccos > 1) {
					continue;
				}
			}
			
			TR_TraceRayFilter(eye_pos, eye_ang, MASK_OPAQUE,
				RayType_Infinite, DidNotHitSelf, client);
			
			if (TR_DidHit()) {
				if (TR_GetEntityIndex() == other_client) {
					// Aim directly at head at close range, but aim for chest at longer distances.
					float max_headshot_distance = 128.0;
					float aim_lower_scale = fmin(max_headshot_distance, GetVectorDistance(eye_pos, other_client_eye_pos)) / max_headshot_distance;
					float aim_lower_max = 16.0;
					other_client_eye_pos[2] -= aim_lower_max * aim_lower_scale; // aim slightly lower than eye height
					NoticeEnemy(client, other_client, other_client_eye_pos);
				}
			}
		}
	}
}

static float lastPos[NEO_MAX_PLAYERS + 1][3];
static int lastStuckTraceTime[NEO_MAX_PLAYERS + 1];
static int _num_cycles_stuck[NEO_MAX_PLAYERS + 1];

bool CheckIfStuck(int client)
{
	float currentPos[3], currentAng[3];
	GetClientAbsOrigin(client, currentPos);
	GetClientAbsAngles(client, currentAng);
	
	float result = 0.0;
	if (GetVectorLength(lastPos[client]) != 0) {
		result = GetVectorDistance(lastPos[client], currentPos);
	}
	lastPos[client] = currentPos;
	
	// Are we not moving enough?
	if (result < 0.5) {
		++_num_cycles_stuck[client];
		return true;
	}
	
	int thisTime = GetTime();
	int deltaTime = thisTime - lastStuckTraceTime[client];
	if (deltaTime > 10) {
		lastStuckTraceTime[client] = thisTime;
		
		float xyz[3];
		TR_TraceRayFilter(currentPos, currentAng, MASK_OPAQUE,
			RayType_Infinite, DidNotHitSelf, client);
		if (TR_DidHit(INVALID_HANDLE) /*&& !TR_AllSolid(INVALID_HANDLE)*/) {
			TR_GetEndPosition(xyz, INVALID_HANDLE);
		}
		
		if (!VectorsEqual(xyz, vec3_origin)) {
			if (GetVectorDistance(currentPos, xyz) > _close_enough_dist[client]) {
				return true;
			}
		}
	}
	
	if (_num_cycles_stuck[client] > 0) {
		--_num_cycles_stuck[client];
	}
	return false;
}

int GetNumCyclesStuck(int client)
{
	return _num_cycles_stuck[client];
}

int DecideVelocity(int client, bool is_stuck, const float aim_ang[3], float vel[3])
{
	int buttons = 0;
	
	// If currently jumping, maintain crouched jump for max climbing ability over obstacles
	if (!(GetEntityFlags(client) & FL_ONGROUND)) {
		buttons |= IN_DUCK;
	}
	
	// Bot doesn't yet understand ladders. Just jump to dismount any accidental ladder mounts.
	if (GetEntityMoveType(client) & MOVETYPE_LADDER) {
		buttons |= IN_JUMP;
	}
	
	if (!HasActivePlan(client)) {
		SetFailState("DecideVelocity: No plan");
	}
	if (IsNearPlannedPosition(client)) {
		ContinuePlan(client);
		return buttons;
	}
	
	float planned_pos[3];
	if (!GetPlannedPosition(client, planned_pos)) {
		return buttons;
	}
	else if (VectorsEqual(planned_pos, vec3_origin)) {
		vel = vec3_origin;
		
		if (GetPlayerClass(client) != CLASS_SUPPORT && !IsCloaked(client) && GetEntPropFloat(client, Prop_Send, "m_fThermopticNRG") > 3) {
			SetEntProp(client, Prop_Send, "m_iThermoptic", 1);
		}
		
		return buttons | IN_DUCK;
	}
	
	float client_pos[3];
	GetClientAbsOrigin(client, client_pos);
	
	SubtractVectors(planned_pos, client_pos, vel);
	vel[1] = -vel[1];
	
	float cos_ang = Cosine(DegToRad(aim_ang[1]));
	float sin_ang = Sine(DegToRad(aim_ang[1]));
	
	float velocity_offset_by_yaw[2];
	velocity_offset_by_yaw[0] = cos_ang * vel[0] - sin_ang * vel[1];
	velocity_offset_by_yaw[1] = sin_ang * vel[0] + cos_ang * vel[1];
	
	vel[0] = velocity_offset_by_yaw[0];
	vel[1] = velocity_offset_by_yaw[1];
	
	NormalizeVector(vel, vel);
	ScaleVector(vel, 1000.0);
	
	if (g_cDrawDebugBeams.BoolValue) {
		TE_SetupBeamPoints(client_pos, planned_pos, rayIndex, rayIndex,
			0, 1, 1.0, 2.0, 2.5, 5, 1.0, ray_color, 5);
		TE_SendToAll();
	}
	
	if (is_stuck) {
		int num_cycles_stuck = GetNumCyclesStuck(client);
		
		float randX = 1.0;
		float randY = 1.0;
		
		float wiggle_scale = 10.0;
		if (++num_cycles_stuck <= 10000) {
			num_cycles_stuck *= 2;
		}
		SetRandomSeed(GetGameTickCount() + client);
		randX *= (num_cycles_stuck * GetRandomFloat(-wiggle_scale, wiggle_scale));
		SetRandomSeed(GetGameTickCount() + client + 1);
		randY *= (num_cycles_stuck * GetRandomFloat(-wiggle_scale, wiggle_scale));
		
		vel[0] += randX;
		vel[1] += randY;
		
		if (num_cycles_stuck >= 10) {
			float fwd_distance;
			float right_distance;
			// This is kind of pointlessly complex, as it seems best to always spin the same way
			// for best chances of un-stucking. Should probably remove GetStuckDir here.
			switch(GetStuckDir(client, client_pos, aim_ang, fwd_distance, right_distance)) {
				case STUCK_DIR_NORTH:
				{
					vel[0] -= (1.0 * num_cycles_stuck);
				}
				case STUCK_DIR_SOUTH:
				{
					vel[0] -= (1.0 * num_cycles_stuck);
				}
				case STUCK_DIR_EAST:
				{
					vel[1] += (0.5 * num_cycles_stuck);
				}
				case STUCK_DIR_WEST:
				{
					vel[1] += (0.5 * num_cycles_stuck);
				}
			}
			
			if (GetVectorDistance(client_pos, planned_pos) > _close_enough_dist[client]) {
				if (fwd_distance > 64 || right_distance > 64) {
					SetRandomSeed(GetGameTickCount() + client + 3);
					if (GetRandomInt(0, server_tick_rate * 10) == 0) {
						buttons |= (IN_JUMP | IN_DUCK);
					}
				}
			}
		}
	}
	
	return buttons;
}

static float last_spotted[NEO_MAX_PLAYERS + 1];

static float previous_aim[NEO_MAX_PLAYERS + 1][3];
static float perfect_angles[NEO_MAX_PLAYERS + 1][3];

int DecideAngles(int client, bool& is_stuck, float[3] angles)
{
	int buttons = 0;
	
	// Get the most recently sighted client index, or -1 if the bot has seen nobody recently.
	int last_seen_client = -1;
	float last_spotted_time;
	float time_now = GetGameTime();
	for (int other_client = 1; other_client <= MaxClients; ++other_client) {
		if (other_client == client || !IsClientInGame(other_client) || !IsPlayerAlive(other_client)) {
			continue;
		}
		
		float time = _enemy_last_spotted_time[client][other_client];
		
		if (time == 0) {
			continue;
		}
		else if (time > time_now) {
			_enemy_last_spotted_time[client][other_client] = 0.0;
			continue;
		}
		else if (last_seen_client == -1 || time < last_spotted_time) {
			last_seen_client = other_client;
			last_spotted_time = time;
		}
		
		if (last_spotted[other_client] != 0 && time - last_spotted[other_client] > g_cForgetEnemyTime.FloatValue) {
			last_spotted[other_client] = 0.0;
		}
	}
	
	static int wep_slot = SLOT_PRIMARY;
	int weapon = GetPlayerWeaponSlot(client, wep_slot);
	if (!IsValidEdict(weapon)) {
		wep_slot = ++wep_slot % 2;
		weapon = GetPlayerWeaponSlot(client, wep_slot);
	}
	
	if (IsValidEdict(weapon)) {
		if (IsWeaponGhost(GetPlayerWeaponSlot(client, SLOT_PRIMARY))) {
			SDKHooks_DropWeapon(client, GetPlayerWeaponSlot(client, SLOT_PRIMARY), NULL_VECTOR, NULL_VECTOR);
		}
		else {
			int current_ammo = GetEntProp(weapon, Prop_Send, "m_iClip1");
			int total_ammo = GetWeaponAmmo(client, GetAmmoType(weapon));
			
			float eye_pos[3]; float eye_ang[3]; float eye_dir[3]; float target_dir[3];
			GetClientEyePosition(client, eye_pos);
			GetClientEyeAngles(client, eye_ang);
			GetAngleVectors(eye_ang, eye_dir, NULL_VECTOR, NULL_VECTOR);
			NormalizeVector(eye_dir, eye_dir);
			
			if (current_ammo > 0) {
				//PrintToChatAll("Current ammo: %d", current_ammo);
				if (last_seen_client != -1 && (time_now - last_spotted_time) < g_cForgetEnemyTime.FloatValue) {
					if (!VectorsEqual(_enemy_positions[client][last_seen_client], vec3_origin)) {
						
						if (GetPlayerClass(client) != CLASS_SUPPORT && !IsCloaked(client) && GetEntPropFloat(client, Prop_Send, "m_fThermopticNRG") > 3) {
							SetEntProp(client, Prop_Send, "m_iThermoptic", 1);
						}
						
						is_stuck = false;
						
						SubtractVectors(_enemy_positions[client][last_seen_client], eye_pos, target_dir);
						NormalizeVector(target_dir, target_dir);
						
						if (g_cDrawDebugBeams.BoolValue) {
							float ray_pos[3];
							TR_TraceRay(eye_pos, angles, MASK_OPAQUE, RayType_Infinite);
							TR_GetEndPosition(ray_pos);
							
							TE_SetupBeamPoints(eye_pos, ray_pos, rayIndex, rayIndex,
								0, 1, 1.0, 2.0, 2.5, 5, 1.0, ray_color, 5);
							TE_SendToAll();
						}
	
						if (last_seen_client != -1 && (time_now - last_spotted_time) < g_cSuppressFireTime.FloatValue) {
							GetVectorAngles(target_dir, perfect_angles[client]);
							
							if (last_spotted[client] == 0) {
								last_spotted[client] = last_spotted_time;
							}
							else {
								if (time_now - last_spotted[client] >= g_cReactionTime.FloatValue) {
									if (wep_slot == SLOT_SECONDARY || IsWeaponSemiautoFire(weapon)) {
										if (!(prev_buttons[client] & IN_ATTACK)) {
											buttons |= IN_ATTACK;
										}
									}
									else if (current_ammo != 0) {
										buttons |= IN_ATTACK;
									}
									else if (wep_slot == SLOT_PRIMARY) { // Fallback for handling the Supa with its slug reloads. Should fix some time.
										SDKHooks_DropWeapon(client, weapon, NULL_VECTOR, NULL_VECTOR);
										FakeClientCommand(client, "slot2");
									}
								}
							}
						}
					}
				}
				else {
					float planned_pos[3];
					if (GetPlannedPosition(client, planned_pos, 2)) {
						if (VectorsEqual(planned_pos, vec3_origin)) {
							is_stuck = false;
							perfect_angles[client] = previous_aim[client];
						}
						else {
							if (IsCloaked(client) && GetEntPropFloat(client, Prop_Send, "m_fThermopticNRG") < 5) {
								SetEntProp(client, Prop_Send, "m_iThermoptic", 0);
							}
							
							SubtractVectors(planned_pos, eye_pos, target_dir);
							
							GetVectorAngles(target_dir, perfect_angles[client]);
						}
					}
				}
			}
			else {
				if (total_ammo == 0) {
					int owner = GetEntPropEnt(weapon, Prop_Data, "m_hOwnerEntity");
					if (owner == client) {
						if (wep_slot == SLOT_PRIMARY) {
							SDKHooks_DropWeapon(client, weapon, NULL_VECTOR, NULL_VECTOR);
							FakeClientCommand(client, "slot2");
						}
						else {
							int main_weapon = GetPlayerWeaponSlot(client, SLOT_PRIMARY);
							int main_ammo;
							if (IsValidEdict(main_weapon)) {
								main_ammo = GetEntProp(GetPlayerWeaponSlot(client, SLOT_PRIMARY), Prop_Send, "m_iClip1");
							}
							if (main_ammo > 0) {
								FakeClientCommand(client, "slot1");
								wep_slot = SLOT_PRIMARY;
							}
							else {
								// If we're exhausted all primary ammo, and pistol ammo, then cheat a bit and give us more secondary ammo to keep on fighting.
								// This could be revisited later to make the bot scavenge for another weapon, or use the knife, etc.
								SetWeaponAmmo(client, GetAmmoType(weapon), 30);
							}
						}
					}
				}
				else {
					if (!(prev_buttons[client] & IN_RELOAD)) {
						buttons |= IN_RELOAD;
					}
				}
			}
			
			SetRandomSeed(GetGameTickCount() + 1);
			angles[0] = perfect_angles[client][0] + GetRandomFloat(-g_cAimInaccuracy.FloatValue, g_cAimInaccuracy.FloatValue);
			SetRandomSeed(GetGameTickCount() + 2);
			angles[1] = perfect_angles[client][1] + GetRandomFloat(-g_cAimInaccuracy.FloatValue, g_cAimInaccuracy.FloatValue);
			
			float scale = g_cBotAimSpeed.FloatValue * GetTickInterval();
			if (last_seen_client != -1) {
				// If enemy is cloaked, and we don't have a strong vision mode, add some delay to tracking aim.
				if (IsCloaked(last_seen_client)) {
					switch (GetPlayerClass(client)) {
						case CLASS_RECON: scale *= 1.5;
						case CLASS_SUPPORT: scale *= 2;
					}
				}
			}
			
			angles[0] = Lerp(previous_aim[client][0], perfect_angles[client][0], scale);
			angles[1] = Lerp(previous_aim[client][1], perfect_angles[client][1], scale);
			previous_aim[client][0] = angles[0];
			previous_aim[client][1] = angles[1];
		}
	}
	
	if (is_stuck) {
		int num_cycles_stuck = GetNumCyclesStuck(client);
		
		if (num_cycles_stuck > server_tick_rate) {
			static float rotate = 0.0;
			//SetRandomSeed(GetGameTickCount() + num_cycles_stuck);
			rotate -= 0.001 * num_cycles_stuck;
			if (rotate >= 360) {
				rotate -= 360.0;
			}
			else if (rotate < 0) {
				rotate += 360.0;
			}
			angles[1] += rotate;
		}
	}
	
	if (!(buttons & IN_ATTACK) && GetPlayerClass(client) != CLASS_SUPPORT) {
		buttons |= (IN_RUN | IN_SPRINT);
	}
	
	return buttons;
}

stock StuckDir GetStuckDir(int client, const float[3] current_pos,
	const float[3] current_ang, float& fwd_distance, float& right_distance)
{
	int closest_dir = -1;
	float closest_distance;
	
	float ang[3];
	ang[0] = current_ang[0];
	ang[1] = current_ang[1];
	ang[2] = current_ang[2];
	
	float start_pos[3];
	start_pos[0] = current_pos[0];
	start_pos[1] = current_pos[1];
	start_pos[2] = current_pos[2] + 64.0;
	
	for (int dir = 0; dir < 4; ++dir) {
		ang[1] += dir * 90.0;
		if (ang[1] > 360) {
			ang[1] -= 360.0;
		}
		
		TR_TraceRayFilter(start_pos, ang, MASK_OPAQUE,
			RayType_Infinite, DidNotHitSelf, client);
		if (TR_DidHit(INVALID_HANDLE)) {
			float pos[3];
			TR_GetEndPosition(pos, INVALID_HANDLE);
			float distance = GetVectorDistance(pos, current_pos);
			
			if (dir == 0) {
				fwd_distance = distance;
			}
			else if (dir == 1) {
				right_distance = distance;
			}
			
			if (closest_dir == -1 || distance < closest_distance) {
				closest_dir = dir;
				closest_distance = distance;
			}
		}
	}
	
	switch (closest_dir) {
		case 0: return STUCK_DIR_NORTH;
		case 1: return STUCK_DIR_EAST;
		case 2: return STUCK_DIR_SOUTH;
		case 3: return STUCK_DIR_WEST;
		default: return STUCK_DIR_NONE;
	}
}

#define BOT_THINK_INVERTAL_SECONDS 0.16666666666

static float bot_last_think[NEO_MAX_PLAYERS + 1];
bool IsBotThinkFrame(int client)
{
	if (GetNextBotThinker() != client) {
		return false;
	}
	
	float time_now = GetGameTime();
	
	if (bot_last_think[client] == 0 || bot_last_think[client] > time_now) {
		bot_last_think[client] = time_now;
		return true;
	}
	
	else if ((time_now - bot_last_think[client]) * num_bot_clients >= BOT_THINK_INVERTAL_SECONDS) {
		bot_last_think[client] = time_now;
		return true;
	}
	
	return false;
}

static float s_movementPlan[NEO_MAX_PLAYERS + 1][MAX_AREA_MOVES_PLANNED_FOR_FUTURE][3];
static int s_movementPlan_Head[NEO_MAX_PLAYERS + 1];
static int s_movementPlan_WriteHead[NEO_MAX_PLAYERS + 1];

bool IsNearPlannedPosition(int client)
{
	if (_close_enough_dist[client] == 0) {
		return false;
	}
	
	float pos[3];
	GetClientAbsOrigin(client, pos);
	
	float planned_pos[3];
	if (!GetPlannedPosition(client, planned_pos)) {
		return false;
	}
	
	return GetVectorDistance(pos, planned_pos) <= _close_enough_dist[client];
}

bool GetPlannedPosition(int client, float[3] out_pos, int offset = 0)
{
	if (!HasActivePlan(client)) {
		return false;
	}
	
	int index = s_movementPlan_Head[client] + offset;
	
	if (index >= MAX_AREA_MOVES_PLANNED_FOR_FUTURE) {
		PrintToServer("!!! Failed to get ahead pos");
		return false;
	}
	
	out_pos = s_movementPlan[client][index];
	
	return true;
}

void ClearPlan(int client)
{
	s_movementPlan_Head[client] = -1;
	s_movementPlan_WriteHead[client] = 0;
}

bool HasActivePlan(int client)
{
	return s_movementPlan_Head[client] != -1;
}

void ContinuePlan(int client)
{
	if (++s_movementPlan_Head[client] + 1 == s_movementPlan_WriteHead[client]) {
		return;
	}
	
	if (++s_movementPlan_Head[client] >= MAX_AREA_MOVES_PLANNED_FOR_FUTURE) {
		ClearPlan(client);
		SetFailState("ContinuePlan: PLAN STACK OVERFLOWED");
	}
}

bool PlanMove(int client, const float[3] planned_pos)
{
	//PrintToServer("Planning move for client %d", client);
	
	if (s_movementPlan_WriteHead[client] >= MAX_AREA_MOVES_PLANNED_FOR_FUTURE) {
		ClearPlan(client);
		SetFailState("PlanMove: PLAN STACK OVERFLOWED (%d)", s_movementPlan_WriteHead[client]);
		return false;
	}
	
	s_movementPlan[client][s_movementPlan_WriteHead[client]][0] = planned_pos[0];
	s_movementPlan[client][s_movementPlan_WriteHead[client]][1] = planned_pos[1];
	s_movementPlan[client][s_movementPlan_WriteHead[client]][2] = planned_pos[2];
	
	//PrintToServer("Wrote coords to head %d: (%f %f %f)",
	//	s_movementPlan_WriteHead[client],
	//	s_movementPlan[client][s_movementPlan_WriteHead[client]][0],
	//	s_movementPlan[client][s_movementPlan_WriteHead[client]][1],
	//	s_movementPlan[client][s_movementPlan_WriteHead[client]][2]);
	
	++s_movementPlan_WriteHead[client];
	
	if (s_movementPlan_Head[client] == -1) {
		s_movementPlan_Head[client] = 0;
	}
	
	return true;
}

static int bot_clients_head;

void SetNextBotThinkerHead()
{
	bot_clients_head = (num_bot_clients == 0 ? 0 : ++bot_clients_head % num_bot_clients);
}

int GetNextBotThinker()
{
	return bot_clients[bot_clients_head];
}
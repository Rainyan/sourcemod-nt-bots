#if defined(_NT_BOTS_BASE_INC_)
#endinput
#endif
#define _NT_BOTS_BASE_INC_

// Is this client a bot that we want to use for bot AI?
// Assumes a valid client index and IsClientConnected == true.
stock bool IsBotClient(int client)
{
    return client != 0 && IsFakeClient(client) && !IsClientSourceTV(client);
}

// Predicate for client position initiated ray trace.
stock bool DidNotHitSelf(int self, int contentsMask, int target)
{
    return self != target;
}

stock void MultiplyVectors(const float v1[3], const float v2[3], float res[3])
{
    res[0] = v1[0] * v2[0];
    res[1] = v1[1] * v2[1];
    res[2] = v1[2] * v2[2];
}

stock bool VectorsEqual(const float[3] v1, const float[3] v2, const float max_ulps = 0.0)
{
	// Needs to exactly equal.
	if (max_ulps == 0) {
		return v1[0] == v2[0] && v1[1] == v2[1] && v1[2] == v2[2];
	}
	// Allow an inaccuracy of size max_ulps.
	else {
		if (FloatAbs(v1[0] - v2[0]) > max_ulps) { return false; }
		if (FloatAbs(v1[1] - v2[1]) > max_ulps) { return false; }
		if (FloatAbs(v1[2] - v2[2]) > max_ulps) { return false; }
		return true;
	}
}

stock float Lerp(float a, float b, float scale = 0.0)
{
	if (scale == 0) {
		scale = GetGameFrameTime();
	}
#if(0)
	scale = fclamp(scale, -1.0, 1.0);
#endif
	return a + (b - a) * scale;
}

stock void VectorLerp(const float[3] v1, const float[3] v2, float[3] res, float scale = 0.0)
{
	res[0] = Lerp(v1[0], v2[0], scale);
	res[1] = Lerp(v1[1], v2[1], scale);
	res[2] = Lerp(v1[2], v2[2], scale);
}

stock void AngleMatrix(const float[3] ang, float[3][3] out_matrix)
{
	float sp, cp, sy, cy, sr, cr;
	
	float deg2rad_pitch = DegToRad(ang[0]);
	float deg2rad_yaw = DegToRad(ang[1]);
	float deg2rad_roll = DegToRad(ang[2]);
	
	sp = Sine(deg2rad_pitch);
	cp = Cosine(deg2rad_pitch);
	sy = Sine(deg2rad_yaw);
	cy = Cosine(deg2rad_yaw);
	sr = Sine(deg2rad_roll);
	cr = Cosine(deg2rad_roll);
	
	out_matrix[0][0] = cp * cy;
	out_matrix[1][0] = cp * sy;
	out_matrix[2][0] = -sp;
	
	float crcy = cr * cy;
	float crsy = cr * sy;
	float srcy = sr * cy;
	float srsy = sr * sy;
	
	out_matrix[0][1] = sp * srcy - crsy;
	out_matrix[1][1] = sp * srsy + crcy;
	out_matrix[2][1] = sr * cp;

	out_matrix[0][2] = sp * crcy + srsy;
	out_matrix[1][2] = sp * crsy - srcy;
	out_matrix[2][2] = cr * cp;
}

stock void MatrixAngle(const float[3][3] matrix, float[3] out_ang)
{
	float fwd[3]; float left[3]; float up[3];
	
	fwd[0] = matrix[0][0];
	fwd[1] = matrix[1][0];
	fwd[2] = matrix[2][0];
	left[0] = matrix[0][1];
	left[1] = matrix[1][1];
	left[2] = matrix[2][1];
	up[2] = matrix[2][2];
	
	float xy_dist = SquareRoot(fwd[0] * fwd[0] + fwd[1] * fwd[1]);
	
	if (xy_dist > 0.001) {
		out_ang[1] = RadToDeg(ArcTangent2(fwd[1], fwd[0]));
		out_ang[0] = RadToDeg(ArcTangent2(-fwd[2], xy_dist));
		out_ang[2] = RadToDeg(ArcTangent2(left[2], up[2]));
	}
	else {
		out_ang[1] = RadToDeg(ArcTangent2(-left[0], left[1]));
		out_ang[0] = RadToDeg(ArcTangent2(-fwd[2], xy_dist));
	}
}

stock void MatrixMultiply(const float[3][3] m1, const float[3][3] m2, float [3][3] m_out)
{
	m_out[0][0] = m1[0][0] * m2[0][0] + m1[0][1] * m2[1][0] + m1[0][2] * m2[2][0];
	m_out[0][1] = m1[0][0] * m2[0][1] + m1[0][1] * m2[1][1] + m1[0][2] * m2[2][1];
	m_out[0][2] = m1[0][0] * m2[0][2] + m1[0][1] * m2[1][2] + m1[0][2] * m2[2][2];
	
	m_out[1][0] = m1[1][0] * m2[0][0] + m1[1][1] * m2[1][0] + m1[1][2] * m2[2][0];
	m_out[1][1] = m1[1][0] * m2[0][1] + m1[1][1] * m2[1][1] + m1[1][2] * m2[2][1];
	m_out[1][2] = m1[1][0] * m2[0][2] + m1[1][1] * m2[1][2] + m1[1][2] * m2[2][2];
	
	m_out[2][0] = m1[2][0] * m2[0][0] + m1[2][1] * m2[1][0] + m1[2][2] * m2[2][0];
	m_out[2][1] = m1[2][0] * m2[0][1] + m1[2][1] * m2[1][1] + m1[2][2] * m2[2][1];
	m_out[2][2] = m1[2][0] * m2[0][2] + m1[2][1] * m2[1][2] + m1[2][2] * m2[2][2];
}

stock void CopyMatrix(const float[3][3] m, float[3][3] out_m)
{
	out_m[0][0] = m[0][0];
	out_m[0][1] = m[0][1];
	out_m[0][2] = m[0][2];
	
	out_m[1][0] = m[1][0];
	out_m[1][1] = m[1][1];
	out_m[1][2] = m[1][2];
	
	out_m[2][0] = m[2][0];
	out_m[2][1] = m[2][1];
	out_m[2][2] = m[2][2];
}

stock void VectorRotate(const float[3] vec, const float[3][3] mat, float[3] out_vec)
{
	out_vec[0] = GetVectorDotProduct(vec, mat[0]);
	out_vec[1] = GetVectorDotProduct(vec, mat[1]);
	out_vec[2] = GetVectorDotProduct(vec, mat[2]);
}

// Round vector's elements within the accuracy.
stock void RoundVector(float[3] out_vec, int accuracy)
{
	for (int i = 0; i < 3; ++i) {
		out_vec[i] = 1.0 * (RoundToNearest(out_vec[i] / accuracy) * accuracy);
	}
}

stock int teamidx_to_navteamidx(const int team_index)
{
	return team_index == TEAM_NSF ? NAV_TEAM_IDX_NSF : NAV_TEAM_IDX_JINRAI;
}

stock int navteamidx_to_teamidx(const int bot_mesh_team_index)
{
	return bot_mesh_team_index == NAV_TEAM_IDX_NSF ? TEAM_NSF : TEAM_JINRAI;
}

stock float fmin(const float a, const float b)
{
	return a < b ? a : b;
}

stock float fmax(const float a, const float b)
{
	return a > b ? a : b;
}

stock float fclamp(const float value, const float min, const float max)
{
	return fmax(fmin(value, min), max);
}

void MakePlan(int client)
{
	//PrintToServer("MakePlan: %d", client);
	
	float start_pos[3];
	GetClientAbsOrigin(client, start_pos);
	
	float the_target_pos[3];
	
	// No ghost found, and no enemies found. Do nothing.
	if (!GetGhostPos(the_target_pos) && !GetNearestEnemy(client, the_target_pos)) {
		return;
	}
	
	RequestPath(client, start_pos, the_target_pos);
}

// Assumes weapon input to always be a valid NT wep index,
// or -1 for invalid weapon.
bool IsWeaponGhost(int weapon)
{
	if (weapon == -1) {
		return false;
	}

	// Not zero initializing this, because all NT weps have a classname
	// longer than this. We assume any non -1 ent index we get is always
	// a valid NT weapon ent index.
	decl String:wepName[9 + 1]; // "weapon_gh" + '\0' == strlen 10
	if (!GetEntityClassname(weapon, wepName, sizeof(wepName))) {
		return false;
	}

	// weapon_gHost -- only weapon with letter H on 8th position of its name.
	return wepName[8] == 'h';
}

bool GetGhostPos(float[3] out_pos)
{
	char name[12 + 1];
	
	for (int client = 1; client <= MaxClients; ++client) {
		if (!IsClientConnected(client) || !IsBotClient(client)) {
			continue;
		}
	}
	
	for (int client = 1; client <= MaxClients; ++client) {
		if (!IsClientInGame(client)) {
			continue;
		}
		GetClientWeapon(client, name, sizeof(name));
		if (StrEqual(name, "weapon_ghost")) {
			GetClientAbsOrigin(client, out_pos);
			return true;
		}
	}
	
	for (int ent = NEO_MAX_PLAYERS + 1; ent <= GetMaxEntities(); ++ent) {
		if (IsValidEdict(ent) && GetEdictClassname(ent, name, sizeof(name))) {
			if (StrEqual(name, "weapon_ghost")) {
				GetEntPropVector(ent, Prop_Send, "m_vecOrigin", out_pos);
				return true;
			}
		}
	}
	
	return false;
}

bool GetNearestEnemy(int self, float[3] out_pos)
{
	for (int client = 1; client <= MaxClients; ++client) {
		if (!IsClientInGame(client) || GetClientTeam(client) == GetClientTeam(self)) {
			continue;
		}
		GetClientAbsOrigin(client, out_pos);
		return true;
	}
	return false;
}

// Gets a random loadout weapon string based on class and XP.
void SuggestWeapon(int class, int xp, char[] out_weapon, int maxlen)
{
	enum {
		//WEPNAME_GHOST = 0,
		WEPNAME_MPN = 1,
		WEPNAME_SRM,
		WEPNAME_SRM_S,
		WEPNAME_JITTE,
		WEPNAME_JITTESCOPED,
		WEPNAME_ZR68C,
		WEPNAME_ZR68L,
		WEPNAME_ZR68S,
		WEPNAME_SUPA7,
		WEPNAME_M41,
		WEPNAME_M41S,
		WEPNAME_MX,
		WEPNAME_MX_SILENCED,
		WEPNAME_AA13,
		WEPNAME_SRS,
		WEPNAME_PZ
	};
	
	SetRandomSeed(GetGameTickCount() + class + xp);
	
	if (class == CLASS_RECON) {
		if (xp < 4) {
			int recon_private_weps[] = {
				WEPNAME_MPN, WEPNAME_SRM, WEPNAME_JITTE
			};
			strcopy(out_weapon, maxlen, weapons_primary[recon_private_weps[GetRandomInt(0, sizeof(recon_private_weps) - 1)]]);
		}
		else if (xp < 20) {
			int recon_corporal_weps[] = {
				WEPNAME_MPN, WEPNAME_SRM, WEPNAME_JITTE,
				WEPNAME_SRM_S, WEPNAME_JITTESCOPED, WEPNAME_ZR68L
			};
			strcopy(out_weapon, maxlen, weapons_primary[recon_corporal_weps[GetRandomInt(0, sizeof(recon_corporal_weps) - 1)]]);
		}
		else {
			int recon_lieutenant_weps[] = {
				WEPNAME_MPN, WEPNAME_SRM, WEPNAME_JITTE,
				WEPNAME_SRM_S, WEPNAME_JITTESCOPED, WEPNAME_ZR68L,
				WEPNAME_ZR68C, WEPNAME_SUPA7, WEPNAME_M41S
			};
			strcopy(out_weapon, maxlen, weapons_primary[recon_lieutenant_weps[GetRandomInt(0, sizeof(recon_lieutenant_weps) - 1)]]);
		}
	}
	else if (class == CLASS_ASSAULT) {
		if (xp < 4) {
			int assault_private_weps[] = {
				WEPNAME_MPN, WEPNAME_SRM, WEPNAME_JITTE,
				WEPNAME_ZR68C, WEPNAME_ZR68S
			};
			strcopy(out_weapon, maxlen, weapons_primary[assault_private_weps[GetRandomInt(0, sizeof(assault_private_weps) - 1)]]);
		}
		else if (xp < 10) {
			int assault_corporal_weps[] = {
				WEPNAME_MPN, WEPNAME_SRM, WEPNAME_JITTE,
				WEPNAME_ZR68C, WEPNAME_ZR68S, WEPNAME_SUPA7,
				WEPNAME_M41, WEPNAME_M41S
			};
			strcopy(out_weapon, maxlen, weapons_primary[assault_corporal_weps[GetRandomInt(0, sizeof(assault_corporal_weps) - 1)]]);
		}
		else if (xp < 20) {
			int assault_sergeant_weps[] = {
				WEPNAME_MPN, WEPNAME_SRM, WEPNAME_JITTE,
				WEPNAME_ZR68C, WEPNAME_ZR68S, WEPNAME_SUPA7,
				WEPNAME_M41, WEPNAME_M41S, WEPNAME_MX,
				WEPNAME_MX_SILENCED
			};
			strcopy(out_weapon, maxlen, weapons_primary[assault_sergeant_weps[GetRandomInt(0, sizeof(assault_sergeant_weps) - 1)]]);
		}
		else {
			int assault_lieutenant_weps[] = {
				WEPNAME_MPN, WEPNAME_SRM, WEPNAME_JITTE,
				WEPNAME_ZR68C, WEPNAME_ZR68S, WEPNAME_SUPA7,
				WEPNAME_M41, WEPNAME_M41S, WEPNAME_MX,
				WEPNAME_MX_SILENCED, WEPNAME_AA13, WEPNAME_SRS
			};
			strcopy(out_weapon, maxlen, weapons_primary[assault_lieutenant_weps[GetRandomInt(0, sizeof(assault_lieutenant_weps) - 1)]]);
		}
	}
	else { // Support
		if (xp < 4) {
			int support_private_weps[] = {
				WEPNAME_MPN, WEPNAME_SRM, WEPNAME_ZR68C,
				WEPNAME_M41, WEPNAME_SUPA7
			};
			strcopy(out_weapon, maxlen, weapons_primary[support_private_weps[GetRandomInt(0, sizeof(support_private_weps) - 1)]]);
		}
		else if (xp < 10) {
			int support_corporal_weps[] = {
				WEPNAME_MPN, WEPNAME_SRM, WEPNAME_ZR68C,
				WEPNAME_M41, WEPNAME_SUPA7, WEPNAME_MX,
				WEPNAME_M41S
			};
			strcopy(out_weapon, maxlen, weapons_primary[support_corporal_weps[GetRandomInt(0, sizeof(support_corporal_weps) - 1)]]);
		}
		else if (xp < 20) {
			int support_sergeant_weps[] = {
				WEPNAME_MPN, WEPNAME_SRM, WEPNAME_ZR68C,
				WEPNAME_M41, WEPNAME_SUPA7, WEPNAME_MX,
				WEPNAME_M41S, WEPNAME_MX_SILENCED
			};
			strcopy(out_weapon, maxlen, weapons_primary[support_sergeant_weps[GetRandomInt(0, sizeof(support_sergeant_weps) - 1)]]);
		}
		else {
			int support_lieutenant_weps[] = {
				WEPNAME_MPN, WEPNAME_SRM, WEPNAME_ZR68C,
				WEPNAME_M41, WEPNAME_SUPA7, WEPNAME_MX,
				WEPNAME_M41S, WEPNAME_MX_SILENCED, WEPNAME_PZ
			};
			strcopy(out_weapon, maxlen, weapons_primary[support_lieutenant_weps[GetRandomInt(0, sizeof(support_lieutenant_weps) - 1)]]);
		}
	}
}

stock void AddToArr(int[] array, int& num_elements, const int value)
{
    for (int i = 0; i < num_elements; ++i) {
        if (array[i] == value) {
            return;
        }
    }
    array[num_elements++] = value;
}

stock void RemoveFromArr(int[] array, int& num_elements, const int value)
{
    for (int i = 0; i < num_elements; ++i) {
        if (array[i] == value) {
            for (int j = i + 1; j < num_elements; ++j) {
                array[j - 1] = array[j]; // move each superseding element back
            }
            // clear the trailing copy of the final moved element
            array[--num_elements] = 0;
            return;
        }
    }
}

int Debug_GetSomeBotClient()
{
	for (int client = 1; client <= MaxClients; ++client) {
		if (IsClientInGame(client) && IsBotClient(client)) {
			return client;
		}
	}
	return 0;
}

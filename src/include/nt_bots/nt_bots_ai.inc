#if !defined(_NT_BOTS_AI_INC_)
#define _NT_BOTS_AI_INC_

static float _dir[NEO_MAX_PLAYERS+1][3];
static float _pos[NEO_MAX_PLAYERS+1][3];
static float _pos_offset[] = { 0.0, 0.0, 32.0 };

public void ThinkHook(int client)
{
#define PITCH 0
#define YAW 1
#define ROLL 2
	//s_ang[PITCH] = 0.0;
	//s_ang[ROLL] = 0.0;
	//
	//float maxAng = 145.0;
	//if ((lastAng > maxAng) || (lastAng < -maxAng)) {
	//	turnStep = -turnStep;
	//}
	//lastAng += turnStep;
	//
	//float scaleDown = 0.5;
	//
	//s_ang[YAW] = lastAng * scaleDown;
	//s_ang[PITCH] = lastAng * 0.5 * scaleDown;
}

float lastPos[3];
int lastStuckTraceTime = 0;
bool CheckIfStuck(int client)
{
	float currentPos[3], currentAng[3];
	GetClientAbsOrigin(client, currentPos);
	GetClientAbsAngles(client, currentAng);
	
	float result = 0.0;
	if (GetVectorLength(lastPos) != 0) {
		result = GetVectorDistance(lastPos, currentPos);
	}
	lastPos = currentPos;
	
	// Are we not moving enough?
	if (result < 0.5) {
		return true;
	}
	
	int thisTime = GetTime();
	int deltaTime = thisTime - lastStuckTraceTime;
	if (deltaTime > 10) {
		lastStuckTraceTime = thisTime;
		
		float xyz[3];
		Handle ray = TR_TraceRayFilterEx(currentPos, currentAng, MASK_OPAQUE,
			RayType_Infinite, DidNotHitSelf, client);
		if (TR_DidHit(ray) /*&& !TR_AllSolid(ray)*/) {
			TR_GetEndPosition(xyz, ray);
		}
		delete ray;
		
		if (!IsNullVector(xyz)) {
			if (GetVectorDistance(currentPos, xyz) < 64.0) {
				return true;
			}
		}
	}
	
	return false;
}

float lastVal = 0.0;
// returns buttons
int DecideVelocity(int client, float vel[3])
{
#define FORWARD 0
#define RIGHT 1
#define UP 2
	
	GetClientAbsOrigin(client, _pos[client]);
	AddVectors(_pos[client], _pos_offset, _pos[client]);
	
	SubtractVectors(s_targetPos, _pos[client], _dir[client]);
	
	float absAng[3];
	GetClientAbsAngles(client, absAng);
	
	bool isStuck = CheckIfStuck(client);
	
	GetVectorAngles(_dir[client], s_ang);
	s_ang[1] += g_cAngleOffset.FloatValue;
	if (isStuck) {
		float randAng = Lerp(lastVal, GetRandomFloat(lastVal, lastVal + 5.0));
		if (FloatAbs(randAng) > 360) {
			randAng = 0.0;
		}
		lastVal = randAng;
		s_ang[1] += randAng;
	}
	
#if(0)
	// Spin angle test
	static int rotate = 0;
	rotate = (rotate + 1) % 360;
	s_ang[1] += rotate;
#endif
	
	// Offset the angle we're moving towards
	float fwd[3], right[3], up[3], res[3];
	GetAngleVectors(s_ang, fwd, right, up);
	
	AddVectors(fwd, right, res);
	AddVectors(res, up, res);
	
	// Offset the angle we're looking towards
	GetAngleVectors(absAng, fwd, right, up);
	
	AddVectors(fwd, right, res);
	AddVectors(res, up, res);
	NormalizeVector(res, res);
	
	float dist = GetVectorDistance(_pos[client], s_targetPos);
	
	int color[] = { 235, 50, 100, 255 };
			
	if (dist > 100.0) {
		
	} else {
		_dir[client] = NULL_VECTOR;
		color[0] = 0;
		color[1] = 100;
		color[2] = 75;
	}
	
	MultiplyVectors(_dir[client], res, _dir[client]);
	SetRandomSeed(GetTime());
	float randX = GetRandomFloat(0.5, 10.0) * GetRandomFloat(_dir[client][RIGHT] * 1.5, _dir[client][RIGHT]);
	SetRandomSeed(GetTime() - 10);
	float randY = GetRandomFloat(0.5, 10.0) * GetRandomFloat(-_dir[client][FORWARD], _dir[client][FORWARD]);
	_dir[client][FORWARD] += randX;
	_dir[client][RIGHT] += randY;
	
	TE_SetupBeamPoints(_pos[client], s_targetPos, rayIndex, rayIndex,
		0, 1, 1.0, 2.0, 2.5, 5, 1.0, color, 5);
	TE_SendToAll();
	
	vel = _dir[client];
	
	if (isStuck) {
		if (dist > 100.0) {
			return IN_JUMP;
		}
	}
	return 0;
}

#endif // _NT_BOTS_AI_INC_
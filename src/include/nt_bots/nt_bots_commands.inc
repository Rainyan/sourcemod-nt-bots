#if defined(_NT_BOTS_COMMANDS_INC_)
#endinput
#endif
#define _NT_BOTS_COMMANDS_INC_

public Action Cmd_OrderPos(int client, int argc)
{
	int bot_client = Debug_GetSomeBotClient();
	if (bot_client == 0) {
		ReplyToCommand(client, "%s No active bots found.", BOTS_MSG_PREFIX);
		return Plugin_Handled;
	}
	
	float target_pos[3];
	
	if (argc == 3) {
		decl String:arg[4];
		for (int coord = 0; coord < 3; ++coord) {
			if (GetCmdArg(1 + coord, arg, sizeof(arg)) < 1) {
				ReplyToCommand(client, "%s Unknown position", BOTS_MSG_PREFIX);
				return Plugin_Handled;
			} else {
				target_pos[coord] = StringToFloat(arg);
			}
		}
	} else {
		if (client == 0) {
			ReplyToCommand(client, "%s Need to provide X Y Z coordinates if \
invoking as server.", BOTS_MSG_PREFIX);
			return Plugin_Handled;
		} else if (argc != 0) {
				ReplyToCommand(client, "%s Either provide X Y Z coords, or \
none to raycast from position.", BOTS_MSG_PREFIX);
				return Plugin_Handled;
		} else {
			float eyePos[3], eyeAng[3];
			GetClientEyePosition(client, eyePos);
			GetClientEyeAngles(client, eyeAng);
			
			TR_TraceRayFilter(eyePos, eyeAng, MASK_OPAQUE, RayType_Infinite, DidNotHitSelf, client);
			if (TR_DidHit() /*&& !TR_AllSolid()*/) {
				TR_GetEndPosition(target_pos);
			} else {
				ReplyToCommand(client, "%s Indeterminate trace", BOTS_MSG_PREFIX);
				return Plugin_Handled;
			}
		}
	}
	
	ClearPlan(bot_client);
	float bot_pos[3];
	GetClientAbsOrigin(bot_client, bot_pos);
	RequestPath(bot_client, bot_pos, target_pos);
	
	ReplyToCommand(client, "%s Moving to %f, %f, %f", BOTS_MSG_PREFIX,
		target_pos[0], target_pos[1], target_pos[2]);
	
	return Plugin_Handled;
}

public Action Cmd_SetPos(int client, int argc)
{
	if (argc != 3) {
		ReplyToCommand(client, "%s Need to provide X Y Z coords",
			BOTS_MSG_PREFIX);
		return Plugin_Handled;
	}
	
	int bot_client = Debug_GetSomeBotClient();
	if (bot_client == 0) {
		ReplyToCommand(client, "%s No active bots found.", BOTS_MSG_PREFIX);
		return Plugin_Handled;
	}
	
	float xyz[3];
	
	decl String:arg[4];
	for (int coord = 0; coord < 3; ++coord) {
		if (GetCmdArg(1 + coord, arg, sizeof(arg)) < 1) {
			ReplyToCommand(client, "%s Unknown position", BOTS_MSG_PREFIX);
			return Plugin_Handled;
		} else {
			xyz[coord] = StringToFloat(arg);
		}
	}
	
	TeleportEntity(bot_client, xyz, NULL_VECTOR, NULL_VECTOR);
	
	return Plugin_Handled;
}

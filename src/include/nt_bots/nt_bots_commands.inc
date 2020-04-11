#if !defined(_NT_BOTS_COMMANDS_INC_)
#define _NT_BOTS_COMMANDS_INC_

#define BOTS_MSG_PREFIX "[BOT]"

public Action Cmd_OrderPos(int client, int argc)
{
    float xyz[3];
    
    if (argc == 3) {
        decl String:arg[4];
        for (int coord = 0; coord < 3; ++coord) {
            if (GetCmdArg(1 + coord, arg, sizeof(arg)) < 1) {
                ReplyToCommand(client, "Unknown position");
                return Plugin_Handled;
            } else {
                xyz[coord] = StringToFloat(arg);
            }
        }
        s_targetPos = xyz;
    } else {
        if (client == 0) {
            ReplyToCommand(client, "Need to provide X Y Z coordinates if invoking as server.");
            return Plugin_Handled;
        } else if (argc != 0) {
                ReplyToCommand(client, "Either provide X Y Z coords, or none to raycast from position.");
                return Plugin_Handled;
        } else {
            float eyePos[3], eyeAng[3];
            GetClientEyePosition(client, eyePos);
            GetClientEyeAngles(client, eyeAng);
            
            Handle ray = TR_TraceRayFilterEx(eyePos, eyeAng, MASK_OPAQUE,
                RayType_Infinite, DidNotHitSelf, client);
            if (TR_DidHit(ray) /*&& !TR_AllSolid(ray)*/) {
                TR_GetEndPosition(xyz, ray);
            } else {
                ReplyToCommand(client, "%s Indeterminate trace", BOTS_MSG_PREFIX);
                delete ray;
                return Plugin_Handled;
            }
            delete ray;
        }
    }
    
    s_targetPos = xyz;
    
    ReplyToCommand(client, "%s Moving to %f, %f, %f", BOTS_MSG_PREFIX,
        s_targetPos[0], s_targetPos[1], s_targetPos[2]);
    
    return Plugin_Handled;
}

public Action Cmd_SetPos(int client, int argc)
{
    float xyz[3];
    
    if (argc != 3) {
        ReplyToCommand(client, "Need to provide X Y Z coords");
        return Plugin_Handled;
    }
    
    decl String:arg[4];
    for (int coord = 0; coord < 3; ++coord) {
        if (GetCmdArg(1 + coord, arg, sizeof(arg)) < 1) {
            ReplyToCommand(client, "Unknown position");
            return Plugin_Handled;
        } else {
            xyz[coord] = StringToFloat(arg);
        }
    }
    
    for (int i = 1; i <= MaxClients; ++i) {
        if (IsClientInGame(i) && IsBotClient(i)) {
            TeleportEntity(i, xyz, NULL_VECTOR, NULL_VECTOR);
            break;
        }
    }
    
    return Plugin_Handled;
}

#endif // _NT_BOTS_COMMANDS_INC_
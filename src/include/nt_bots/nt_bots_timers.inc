#if !defined(_NT_BOTS_TIMERS_INC_)
#define _NT_BOTS_TIMERS_INC_

// Generic timer placeholder for repeat updates
// TODO: Is this needed? Relevant hook replacements?
public Action Timer_Generic(Handle timer, any data)
{
    for (int client = 1; client <= MaxClients; ++client) {
        if (!IsClientConnected(client) || !IsBotClient(client)) {
            continue;
        }
        // Do something
    }
    return Plugin_Continue;
}

#endif // _NT_BOTS_TIMERS_INC_
#if !defined(_NT_BOTS_BASE_INC_)
#define _NT_BOTS_BASE_INC_

// Is this client a bot that we want to use for bot AI?
// Assumes a valid client index and IsClientConnected == true.
bool IsBotClient(int client)
{
    return (IsFakeClient(client) && !IsClientSourceTV(client));
}

// Predicate for client position initiated ray trace.
bool DidNotHitSelf(int self, int contentsMask, int target)
{
    return self != target;
}

void MultiplyVectors(float v1[3], float v2[3], float res[3])
{
    res[0] = v1[0] * v2[0];
    res[1] = v1[1] * v2[1];
    res[2] = v1[2] * v2[2];
}

float Lerp(float a, float b, float scale = 0.0)
{
    if (scale == 0) {
        scale = GetGameFrameTime();
    }
    return a + (b - a) * scale;
}

#endif // _NT_BOTS_BASE_INC_
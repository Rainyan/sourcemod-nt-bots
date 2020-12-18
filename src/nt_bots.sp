#pragma semicolon 1

#include <sourcemod>
#include <sdkhooks>
#include <sdktools>

#include <neotokyo>

#include "include/nt_bots/nt_bots_shared.inc"
#include "include/nt_bots/nt_bots_base.inc"
#include "include/nt_bots/nt_bots_commands.inc"
#include "include/nt_bots/nt_bots_timers.inc"
#include "include/nt_bots/nt_bots_ai.inc"
#include "include/nt_bots/nt_bots_natives.inc"
#include "include/nt_bots/nt_bots_database.inc"

public Plugin myinfo = {
    name = "NT Bots",
    description = "Bots for Neotokyo. WIP.",
    author = "Rain",
    version = PLUGIN_VERSION,
    url = "https://github.com/Rainyan/sourcemod-nt-bots"
};
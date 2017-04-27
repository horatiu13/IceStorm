#pragma once

#include "global_data.h"
#include "icecommon.h"

_Success_(return == ERROR_SUCCESS)
DWORD
InitAppCtrlRules(
    VOID
);

_Success_(return == ERROR_SUCCESS)
DWORD
UninitAppCtrlRules(
    VOID
);

_Success_(return == ERROR_SUCCESS)
DWORD
GetAppCtrlScanResult(
    _In_    IC_APPCTRL_RULE                        *PRule,
    _Inout_ ICE_APP_CTRL_SCAN_RESULT_PACKET        *PResultPack
);

_Success_(ERROR_SUCCESS == return)
DWORD
AddAppCtrlRule(
    _In_opt_z_  PWCHAR                      PProcessPath,
    _In_opt_    DWORD                       DwPid,
    _In_opt_z_  PWCHAR                      PParentPath,
    _In_opt_    DWORD                       DwParentPid,
    _In_        ICE_SCAN_VERDICT            Verdict,
    _Inout_opt_ DWORD                      *PDwRuleId
);

_Success_(ERROR_SUCCESS == return)
DWORD
DeleteAppCtrlRule(
    _In_        DWORD                       DwRuleId
);

_Success_(ERROR_SUCCESS == return)
DWORD
UpdateAppCtrlRule(
    _In_        DWORD                       DwRuleId,
    _In_opt_z_  PWCHAR                      PProcessPath,
    _In_opt_    DWORD                       DwPid,
    _In_opt_z_  PWCHAR                      PParentPath,
    _In_opt_    DWORD                       DwParentPid,
    _In_        ICE_SCAN_VERDICT            Verdict
);

_Success_(ERROR_SUCCESS == return)
DWORD
GetAppCtrlRules(
    _Inout_     PIC_APPCTRL_RULE           *PPRules,
    _Inout_     DWORD                      *PDwLength
);

_Success_(ERROR_SUCCESS == return)
VOID
FreeAppCtrlRulesList(
    _Inout_     PIC_APPCTRL_RULE           PRules,
    _In_        DWORD                      DwLength
);

#include "ice_app_ctrl_scan.h"
#include "ice_user_common.h"
#include "global_data.h"
#include "defines.h"
#include "process_op.h"

PICE_GENERIC_PACKET IceAllocRequest(_In_ ULONG UlSize, _In_ ULONG Tag);
ULONG IceGetScanRequestSize(_In_ PEPROCESS PProcess, _In_ HANDLE HProcessId, _In_ PUNICODE_STRING PUSProcPath, _In_ PUNICODE_STRING PUSParentPath);
NTSTATUS IceBuildScanRequest(_In_ PEPROCESS PProcess, _In_ HANDLE HProcessId, _In_ PPS_CREATE_NOTIFY_INFO PCreateInfo, 
    _In_ PUNICODE_STRING PUSProcPath, _In_opt_ PUNICODE_STRING PUSParentPath, _Inout_ PICE_APP_CTRL_SCAN_REQUEST_PACKET PScanRequest);

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, IceAppCtrlScanProcess)
    #pragma alloc_text(PAGE, IceBuildScanRequest)
    #pragma alloc_text(PAGE, IceGetScanRequestSize)
    #pragma alloc_text(PAGE, IceAllocRequest)
#endif

PICE_GENERIC_PACKET
IceAllocRequest(
    _In_    ULONG                           UlSize,
    _In_    ULONG                           Tag
)
{
    PICE_GENERIC_PACKET pPack = NULL;

    pPack = (PICE_GENERIC_PACKET) ExAllocatePoolWithTag(PagedPool, UlSize, Tag);

    return pPack;
}

FORCEINLINE
ULONG
IceGetScanRequestSize(
    _In_    PEPROCESS                           PProcess,
    _In_    HANDLE                              HProcessId,
    _In_    PUNICODE_STRING                     PUSProcPath,
    _In_    PUNICODE_STRING                     PUSParentPath
)
{
    ULONG ulSize = 0;

    PAGED_CODE();

    PProcess, HProcessId;

    ulSize += sizeof(ICE_GENERIC_PACKET);
    ulSize += sizeof(ICE_APP_CTRL_SCAN_REQUEST_PACKET);
    
    ulSize += PUSProcPath->Length;
    ulSize += PUSParentPath->Length;
    
    return ulSize;
}

NTSTATUS
IceBuildScanRequest(
    _In_        PEPROCESS                           PProcess,
    _In_        HANDLE                              HProcessId,
    _In_        PPS_CREATE_NOTIFY_INFO              PCreateInfo,
    _In_        PUNICODE_STRING                     PUSProcPath,
    _In_opt_    PUNICODE_STRING                     PUSParentPath,
    _Inout_     PICE_APP_CTRL_SCAN_REQUEST_PACKET   PScanRequest

)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    PAGED_CODE();

    if (NULL == PProcess || NULL == HProcessId || NULL == PUSProcPath || NULL == PScanRequest || NULL == PCreateInfo)
    {
        LogErrorNt(STATUS_INVALID_PARAMETER, "(NULL == PProcess || NULL == HProcessId || NULL == PUSProcPath || NULL == PScanRequest || NULL == PCreateInfo)");
        return STATUS_INVALID_PARAMETER;
    }


    RtlCopyMemory(PScanRequest->PStrings, PUSProcPath->Buffer, PUSProcPath->Length);
    RtlCopyMemory(((PCHAR) PScanRequest->PStrings) + PUSProcPath->Length, PUSParentPath->Buffer, PUSParentPath->Length);

    PScanRequest->DwPid             = (DWORD) (SIZE_T) HProcessId;
    PScanRequest->DwProcessPathSize = PUSProcPath->Length;
    PScanRequest->DwParentPid       = (DWORD) (SIZE_T) PCreateInfo->ParentProcessId;
    PScanRequest->DwParentPathSize  = PUSParentPath->Length;

    LogInfo("---- Scan Request ----");
    LogInfo("Proc (%d) [%wZ]", PScanRequest->DwPid, PUSProcPath);
    LogInfo("ParentProc (%d) [%wZ]", PScanRequest->DwParentPid, PUSParentPath);

    return ntStatus;
}

_Use_decl_anno_impl_
NTSTATUS
IceAppCtrlScanProcess(
    PEPROCESS                               PProcess,
    HANDLE                                  HProcessId,
    PPS_CREATE_NOTIFY_INFO                  PCreateInfo,
    PNTSTATUS                               PNtScanResult
)
{
    NTSTATUS                                ntStatus            = STATUS_SUCCESS;
    NTSTATUS                                ntScanResult        = STATUS_SUCCESS;
    ULONG                                   ulPacketLength      = 0;
    ULONG                                   ulResponseLength    = 0;
    PICE_GENERIC_PACKET                     pPacket             = NULL;
    LARGE_INTEGER                           liTimeout           = { 0 };
    PUNICODE_STRING                         pParentPath         = NULL;
    PUNICODE_STRING                         pProcPath           = NULL;
    BYTE                                    pResponseBuffer[sizeof(FILTER_REPLY_HEADER) + sizeof(ICE_GENERIC_PACKET) + sizeof(ICE_APP_CTRL_SCAN_RESULT_PACKET)] = { 0 };
    PFILTER_REPLY_HEADER                    pReplyHeader        = NULL;
    PICE_GENERIC_PACKET                     pResponsePacket     = NULL;
    PICE_APP_CTRL_SCAN_RESULT_PACKET        pResponseVerdict    = NULL;
    PICE_APP_CTRL_SCAN_REQUEST_PACKET       pScanRequest        = NULL;
    
    PAGED_CODE();

    if (NULL == PProcess || NULL == HProcessId || NULL == PCreateInfo || NULL == PNtScanResult)
    {
        LogErrorNt(STATUS_INVALID_PARAMETER, "(NULL == PProcess || NULL == HProcessId || NULL == PCreateInfo || NULL == PNtScanResult)");
        return STATUS_INVALID_PARAMETER;
    }

    __try
    {
        if (NULL == gPIceComPorts || NULL == gPIceComPorts->PClientAppCtrlPort || 0 == gPIceComPorts->LNrOfConnections)
        {
            LogInfo("No client connected");
            __leave;
        }

        // iau info suplimentare
        if (PCreateInfo->FileOpenNameAvailable)
        {
            pProcPath = (PUNICODE_STRING) PCreateInfo->ImageFileName;
        }
        else
        {
            ntStatus = IceGetProcessPathByPid(HProcessId, &pProcPath);
            if (!NT_SUCCESS(ntStatus))
            {
                LogErrorNt(ntStatus, "IceGetProcessPathByPid(%d)", (DWORD) HProcessId);
                __leave;
            }
        }

        IceGetProcessPathByPid(PCreateInfo->ParentProcessId, &pParentPath);

        // construiesc pachetul pentru scanare
        ulPacketLength = IceGetScanRequestSize(PProcess, HProcessId, pProcPath, pParentPath);
        pPacket = IceAllocRequest(ulPacketLength, TAG_ICSP);
        if (NULL == pPacket)
        {
            ntScanResult = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        LogInfo("ulPacketLength: %d", ulPacketLength);
        pPacket->DwPacketLength = ulPacketLength;
        pPacket->DwRequestType = ICE_FILTER_REQUEST_SCAN_PROCESS;
        pScanRequest = (PICE_APP_CTRL_SCAN_REQUEST_PACKET) (pPacket + 1);

        ntStatus = IceBuildScanRequest(PProcess, HProcessId, PCreateInfo, pProcPath, pParentPath, pScanRequest);
        if (!NT_SUCCESS(ntStatus))
        {
            LogErrorNt(ntStatus, "IceBuildScanRequest");
            __leave;
        }
        
        // trimit requestul
        liTimeout.QuadPart = (-10000LL * 500); //-10000LL * 5000


        pReplyHeader = (PFILTER_REPLY_HEADER) pResponseBuffer;
        pResponsePacket = (PICE_GENERIC_PACKET) (pReplyHeader + 1);
        ulResponseLength = sizeof(pResponseBuffer);

        LogInfo("ulResponseLength: %d", ulResponseLength);
        ntStatus = FltSendMessage(gPData->PFilter, &gPIceComPorts->PClientAppCtrlPort, pPacket, pPacket->DwPacketLength, 
            pResponsePacket, &ulResponseLength, &liTimeout);
        if (!NT_SUCCESS(ntStatus) || ntStatus == STATUS_TIMEOUT)
        {
            LogErrorNt(ntStatus, "FltSendMessage, ulResponseLength: %d", ulResponseLength);
            __leave;
        }

        if (ulResponseLength != (sizeof(pResponseBuffer) - sizeof(FILTER_REPLY_HEADER)) || pResponsePacket->DwPacketLength != ulResponseLength)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            LogErrorNt(ntStatus, "(ulResponseLength != sizeof(pResponseBuffer) || pResponsePacket->DwPacketLength != sizeof(pResponseBuffer))");
            LogError("ulResponseLength: %d, sizeof(pResponseBuffer): %d, pResponsePacket->DwPacketLength: %d", ulResponseLength, sizeof(pResponseBuffer), pResponsePacket->DwPacketLength);
            __leave;
        }

        pResponseVerdict = (PICE_APP_CTRL_SCAN_RESULT_PACKET) (pResponsePacket + 1);
        LogInfo("Recived status: %d", pResponseVerdict->NtScanResult);
        ntScanResult = pResponseVerdict->NtScanResult;
    }
    __finally
    {
        if (NULL != pPacket)
        {
            ExFreePoolWithTag(pPacket, TAG_ICSP);
            pPacket = NULL;
        }

        if (NULL != pParentPath)
        {
            ExFreePoolWithTag(pParentPath, TAG_ICPP);
            pParentPath = NULL;
        }

        if (NULL != pProcPath && pProcPath != PCreateInfo->ImageFileName)
        {
            ExFreePoolWithTag(pProcPath, TAG_ICPP);
            pProcPath = NULL;
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        *PNtScanResult = ntScanResult;
    }

    return ntStatus;
}

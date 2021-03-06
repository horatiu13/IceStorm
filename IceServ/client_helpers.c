#define WIN32_LEAN_AND_MEAN

#include "debug2.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "client_helpers.h"
#include "global_data.h"

#undef WIN32_LEAN_AND_MEAN

_Success_(return == ERROR_SUCCESS)
DWORD
ClSendAuxBuffer(
    _In_        PBYTE           PBuffer,
    _In_        DWORD           DwBufferSize
)
{
    INT32       iResult         = 0;
    INT32       iMessageLen     = 0;
    PCHAR       pAuxBuffer      = 0;
    
    iMessageLen = DwBufferSize;
    pAuxBuffer = (PCHAR) PBuffer;

    while (TRUE)
    {
        iResult = send(gServerSocket, pAuxBuffer, iMessageLen, 0);
        if (iResult == SOCKET_ERROR)
        {
            DWORD dwResult = WSAGetLastError();
            LogErrorWin(dwResult, L"send");
            return dwResult;
        }
        LogInfo(L"Sent: %d / %d bytes", iResult, iMessageLen);
        if (iResult == iMessageLen) break;

        iMessageLen -= iResult;
        pAuxBuffer += iResult;
    }

    return ERROR_SUCCESS;
}

_Use_decl_anno_impl_
DWORD
ClSendMessage(
    PBYTE           PBuffer,
    DWORD           DwBufferSize
)
{
    DWORD dwResult = ERROR_SUCCESS;

    if (ERROR_SUCCESS != (dwResult = ClSendAuxBuffer((PBYTE) &DwBufferSize, (DWORD) sizeof(DWORD)))) return dwResult;
    
    if (ERROR_SUCCESS != (dwResult = ClSendAuxBuffer(PBuffer, DwBufferSize))) return dwResult;

    return ERROR_SUCCESS;
}

_Use_decl_anno_impl_
DWORD
ClSendString(
    PWCHAR          PString
)
{
    DWORD dwResult  = ERROR_SUCCESS;
    DWORD dwSize    = (NULL == PString) ? 0 : (DWORD) (wcslen(PString) * sizeof(WCHAR));

    if (ERROR_SUCCESS != (dwResult = ClSendDWORD(dwSize))) return dwResult;
    
    //LogInfo(L"Sending string: %s", PString);
    if (dwSize != 0)
    {
        if (ERROR_SUCCESS != (dwResult = ClSendAuxBuffer((PBYTE) PString, dwSize))) return dwResult;
        //LogInfo(L"Sent STRING: %s", PString);
    }

    return ERROR_SUCCESS;
}

_Use_decl_anno_impl_
DWORD
ClSendDWORD(
    DWORD           DwValue
)
{
    DWORD dwResult  = ERROR_SUCCESS;
    
    if (ERROR_SUCCESS != (dwResult = ClSendAuxBuffer((PBYTE) &DwValue, sizeof(DWORD)))) return dwResult;
    LogInfo(L"Sent DWORD: %d", DwValue);
    return ERROR_SUCCESS;
}

_Success_(return == ERROR_SUCCESS)
DWORD
ClRecvAuxBuffer(
    _Inout_bytecap_(DwBufferSize)   PBYTE           PBuffer,
    _In_                            DWORD           DwBufferSize,
    _In_                            DWORD           DwFlags
)
{
    INT32       iResult     = 0;
    INT32       iMessageLen = 0;
    INT32       iError      = 0;
    PCHAR       pAuxBuffer  = 0;
    DWORD       dwResult    = ERROR_SUCCESS;

    iMessageLen = DwBufferSize;
    pAuxBuffer = (PCHAR) PBuffer;

    while (TRUE)
    {
        iResult = recv(gServerSocket, pAuxBuffer, iMessageLen, DwFlags);
        if (iResult == SOCKET_ERROR)
        {
            iError = WSAGetLastError();
            if (WSAEWOULDBLOCK == iError)
            {
                if (WAIT_OBJECT_0 == WaitForSingleObject(gHStopEvent, 50))
                {
                    dwResult = ERROR_CANCELLED;
                    break;
                }
                continue;
            }
            
            dwResult = iError;
            LogErrorWin(dwResult, L"recv");
            break;
            
        }

        if (iResult == 0)
        {
            if (WAIT_OBJECT_0 == WaitForSingleObject(gHStopEvent, 100))
            {
                LogInfo(L"Stop event signaled");
                dwResult = ERROR_CANCELLED;
                break;
            }
            continue;
        }

        LogInfo(L"Received: %d / %d bytes", iResult, iMessageLen);

        if (iResult == iMessageLen) break;

        iMessageLen -= iResult;
        pAuxBuffer += iResult;
    }

    return dwResult;
}

_Use_decl_anno_impl_
DWORD
ClRecvMessage(
    PBYTE           PBuffer,
    DWORD           DwMexBufferSize
)
{
    DWORD           dwResult            = ERROR_SUCCESS;
    BOOLEAN         bOverflow           = FALSE;
    DWORD           dwMessageSize       = 0;
    DWORD           dwRemainingSize     = 0;
    DWORD           dwSizeToReceive     = 0;

    __try
    {
        dwResult = ClRecvDWORD(&dwMessageSize);
        if (ERROR_SUCCESS != dwResult)
        {
            LogErrorWin(dwResult, L"Failed to get message size");
            __leave;
        }
        
        LogInfo(L"Received size: %d", dwMessageSize);
        int a = 0;
        scanf_s("%d", &a);

        if (dwMessageSize <= DwMexBufferSize)
        {
            dwResult = ClRecvMessageWithoutSize(PBuffer, dwMessageSize);
            if (ERROR_SUCCESS != dwResult)
            {
                LogErrorWin(dwResult, L"Failed to get the message");
                __leave;
            }
            __leave;
        }

        bOverflow = TRUE;
        dwRemainingSize = dwMessageSize;
        dwSizeToReceive = DwMexBufferSize;

        while (0 != dwRemainingSize)
        {
            dwResult = ClRecvMessageWithoutSize(PBuffer, dwSizeToReceive);
            if (ERROR_SUCCESS != dwResult)
            {
                LogErrorWin(dwResult, L"Failead to receive part of the message");
                __leave;
            }

            dwRemainingSize -= dwSizeToReceive;
            dwSizeToReceive = ((dwRemainingSize > DwMexBufferSize) ? DwMexBufferSize : dwRemainingSize);
        }
    }
    __finally
    {
        if (ERROR_SUCCESS == dwResult && bOverflow)
        {
            dwResult = ERROR_BUFFER_OVERFLOW;
        }
    }

    return dwResult;
}

_Use_decl_anno_impl_
DWORD
ClGetNextMessageSize(
    DWORD           *PDwNextSize
)
{
    return ClRecvAuxBuffer((PBYTE) PDwNextSize, sizeof(DWORD), MSG_PEEK);
}

_Use_decl_anno_impl_
DWORD
ClRecvMessageWithoutSize(
    PBYTE           PBuffer,
    DWORD           DwBufferSize
)
{
    return ClRecvAuxBuffer(PBuffer, DwBufferSize, 0);
}

_Use_decl_anno_impl_
DWORD
ClRecvDWORD(
    DWORD           *PDwValue
)
{
    DWORD dwValue   = 0;
    DWORD dwStatus  = ERROR_SUCCESS;

    if (ERROR_SUCCESS != (dwStatus = ClRecvMessageWithoutSize((PBYTE) &dwValue, sizeof(DWORD)))) return dwStatus;
    LogInfo(L"Received DWORD: %d", dwValue);
    *PDwValue = dwValue;

    return dwStatus;
}

_Use_decl_anno_impl_
DWORD
ClRecvString(
    PWCHAR        *PPString
)
{
    DWORD       dwSize      = 0;
    DWORD       dwStatus    = ERROR_SUCCESS;

    if (ERROR_SUCCESS != (dwStatus = ClRecvDWORD(&dwSize))) return dwStatus;
    if (0 == dwSize) return ERROR_SUCCESS;

    *PPString = (PWCHAR) malloc(dwSize + sizeof(WCHAR));
    if (*PPString == NULL) return ERROR_NOT_ENOUGH_MEMORY;
    RtlSecureZeroMemory(*PPString, dwSize + sizeof(WCHAR));

    if (ERROR_SUCCESS != (dwStatus = ClRecvMessageWithoutSize((PBYTE) *PPString, dwSize))) return dwStatus;

    return ERROR_SUCCESS;
}

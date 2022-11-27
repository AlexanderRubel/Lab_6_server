//#include <Windows.h>

//#include <bthsdpdef.h>
//#include <bluetoothapis.h>

#include <WinSock2.h>
#include <ws2bth.h>
#include <stdio.h>
#include <vector>
#include <initguid.h>
#include <strsafe.h>
//#include <fstream>
#include <iostream>
#include <stdlib.h>

#include "Device.h"

#pragma comment(lib, "ws2_32.lib")

DEFINE_GUID(g_guidServiceClass, 0xb62c4e8d, 0x62cc, 0x404b, 0xbb, 0xbf, 0xbf, 0x3e, 0x3b, 0xbb, 0x13, 0x74);

#define CXN_TEST_DATA_STRING              (L"~!@#$%^&*()-_=+?<>1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
#define CXN_TRANSFER_DATA_LENGTH          (sizeof(CXN_TEST_DATA_STRING))


#define CXN_BDADDR_STR_LEN                17   // 6 two-digit hex values plus 5 colons
#define CXN_MAX_INQUIRY_RETRY             3
#define CXN_DELAY_NEXT_INQUIRY            15
#define CXN_SUCCESS                       0
#define CXN_ERROR                         1
#define CXN_DEFAULT_LISTEN_BACKLOG        4
wchar_t g_szRemoteName[BTH_MAX_NAME_SIZE + 1] = { 0 };  // 1 extra for trailing NULL character
wchar_t g_szRemoteAddr[CXN_BDADDR_STR_LEN + 1] = { 0 }; // 1 extra for trailing NULL character
int  g_ulMaxCxnCycles = 1;
#define CXN_INSTANCE_STRING L"Sample Bluetooth Server"

int __cdecl main()
{
    setlocale(LC_ALL, "russian");
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        /* Tell the user that we could not find a usable */
        /* Winsock DLL.                                  */
        printf("WSAStartup failed with error: %d\n", err);
        return 1;
    }

    /* Confirm that the WinSock DLL supports 2.2.*/
    /* Note that if the DLL supports versions greater    */
    /* than 2.2 in addition to 2.2, it will still return */
    /* 2.2 in wVersion since that is the version we      */
    /* requested.                                        */

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        printf("Could not find a usable version of Winsock.dll\n");
        WSACleanup();
        return 1;
    }
    else
        printf("The Winsock 2.2 dll was found okay\n");


    /* The Winsock DLL is acceptable. Proceed to use it. */

    /* Add network programming using Winsock here */

    /* User code starts here */
    WSAQUERYSET wsaQuery{};
    wsaQuery.dwNameSpace = NS_BTH;
    wsaQuery.dwSize = sizeof(WSAQUERYSET);
    HANDLE hLookup{};
    DWORD  flags = LUP_CONTAINERS | LUP_FLUSHCACHE | LUP_RETURN_NAME | LUP_RETURN_ADDR;

    

    /* Run server mode */
    /* Creating SOCKET */
    LPCSADDR_INFO   lpCSAddrInfo = NULL;
    lpCSAddrInfo = (LPCSADDR_INFO)HeapAlloc(GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        sizeof(CSADDR_INFO));
    if (NULL == lpCSAddrInfo) {
        wprintf(L"!ERROR! | Unable to allocate memory for CSADDR_INFO\n");
    }

    wchar_t         szThisComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD           dwLenComputerName = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerName(szThisComputerName, &dwLenComputerName)) {
        wprintf(L"=CRITICAL= | GetComputerName() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
    }

    SOCKET LocalSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
    if (INVALID_SOCKET == LocalSocket) {
        wprintf(L"=CRITICAL= | socket() call failed. WSAGetLastError = [%d]\n", WSAGetLastError());
    }

    SOCKADDR_BTH    SockAddrBthLocal = { 0 };
    SockAddrBthLocal.addressFamily = AF_BTH;
    SockAddrBthLocal.port = BT_PORT_ANY;

    if (SOCKET_ERROR == bind(LocalSocket,
        (struct sockaddr*)&SockAddrBthLocal,
        sizeof(SOCKADDR_BTH))) {
        wprintf(L"=CRITICAL= | bind() call failed w/socket = [0x%I64X]. WSAGetLastError=[%d]\n", (ULONG64)LocalSocket, WSAGetLastError());
    }

    int iAddrLen = sizeof(SOCKADDR_BTH);
    int ulRetCode = getsockname(LocalSocket,
        (struct sockaddr*)&SockAddrBthLocal,
        &iAddrLen);
    if (SOCKET_ERROR == ulRetCode) {
        wprintf(L"=CRITICAL= | getsockname() call failed w/socket = [0x%I64X]. WSAGetLastError=[%d]\n", (ULONG64)LocalSocket, WSAGetLastError());
        ulRetCode = CXN_ERROR;
    }

    lpCSAddrInfo[0].LocalAddr.iSockaddrLength = sizeof(SOCKADDR_BTH);
    lpCSAddrInfo[0].LocalAddr.lpSockaddr = (LPSOCKADDR)&SockAddrBthLocal;
    lpCSAddrInfo[0].RemoteAddr.iSockaddrLength = sizeof(SOCKADDR_BTH);
    lpCSAddrInfo[0].RemoteAddr.lpSockaddr = (LPSOCKADDR)&SockAddrBthLocal;
    lpCSAddrInfo[0].iSocketType = SOCK_STREAM;
    lpCSAddrInfo[0].iProtocol = BTHPROTO_RFCOMM;

    WSAQUERYSET     wsaQuerySet = { 0 };
    ZeroMemory(&wsaQuerySet, sizeof(WSAQUERYSET));
    wsaQuerySet.dwSize = sizeof(WSAQUERYSET);
    wsaQuerySet.lpServiceClassId = (LPGUID)&g_guidServiceClass;

    HRESULT         res;
    size_t          cbInstanceNameSize = 0;
    res = StringCchLength(szThisComputerName, sizeof(szThisComputerName), &cbInstanceNameSize);
    if (FAILED(res)) {
        wprintf(L"-FATAL- | ComputerName specified is too large\n");
        ulRetCode = CXN_ERROR;
    }

    wchar_t* pszInstanceName = NULL;
    cbInstanceNameSize += sizeof(CXN_INSTANCE_STRING) + 1;
    pszInstanceName = (LPWSTR)HeapAlloc(GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        cbInstanceNameSize);
    if (NULL == pszInstanceName) {
        wprintf(L"-FATAL- | HeapAlloc failed | out of memory | gle = [%d] \n", GetLastError());
        ulRetCode = CXN_ERROR;
    }

    wchar_t comment[] = L"Example Service instance registered in the directory service through RnR";
    StringCbPrintf(pszInstanceName, cbInstanceNameSize, L"%s %s", szThisComputerName, CXN_INSTANCE_STRING);
    wsaQuerySet.lpszServiceInstanceName = pszInstanceName;
    wsaQuerySet.lpszComment = comment;
    wsaQuerySet.dwNameSpace = NS_BTH;
    wsaQuerySet.dwNumberOfCsAddrs = 1;      // Must be 1.
    wsaQuerySet.lpcsaBuffer = lpCSAddrInfo; // Req'd.

    //
    // As long as we use a blocking accept(), we will have a race
    // between advertising the service and actually being ready to
    // accept connections.  If we use non-blocking accept, advertise
    // the service after accept has been called.
    //
    if (SOCKET_ERROR == WSASetService(&wsaQuerySet, RNRSERVICE_REGISTER, 0)) {
        wprintf(L"=CRITICAL= | WSASetService() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
        ulRetCode = CXN_ERROR;
    }

    if (SOCKET_ERROR == listen(LocalSocket, CXN_DEFAULT_LISTEN_BACKLOG)) {
        wprintf(L"=CRITICAL= | listen() call failed w/socket = [0x%I64X]. WSAGetLastError=[%d]\n", (ULONG64)LocalSocket, WSAGetLastError());
        ulRetCode = CXN_ERROR;
    }
   
    FILE* fd = NULL;
    fd = fopen("Redecorate.mp3", "wb");
    if (fd == NULL) {
        perror("fopen");
        return 1;
    }


    SOCKET ClientSocket = INVALID_SOCKET;
    ClientSocket = accept(LocalSocket, NULL, NULL);
    /* User code ends here */
    int lengthReceived = 0;
    float totalReceived = 0;
    do {
        unsigned char buff[1024] = {};
        lengthReceived = recv(ClientSocket, (char*)buff, 1024, 0);
        if (lengthReceived < 1024) {
            totalReceived += lengthReceived / 1024;
        } else {
            totalReceived += 1;
        }
        std::cout << "Received total %d kBytes" << totalReceived << std::endl;
        fwrite(buff, sizeof(char), lengthReceived, fd);
    } while (lengthReceived == 1024);

    fclose(fd);

    if (closesocket(ClientSocket)) {
        wprintf(L"=CRITICAL= | closesocket() call failed w/socket = [0x%I64X]. WSAGetLastError=[%d]\n", (ULONG64)ClientSocket, WSAGetLastError());
    }
    if (closesocket(LocalSocket)) {
        wprintf(L"=CRITICAL= | closesocket() call failed w/socket = [0x%I64X]. WSAGetLastError=[%d]\n", (ULONG64)LocalSocket, WSAGetLastError());
    }


    WSACleanup();

    system("pause");
    return 0;
}
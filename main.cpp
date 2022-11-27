//#include <Windows.h>

//#include <bthsdpdef.h>
//#include <bluetoothapis.h>

#include <WinSock2.h>
#include <ws2bth.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <initguid.h>

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

    if (WSALookupServiceBegin(&wsaQuery, flags, &hLookup) == SOCKET_ERROR) {
        wprintf(L"WSALookupServiceBegin failed with error: %d\n", WSAGetLastError());
        return 1;
    }

    std::vector<Device> devices;
    int i = 0;
    devices.emplace_back();
    
    while (devices[i].getDeviceInfo(hLookup, flags) == NO_ERROR) {
        devices[i].printDeviceInfo();
        devices.emplace_back();

        Sleep(1000);
        wprintf(L"\n");
        i++;
    }
    if (WSAGetLastError() != WSA_E_NO_MORE) {
        devices.pop_back();
    }
    WSALookupServiceEnd(hLookup);

    int devNum = 0;
    wprintf(L"______________________\n");
    wprintf(L"Choose device:\n>");
    std::cin >> devNum;

    /* Run client mode */
    /* Creating SOCKET */
    devices[devNum].pBtSockRemote.addressFamily = AF_BTH;
    devices[devNum].pBtSockRemote.serviceClassId = g_guidServiceClass;
    devices[devNum].pBtSockRemote.port = 0;

    SOCKET localSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
    if (INVALID_SOCKET == localSocket) {
        wprintf(L"=CRITICAL= | socket() call failed. WSAGetLastError = [%d]\n", WSAGetLastError());
    }

    //
    // Connect the socket (pSocket) to a given remote socket represented by address (pServerAddr)
    //
    if (SOCKET_ERROR == connect(localSocket,
        (struct sockaddr*)&devices[devNum].pBtSockRemote,
        sizeof(SOCKADDR_BTH))) {
        wprintf(L"=CRITICAL= | connect() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
    }

    char buf[7] = "Hello!";
    std::cout << "___________________________" << std::endl;
    if (SOCKET_ERROR == send(localSocket, (char*)buf, (int)7, 0)) {
        wprintf(L"=CRITICAL= | send() call failed WSAGetLastError=[%d]\n", WSAGetLastError());
    }

    //
    // Close the socket
    //
    if (SOCKET_ERROR == closesocket(localSocket)) {
        wprintf(L"=CRITICAL= | closesocket() call failed WSAGetLastError=[%d]\n", WSAGetLastError());
    }

    /* User code ends here */

    WSACleanup();

    return 0;
}
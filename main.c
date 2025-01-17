#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <winsock2.h>
#include <VersionHelpers.h>
#include <ws2tcpip.h>
#include <tlhelp32.h>

#define BUFFER_SIZE 30000
#define DEFAULT_PORT 2007
#define DEFAULT_PROTO SOCK_STREAM

int main(int argc, char **argv)
{
    char *server_name= "localhost";
    unsigned short port = DEFAULT_PORT;
    int retval, loopflag = 0;
    int i, loopcount, maxloop=-1;
    unsigned int addr;
    int socket_type = DEFAULT_PROTO;
    struct sockaddr_in server;
    struct hostent *hp;
    WSADATA wsaData;
    SOCKET  conn_socket;
    DWORD dwTemp;
    TCHAR tcMsg[BUFFER_SIZE];
    TCHAR tcBuf[BUFFER_SIZE];
    DWORD dwBufCount = BUFFER_SIZE;

    if ((retval = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
    {
        fprintf(stderr,"Client: WSAStartup() failed with error %d\n", retval);
        WSACleanup();
        return -1;
    }
    else
       printf("Client: WSAStartup() is OK.\n");

    if (port == 0)
    {
        WSACleanup();
        exit(1);
    }

    // Attempt to detect if we should call gethostbyname() or gethostbyaddr()
    if (isalpha(server_name[0]))
    {
        // server address is a name
        hp = gethostbyname(server_name);
    }
    else
    {
        // Convert nnn.nnn address to a usable one
        addr = inet_addr(server_name);
        hp = gethostbyaddr((char *)&addr, 4, AF_INET);
    }

    if (hp == NULL )
    {
        fprintf(stderr,"Client: Cannot resolve address \"%s\": Error %d\n", server_name, WSAGetLastError());
        WSACleanup();
        exit(1);
    }
    else
       printf("Client: gethostbyaddr() is OK.\n");

    // Copy the resolved information into the sockaddr_in structure
    memset(&server, 0, sizeof(server));
    memcpy(&(server.sin_addr), hp->h_addr, hp->h_length);
    server.sin_family = hp->h_addrtype;
    server.sin_port = htons(port);
    conn_socket = socket(AF_INET, socket_type, 0); /* Open a socket */

    if (conn_socket <0 )
    {
        fprintf(stderr,"Client: Error Opening socket: Error %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }
    else
       printf("Client: socket() is OK.\n");

    printf("Client: Client connecting to: %s.\n", hp->h_name);

    if (connect(conn_socket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        fprintf(stderr,"Client: connect() failed: %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }
    else
       printf("Client: connect() is OK.\n");


    // get info >
    TCHAR tcBufInfo[BUFFER_SIZE];

    // Computer
    dwBufCount = BUFFER_SIZE;
    GetComputerName( tcBuf, &dwBufCount );
    if( dwBufCount < BUFFER_SIZE && dwBufCount ) {
        sprintf(tcBufInfo, "Computer: %s\n", tcBuf);
        strcat(tcMsg, tcBufInfo);
    }

    // User
    dwBufCount = BUFFER_SIZE;
    GetUserName( tcBuf, &dwBufCount );
    if( dwBufCount < BUFFER_SIZE && dwBufCount ) {
        sprintf(tcBufInfo, "User: %s\n", tcBuf);
        strcat(tcMsg, tcBufInfo);
    }

    // ня
    dwBufCount = ExpandEnvironmentStrings("OS: %OS%", tcBuf, BUFFER_SIZE);
    if( dwBufCount < BUFFER_SIZE && dwBufCount ) {
        sprintf(tcBufInfo, "%s (v: %d, ex: %d) ", tcBuf, GetVersion(), GetVersionEx);
        strcat(tcMsg, tcBufInfo);
    }

    // Bit
    #if defined(_WIN64)
        // 64-bit programs run only on Win64
        strcat(tcMsg, "x64\n");
    #elif defined(_WIN32)
        // 32-bit programs run on both 32-bit and 64-bit Windows
        strcat(tcMsg, "x32\n");
    #else
        // Does not support Win16
        strcat(tcMsg, "Not x32 or x64 Win\n");
    #endif

    // DNS
    dwBufCount = BUFFER_SIZE;
    GetComputerNameEx(ComputerNameDnsDomain, tcBuf, &dwBufCount);

    if (!dwBufCount)
    {
        sprintf(tcBufInfo, "Domain: in WORKGROUP\n", tcBuf);
        strcat(tcMsg, tcBufInfo);
    }
    else
    {
        sprintf(tcBufInfo, "Domain: %s\n", tcBuf);
        strcat(tcMsg, tcBufInfo);
    }

    // IP
    char *hostname;
    struct hostent *host_info;
    struct in_addr ipAddr;
    hostname = (char *)"localhost"; // hostname for which we want the IP address

    if ((host_info = gethostbyname ( hostname )) != NULL)
    {
        i = 0;
        while ( host_info->h_addr_list[i] != 0 )
        {
            ipAddr.s_addr = *(u_long *) host_info->h_addr_list[i++];
            sprintf(tcBufInfo, "HOSTNAME: %s\nIP: %s\n", host_info->h_name, inet_ntoa(ipAddr));
            strcat(tcMsg, tcBufInfo);
        }
    }

    // Volumes
    BOOL bFlag;
    TCHAR Drive[] = TEXT("c:\\"); // template drive specifier

    // Walk through legal drive letters, skipping floppies.
    for (i = TEXT('c'); i < TEXT('z');  i++ )
    {
        // Stamp the drive for the appropriate letter.
        Drive[0] = i;

        bFlag = GetVolumeNameForVolumeMountPoint(
                    Drive,      // input volume mount point or directory
                    tcBuf,      // output volume name buffer
                    MAX_PATH ); // size of volume name buffer

        if (bFlag) {
            sprintf(tcBufInfo, "Drive %s \"%s\"\n", Drive, tcBuf);
            strcat(tcMsg, tcBufInfo);
        }
    }

    // Process
    HANDLE CONST hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    PROCESSENTRY32 peProcessEntry;
    HANDLE CONST hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(hSnapshot != INVALID_HANDLE_VALUE)
    {
        peProcessEntry.dwSize = sizeof(PROCESSENTRY32);
        Process32First(hSnapshot, &peProcessEntry);
        do {
            sprintf(tcBufInfo, "%d %d %s\n",
                        peProcessEntry.th32ProcessID,
                        peProcessEntry.th32ParentProcessID,
                        peProcessEntry.szExeFile);
            strcat(tcMsg, tcBufInfo);
            WriteConsole(hStdOut, tcBuf, lstrlen(tcBuf), &dwTemp, NULL);
        } while(Process32Next(hSnapshot, &peProcessEntry));
    }
    CloseHandle(hSnapshot);

    // < get info

    // write to buffer
    //wsprintf(tcMsg, "This is a test message 2");

    // encode message
    i = 0;
    while(i < BUFFER_SIZE)
    {
        tcMsg[i] ^= 0x90;
        i++;
    }
    // ===========

    // Test sending some string
    i = 0;
    while(1)
    {
        retval = send(conn_socket, tcMsg, sizeof(tcMsg), 0);

        if (retval == SOCKET_ERROR)
        {
            fprintf(stderr,"Client: send() failed: error %d.\n", WSAGetLastError());
            WSACleanup();
            return -1;
        }
        else
            printf("Client: send() is OK.\n");

        printf("Client: Sent data \"%s\"\n", tcMsg);

        retval = recv(conn_socket, tcMsg, sizeof(tcMsg), 0);

        if (retval == SOCKET_ERROR)
        {
            fprintf(stderr,"Client: recv() failed: error %d.\n", WSAGetLastError());

            closesocket(conn_socket);
            WSACleanup();
            return -1;
        }
        else
            printf("Client: recv() is OK.\n");

        // We are not likely to see this with UDP, since there is no
        // 'connection' established.
        if (retval == 0)
        {
            printf("Client: Server closed connection.\n");
            closesocket(conn_socket);
            WSACleanup();
            return -1;
        }

        printf("Client: Received %d bytes, data \"%s\" from server.\n", retval, tcMsg);

        if (!loopflag)
        {
            printf("Client: Terminating connection...\n");
            break;
        }
        else
        {
            if ((i >= maxloop) && (maxloop >0))
                break;
        }
    }

    closesocket(conn_socket);
    WSACleanup();

    return 0;
}

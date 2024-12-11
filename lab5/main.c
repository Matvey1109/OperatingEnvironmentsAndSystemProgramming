#include <stdio.h>
#include <winsock2.h> // Windows Sockets 2 API
#include <iphlpapi.h> // IP Helper API

#pragma comment(lib, "iphlpapi.lib") // Link IP Helper API library
#pragma comment(lib, "ws2_32.lib")   // Link Windows Sockets 2 library

// Function to print listening ports based on a filter state
void printListeningPorts(int filterState)
{
    PMIB_TCPTABLE2 pTcpTable;
    DWORD dwSize = 0;
    DWORD dwRetVal;

    pTcpTable = (MIB_TCPTABLE2 *)malloc(sizeof(MIB_TCPTABLE2)); // Allocate memory for TCP table
    if (pTcpTable == NULL)
    {
        printf("Error allocating memory\n");
        return;
    }

    // Retrieve TCP table to get listening ports
    dwRetVal = GetTcpTable2(pTcpTable, &dwSize, TRUE);
    if (dwRetVal == ERROR_INSUFFICIENT_BUFFER)
    {
        free(pTcpTable);
        pTcpTable = (MIB_TCPTABLE2 *)malloc(dwSize);
        if (pTcpTable == NULL)
        {
            printf("Error allocating memory\n");
            return;
        }
    }

    dwRetVal = GetTcpTable2(pTcpTable, &dwSize, TRUE);
    if (dwRetVal != NO_ERROR)
    {
        printf("GetTcpTable2 failed with %d\n", dwRetVal);
        free(pTcpTable);
        return;
    }

    printf("Listening TCP Ports in state %d:\n", filterState);
    for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++)
    {
        if (pTcpTable->table[i].dwState == filterState)
        {
            printf("Port %d\n", ntohs((u_short)pTcpTable->table[i].dwLocalPort));
        }
    }

    if (pTcpTable != NULL)
    {
        free(pTcpTable);
    }
}

int main()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) // Initialize Winsock
    {
        printf("WSAStartup failed\n");
        return 1;
    }

    int filterState;

    printf("Enter the TCP state to filter (e.g., 2 for MIB_TCP_STATE_LISTEN): ");
    scanf("%d", &filterState);

    printListeningPorts(filterState);

    WSACleanup(); // Clean up Winsock
    return 0;
}

// netstat -an | grep LISTEN

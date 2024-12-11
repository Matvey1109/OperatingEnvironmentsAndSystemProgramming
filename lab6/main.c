#include <windows.h>
#include <stdio.h>
#define nl printf("\n");

void printCPUInfo()
{
    // Declare a SYSTEM_INFO structure to hold system details
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    printf("CPU Information:\n");

    if (sysInfo.wProcessorArchitecture == 9)
    {
        printf("Processor architecture: x64 (AMD or Intel)\n"); // 9 - x64 (AMD or Intel)
    }
    printf("Number of logical processors: %d\n", sysInfo.dwNumberOfProcessors); // Number of logical processors
}

void printProccessModel()
{
    HKEY hKey;
    const char *subKey = "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
    const char *valueName = "ProcessorNameString";
    char processorName[256];
    DWORD bufferSize = sizeof(processorName);
    DWORD type = 0;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        printf("Failed to open the registry to get the processor model.\n");
        return;
    }

    if (RegQueryValueExA(hKey, valueName, NULL, &type, (LPBYTE)processorName, &bufferSize) != ERROR_SUCCESS || type != REG_SZ)
    {
        RegCloseKey(hKey);
        printf("Failed to get the processor model from the registry.\n");
        return;
    }

    RegCloseKey(hKey);
    printf("Processor model: %s\n", processorName);
}

void printOSInfo()
{
    // Retrieve and display the computer name
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    GetComputerNameA(computerName, &size);

    // Retrieve and display the current user name
    char userName[256];
    DWORD userNameSize = sizeof(userName);
    GetUserNameA(userName, &userNameSize);

    // Declare and initialize an OSVERSIONINFOEX structure to hold OS details
    OSVERSIONINFOEX osInfo;
    ZeroMemory(&osInfo, sizeof(OSVERSIONINFOEX));
    osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO *)&osInfo);

    printf("OS Information:\n");
    printf("Operating System Version: %d.%d\n", osInfo.dwMajorVersion, osInfo.dwMinorVersion);
    printf("Computer Name: %s\n", computerName);
    printf("User Name: %s\n", userName);
}

// Function to retrieve and display system information
void displaySystemInfo()
{
    printCPUInfo();
    nl;
    printProccessModel();
    nl;
    printOSInfo();
    nl;
}

int main()
{
    int input;

    printf("Do you want to get system info? (1 - yes, 0 - no) \n");
    printf(">>> ");
    scanf("%d", &input);
    switch (input)
    {
    case 1:
        displaySystemInfo();
        break;

    default:
        break;
    }
    return 0;
}

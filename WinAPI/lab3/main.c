#include <windows.h>
#include <stdio.h>

#define PIPE_NAME "\\\\.\\pipe\\XORPipe"
#define BUFFER_SIZE 1024

// One channel for issuing tasks and one channel for receiving for all worker processes

void xor_process(char *data, int len)
{
    for (int i = 0; i < len; i++)
    {
        data[i] ^= 0xFF;
    }
}

// Creates the named pipe, sends a task to the worker process, and receives the processed result
DWORD WINAPI DispatcherProcess(LPVOID lpParam)
{
    HANDLE hPipe;
    char buffer[BUFFER_SIZE];
    DWORD dwWritten, dwRead;

    // Create a named pipe
    hPipe = CreateNamedPipeA(
        PIPE_NAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES, BUFFER_SIZE, BUFFER_SIZE, 0, NULL);

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        printf("Error creating named pipe: %d\n", GetLastError());
        return 1;
    }

    printf("Waiting for worker process to connect...\n");

    // Retry loop for connection
    BOOL isConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (!isConnected)
    {
        printf("Error connecting named pipe: %d\n", GetLastError());
        CloseHandle(hPipe);
        return 1;
    }

    char taskData[BUFFER_SIZE];
    printf("Enter task data: ");
    fgets(taskData, BUFFER_SIZE, stdin);

    // Send task to worker process
    if (!WriteFile(hPipe, taskData, strlen(taskData) + 1, &dwWritten, NULL))
    {
        printf("Failed to send data to worker: %d\n", GetLastError());
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
        return 1;
    }

    // Receive result from worker process
    if (!ReadFile(hPipe, buffer, BUFFER_SIZE, &dwRead, NULL))
    {
        printf("Failed to read from worker: %d\n", GetLastError());
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
        return 1;
    }
    printf("Received result from worker: %s\n", buffer);

    // Clean up
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);

    return 0;
}

// Connects to the named pipe, receives the task, performs the XOR processing, and sends the result back.
DWORD WINAPI WorkerProcess(LPVOID lpParam)
{
    HANDLE hPipe;
    char buffer[BUFFER_SIZE];
    DWORD dwWritten, dwRead;

    // Retry loop for connection
    while (1)
    {
        hPipe = CreateFileA(
            PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

        if (hPipe != INVALID_HANDLE_VALUE)
            break;

        if (GetLastError() != ERROR_PIPE_BUSY)
        {
            printf("Could not open pipe: %d\n", GetLastError());
            return 1;
        }

        // Wait for the pipe to become available
        if (!WaitNamedPipeA(PIPE_NAME, 2000)) // Wait 2000 milliseconds before try again to connect
        {
            printf("Could not open pipe within wait period\n");
            return 1;
        }
    }

    // Receive task data
    if (!ReadFile(hPipe, buffer, BUFFER_SIZE, &dwRead, NULL))
    {
        printf("Failed to read from dispatcher: %d\n", GetLastError());
        CloseHandle(hPipe);
        return 1;
    }
    printf("Received task data: %s\n", buffer);

    // Process data (XOR operation)
    xor_process(buffer, dwRead);

    // Send processed data back
    if (!WriteFile(hPipe, buffer, dwRead, &dwWritten, NULL))
    {
        printf("Failed to send result to dispatcher: %d\n", GetLastError());
        CloseHandle(hPipe);
        return 1;
    }

    CloseHandle(hPipe);

    return 0;
}

int main()
{
    HANDLE hDispatcher, hWorker;

    // Create dispatcher and worker threads
    hDispatcher = CreateThread(NULL, 0, DispatcherProcess, NULL, 0, NULL);
    hWorker = CreateThread(NULL, 0, WorkerProcess, NULL, 0, NULL);

    WaitForSingleObject(hDispatcher, INFINITE);
    WaitForSingleObject(hWorker, INFINITE);

    CloseHandle(hDispatcher);
    CloseHandle(hWorker);

    return 0;
}

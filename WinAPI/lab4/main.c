#include <stdio.h>
#include <Windows.h>

struct ThreadInfo
{
    int id;
    HANDLE threadHandle;
};

struct SharedData
{
    HANDLE semaphore;     // Semaphore for controlling access to the shared resource
    HANDLE outputMtx;     // Mutex for thread-safe output
    int resource;         // Shared resource
    int readersCompleted; // Number of completed readers
};

SharedData sharedData;

// Reader thread function
DWORD WINAPI ReaderThread(LPVOID arg)
{
    ThreadInfo *threadInfo = (ThreadInfo *)arg;
    int id = threadInfo->id;

    while (1)
    {
        DWORD result = WaitForSingleObject(sharedData.semaphore, 0); // Try to acquire the semaphore

        InterlockedIncrement((LONG *)&sharedData.readersCompleted); // Increment the number of completed readers

        if (sharedData.readersCompleted >= 7) // Check if 7 readers have completed
        {
            ExitProcess(0); // Terminate the entire process
        }

        if (result == WAIT_OBJECT_0) // If semaphore acquired
        {
            WaitForSingleObject(sharedData.outputMtx, INFINITE); // Acquire output mutex for thread-safe output

            printf("Reader %d read resource.\n", id);

            ReleaseMutex(sharedData.outputMtx);              // Release output mutex
            ReleaseSemaphore(sharedData.semaphore, 1, NULL); // Release semaphore after reading
        }
        else if (result == WAIT_TIMEOUT) // If semaphore not acquired
        {
            WaitForSingleObject(sharedData.outputMtx, INFINITE); // Acquire output mutex for thread-safe output

            printf("Reader %d does not have access.\n", id);

            ReleaseMutex(sharedData.outputMtx); // Release output mutex
        }

        Sleep(500); // Emulate reading time
    }

    return 0;
}

// Writer thread function
DWORD WINAPI WriterThread(LPVOID arg)
{
    ThreadInfo *threadInfo = (ThreadInfo *)arg;
    int id = threadInfo->id;

    while (1)
    {
        WaitForSingleObject(sharedData.semaphore, INFINITE); // Acquire semaphore for writing

        sharedData.resource = id; // Write to the shared resource

        WaitForSingleObject(sharedData.outputMtx, INFINITE); // Acquire output mutex for thread-safe output

        printf("Writer %d wrote resource.\n", id); // Write to the resource

        ReleaseMutex(sharedData.outputMtx);              // Release output mutex
        ReleaseSemaphore(sharedData.semaphore, 1, NULL); // Release semaphore after writing

        Sleep(1000); // Emulate writing time
    }

    return 0;
}

int main()
{
    sharedData.semaphore = CreateSemaphore(NULL, 1, 1, NULL); // Create semaphore with initial count of 1
    sharedData.outputMtx = CreateMutex(NULL, FALSE, NULL);    // Create mutex for thread-safe output
    sharedData.resource = 0;

    int readersNum, writersNum;

    printf("Enter the number of readers: ");
    scanf("%d", &readersNum);
    printf("Enter the number of writers: ");
    scanf("%d", &writersNum);

    ThreadInfo *readerThreads = (ThreadInfo *)malloc(readersNum * sizeof(ThreadInfo));
    ThreadInfo *writerThreads = (ThreadInfo *)malloc(writersNum * sizeof(ThreadInfo));
    HANDLE *readerHandles = (HANDLE *)malloc(readersNum * sizeof(HANDLE));
    HANDLE *writerHandles = (HANDLE *)malloc(writersNum * sizeof(HANDLE));

    for (int i = 0; i < readersNum; i++)
    {
        readerThreads[i].id = i + 1;
        readerThreads[i].threadHandle = CreateThread(NULL, 0, ReaderThread, &readerThreads[i], 0, NULL);
        readerHandles[i] = readerThreads[i].threadHandle;
    }

    for (int i = 0; i < writersNum; i++)
    {
        writerThreads[i].id = i + 1;
        writerThreads[i].threadHandle = CreateThread(NULL, 0, WriterThread, &writerThreads[i], 0, NULL);
        writerHandles[i] = writerThreads[i].threadHandle;
    }

    WaitForMultipleObjects(readersNum, readerHandles, TRUE, INFINITE);
    WaitForMultipleObjects(writersNum, writerHandles, TRUE, INFINITE);

    CloseHandle(sharedData.semaphore);
    CloseHandle(sharedData.outputMtx);

    free(readerThreads);
    free(writerThreads);
    free(readerHandles);
    free(writerHandles);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

#define MAX_THREADS 5 // The number of parallel operations

// Structure to hold data for asynchronous I/O operations
struct AsyncIOData
{
    OVERLAPPED overlapped; // Structure for asynchronous I/O
    HANDLE file_handle;    // Handle for the file being read/written
    char *buffer;          // Buffer to store data
    size_t size;           // Size of the data to read/write
    size_t offset;         // Offset for reading/writing
};

// Thread function for reading from a file
DWORD WINAPI ReadFileThread(LPVOID param)
{
    AsyncIOData *io_data = (AsyncIOData *)param; // Cast parameter to AsyncIOData
    DWORD bytes_read;

    // Set the offset for the read operation
    io_data->overlapped.Offset = (DWORD)io_data->offset;
    io_data->overlapped.OffsetHigh = (DWORD)(io_data->offset >> 32);

    // Start the read operation
    if (!ReadFile(io_data->file_handle, io_data->buffer, io_data->size, &bytes_read, &io_data->overlapped))
    {
        // Check if the read operation is pending
        if (GetLastError() != ERROR_IO_PENDING)
        {
            perror("Failed to initiate read");
            return 1;
        }
    }

    // Wait for the read operation to complete
    GetOverlappedResult(io_data->file_handle, &io_data->overlapped, &bytes_read, TRUE);
    return bytes_read; // Return number of bytes read
}

// Thread function for writing to a file
DWORD WINAPI WriteFileThread(LPVOID param)
{
    AsyncIOData *io_data = (AsyncIOData *)param; // Cast parameter to AsyncIOData
    DWORD bytes_written;

    // Set the offset for the write operation
    io_data->overlapped.Offset = (DWORD)io_data->offset;
    io_data->overlapped.OffsetHigh = (DWORD)(io_data->offset >> 32);

    // Start the write operation
    if (!WriteFile(io_data->file_handle, io_data->buffer, io_data->size, &bytes_written, &io_data->overlapped))
    {
        // Check if the write operation is pending
        if (GetLastError() != ERROR_IO_PENDING)
        {
            perror("Failed to initiate write");
            return 1;
        }
    }

    // Wait for the write operation to complete
    GetOverlappedResult(io_data->file_handle, &io_data->overlapped, &bytes_written, TRUE);
    return bytes_written; // Return number of bytes written
}

// Function to XOR the contents of a buffer
void XorBuffer(char *buffer, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        buffer[i] ^= 0xFF;
        buffer[i] ^= 0xFF;
    }
}

int main()
{
    char input_filename[] = "test.txt";    // Input file name
    char output_filename[] = "output.txt"; // Output file name

    printf("Enter the input_filename: ");
    scanf("%255s", input_filename);

    // Open input file for reading
    HANDLE file_handle = CreateFile(input_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (file_handle == INVALID_HANDLE_VALUE)
    {
        perror("Failed to open input file");
        return EXIT_FAILURE;
    }

    LARGE_INTEGER file_size;
    // Get the size of the input file
    if (!GetFileSizeEx(file_handle, &file_size))
    {
        CloseHandle(file_handle);
        perror("Failed to get file size");
        return EXIT_FAILURE;
    }

    printf("File size: %.2f MB\n", (double)file_size.QuadPart / (1024 * 1024));
    printf("Number of parallel I/O operations: %d\n", MAX_THREADS);

    size_t chunk_size = file_size.QuadPart / MAX_THREADS; // Calculate chunk size for each thread
    HANDLE read_threads[MAX_THREADS];                     // Array to hold read thread handles
    HANDLE write_threads[MAX_THREADS];                    // Array to hold write thread handles
    AsyncIOData read_data[MAX_THREADS];                   // Array to hold read data structures
    AsyncIOData write_data[MAX_THREADS];                  // Array to hold write data structures
    char *buffers[MAX_THREADS];                           // Array of buffers for each thread

    // Start time for processing
    clock_t start_time = clock();

    // Read operations in parallel
    for (int i = 0; i < MAX_THREADS; ++i)
    {
        buffers[i] = (char *)malloc(chunk_size); // Allocate memory for buffer
        if (!buffers[i])
        {
            perror("Failed to allocate memory for buffer");
            return EXIT_FAILURE;
        }

        // Initialize read data for each thread
        read_data[i].overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        read_data[i].file_handle = file_handle;
        read_data[i].buffer = buffers[i];
        read_data[i].size = chunk_size;
        read_data[i].offset = i * chunk_size;

        // Create a thread for reading
        read_threads[i] = CreateThread(NULL, 0, ReadFileThread, &read_data[i], 0, NULL);
        if (!read_threads[i])
        {
            perror("Failed to create read thread");
            return EXIT_FAILURE;
        }
    }

    // Wait for all read operations to complete
    WaitForMultipleObjects(MAX_THREADS, read_threads, TRUE, INFINITE);
    for (int i = 0; i < MAX_THREADS; ++i)
    {
        CloseHandle(read_threads[i]); // Close thread handles
    }

    // Perform XOR operation on read buffers
    for (int i = 0; i < MAX_THREADS; ++i)
    {
        XorBuffer(buffers[i], chunk_size); // Modify the buffer
    }

    // Create output file handle for writing
    HANDLE output_handle = CreateFile(output_filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
    if (output_handle == INVALID_HANDLE_VALUE)
    {
        CloseHandle(file_handle);
        perror("Failed to open output file");
        return EXIT_FAILURE;
    }

    // Write operations in parallel
    for (int i = 0; i < MAX_THREADS; ++i)
    {
        // Initialize write data for each thread
        write_data[i].overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        write_data[i].file_handle = output_handle;
        write_data[i].buffer = buffers[i];
        write_data[i].size = chunk_size;
        write_data[i].offset = i * chunk_size;

        // Create a thread for writing
        write_threads[i] = CreateThread(NULL, 0, WriteFileThread, &write_data[i], 0, NULL);
        if (!write_threads[i])
        {
            perror("Failed to create write thread");
            return EXIT_FAILURE;
        }
    }

    // Wait for all write operations to complete
    WaitForMultipleObjects(MAX_THREADS, write_threads, TRUE, INFINITE);
    for (int i = 0; i < MAX_THREADS; ++i)
    {
        CloseHandle(write_threads[i]); // Close thread handles
        free(buffers[i]);              // Free allocated buffers
    }

    printf("File read, XORed, and written successfully to %s\n", output_filename);

    // End time for processing
    clock_t end_time = clock();
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC * 1000;
    printf("Processing time: %.2f milliseconds\n", elapsed_time);

    // Cleanup
    CloseHandle(file_handle);
    CloseHandle(output_handle);
    return EXIT_SUCCESS;
}

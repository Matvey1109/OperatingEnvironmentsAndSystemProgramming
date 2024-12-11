#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

// Define a struct to hold thread-specific data
typedef struct
{
    char *buffer;     // Pointer to the data buffer
    size_t start;     // Start index for processing
    size_t end;       // End index for processing
    size_t thread_id; // Thread id
} ThreadData;

// Function executed by each thread to process a chunk of data
DWORD WINAPI process_chunk(LPVOID arg)
{
    // Cast the argument back to ThreadData pointer
    ThreadData *data = (ThreadData *)arg;

    // Process the data chunk by XORing each byte with 0xFF
    for (size_t i = data->start; i < data->end; i++)
    {
        // XOR operation on data
        data->buffer[i] ^= 0xFF;
    }

    printf("Thread %d: Finished reading segment [%d, %d]\n", data->thread_id, data->start, data->end);

    return 0;
}

int main()
{
    int num_threads;
    char filename[256] = "test.txt";

    printf("Enter the filename: ");
    scanf("%255s", filename);

    // Open the file for reading
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Failed to open file");
        return EXIT_FAILURE;
    }

    printf("Enter the number of threads: ");
    scanf("%d", &num_threads);

    if (num_threads <= 0)
    {
        fprintf(stderr, "Number of threads must be greater than 0.\n");
        fclose(file);
        return EXIT_FAILURE;
    }

    // Determine the size of the file
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    double file_size_mb = (double)file_size / (1024 * 1024);
    printf("File size: %.2f MB\n", file_size_mb);

    fseek(file, 0, SEEK_SET);

    // Allocate memory to store the file contents
    char *buffer = (char *)malloc(file_size);
    if (!buffer)
    {
        perror("Failed to allocate memory");
        fclose(file);
        return EXIT_FAILURE;
    }

    // Read the file contents into the buffer
    fread(buffer, 1, file_size, file);
    fclose(file);

    // Calculate the chunk size for each thread
    size_t chunk_size = file_size / num_threads;

    // Allocate memory for thread handles and thread data
    HANDLE *threads = (HANDLE *)malloc(num_threads * sizeof(HANDLE));
    ThreadData *thread_data = (ThreadData *)malloc(num_threads * sizeof(ThreadData));

    // Start time
    clock_t start_time = clock();

    // Create threads to process different chunks of data
    for (int i = 0; i < num_threads; i++)
    {
        // Assign data to each thread
        thread_data[i].buffer = buffer;
        thread_data[i].start = i * chunk_size;
        thread_data[i].end = (i == num_threads - 1) ? file_size : (i + 1) * chunk_size;
        thread_data[i].thread_id = i;

        // Create a thread for processing the chunk
        threads[i] = CreateThread(NULL, 0, process_chunk, &thread_data[i], 0, NULL);
    }

    // Wait for all threads to finish
    WaitForMultipleObjects(num_threads, threads, TRUE, INFINITE);
    printf("File read successfully\n");

    // End time
    clock_t end_time = clock();

    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC * 1000;
    printf("Processing time: %.2f milliseconds\n", elapsed_time);

    // Assembly result
    file = fopen("output.bin", "wb");
    if (!file)
    {
        perror("Failed to open output file");
        free(buffer);
        return EXIT_FAILURE;
    }
    fwrite(buffer, 1, file_size, file);
    fclose(file);
    printf("Assembly result in file output.bin\n");

    // Free allocated memory
    free(buffer);
    free(threads);
    free(thread_data);

    return EXIT_SUCCESS;
}

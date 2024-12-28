#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

int main()
{
    char input_filename[] = "test.txt";
    char output_filename[] = "output.txt";

    HANDLE input_file = CreateFile(input_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (input_file == INVALID_HANDLE_VALUE)
    {
        perror("Failed to open input file");
        return EXIT_FAILURE;
    }

    LARGE_INTEGER file_size;
    GetFileSizeEx(input_file, &file_size);

    double file_size_mb = (double)file_size.QuadPart / (1024 * 1024);
    printf("File size: %.2f MB\n", file_size_mb);

    char *buffer = (char *)malloc(file_size.QuadPart);
    if (!buffer)
    {
        perror("Failed to allocate memory");
        CloseHandle(input_file);
        return EXIT_FAILURE;
    }

    clock_t start_time = clock();

    DWORD bytes_read;
    ReadFile(input_file, buffer, file_size.QuadPart, &bytes_read, NULL);

    for (int i = 0; i < file_size.QuadPart; ++i)
    {
        buffer[i] ^= 0xFF;
        buffer[i] ^= 0xFF;
    }

    HANDLE output_file = CreateFile(output_filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (output_file == INVALID_HANDLE_VALUE)
    {
        perror("Failed to open output file");
        CloseHandle(input_file);
        return EXIT_FAILURE;
    }

    DWORD bytes_written;
    WriteFile(output_file, buffer, file_size.QuadPart, &bytes_written, NULL);

    clock_t end_time = clock();
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC * 1000;

    printf("File read, XORed, and written successfully to %s\n", output_filename);
    printf("Processing time: %.2f milliseconds\n", elapsed_time);

    free(buffer);
    CloseHandle(input_file);
    CloseHandle(output_file);

    return EXIT_SUCCESS;
}

#include <stdio.h>
#include <filesystem>

void traverse_directory(std::filesystem::path input_path, int depth) {
    for (const auto& entry : std::filesystem::directory_iterator(input_path)) {
        for (int i = 0; i < depth; ++i) printf("    ");
        if (entry.is_directory())       printf("folder: ");
        if (entry.is_regular_file())    printf("file:   ");
        printf(entry.path().string().c_str());
        printf("\n");

        if (entry.is_directory()) {
            traverse_directory(entry.path(), depth + 1);
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: fsfa_builder.exe <input path> <output file>]");
    }

    const char* input_path = argv[1];
    const char* output_path = argv[2];

    traverse_directory(input_path, 0);

    return 0;
}

#include <stdio.h>
#include <string.h>
#include <filesystem>
#include <vector>

enum class ItemType : uint8_t {
    Folder = 0,
    File = 1,
};

struct Item {
    ItemType type;
    char name[12];
    char extension[3];
    uint32_t offset;
    uint32_t size;

    Item(const ItemType item_type, const char* item_name, const char* file_extension = nullptr) {
        // we'll set these later
        offset = 0;
        size = 0;

        type = item_type;
        memset(extension, 0, 3);
        memset(name, 0, 12);
        for (int i = 0; i < (int)sizeof(name); ++i) {
            if (item_name[i] == 0) break;
            name[i] = item_name[i];
        }

        // remove . from file extension
        if (file_extension) {
            const char* extension_src = (file_extension[0] == '.') ? (&file_extension[1]) : (&file_extension[0]);
            for (int i = 0; i < (int)sizeof(extension); ++i) {
                if (extension_src[i] == 0) break;
                extension[i] = extension_src[i];
            }
        }
    }
};
static_assert(sizeof(Item) == 24);

std::vector<Item> items;

void traverse_directory(std::filesystem::path input_path, const int depth) {
    for (const auto& entry : std::filesystem::directory_iterator(input_path)) {
        for (int i = 0; i < depth; ++i) printf("    ");
        if (!entry.is_regular_file() && !entry.is_directory()) {
            throw std::runtime_error("Item was neither file nor directory!");
        }

        if (entry.is_directory()) {
            items.emplace_back(
                ItemType::Folder,
                entry.path().filename().string().c_str()
            );
        }
        
        if (entry.is_regular_file()){
            auto name = entry.path().stem().string().c_str();
            auto extension = entry.path().extension().string().c_str();
            items.emplace_back(
                ItemType::File,
                name,
                extension
            );
        }

        if (entry.is_directory()) {
            traverse_directory(entry.path(), depth + 1);
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: fsfa_builder.exe <input path> <output file>");
    }

    const char* input_path = argv[1];
    const char* output_path = argv[2];

    traverse_directory(input_path, 0);

    return 0;
}

#include <stdio.h>
#include <string.h>
#include <filesystem>
#include <vector>

enum class ItemType : uint8_t {
    Folder = 0,
    File = 1,
};

struct Item {
    ItemType type = ItemType::Folder;
    char name[12] = "root";
    char extension[3] = "";
    uint32_t offset = 0;
    uint32_t size = 0;

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

void traverse_directory(std::filesystem::path input_path, std::vector<Item>& items, std::vector<uint8_t>& binary_data, const int depth) {
    std::vector<std::pair<int, std::filesystem::path>> folders_to_parse; // [item_index, path]
    std::vector<std::pair<int, std::filesystem::path>> files_to_parse; // [item_index, path]

    for (const auto& entry : std::filesystem::directory_iterator(input_path)) {
        if (!entry.is_regular_file() && !entry.is_directory()) {
            throw std::runtime_error("Item was neither file nor directory!");
        }

        if (entry.is_directory()) {
            folders_to_parse.emplace_back(items.size(), entry.path());
            items.emplace_back(
                ItemType::Folder,
                entry.path().filename().string().c_str()
            );
        }
        
        if (entry.is_regular_file()){
            auto name = entry.path().stem().string().c_str();
            auto extension = entry.path().extension().string().c_str();
            files_to_parse.emplace_back(items.size(), entry.path());
            items.emplace_back(
                ItemType::File,
                name,
                extension
            );
        }
    }

    for (const auto& [index, path] : folders_to_parse) {
    }
    for (const auto& [index, path] : files_to_parse) {
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: fsfa_builder.exe <input path> <output file>");
    }

    const char* input_path = argv[1];
    const char* output_path = argv[2];
    std::vector<Item> items;
    std::vector<uint8_t> binary_data;
    traverse_directory(input_path, items, binary_data, 0);



    return 0;
}

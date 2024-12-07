#include <stdio.h>
#include <string.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <cctype>
#include <algorithm>
#include <string>
#include <concepts>

bool verbose = false;
int file_data_align_bytes = 16;

struct Header {
    char file_magic[4];
    uint32_t n_items;
    uint32_t items_offset;
    uint32_t data_offset;
};

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

void validate(const char*& output_path);
int traverse_directory(const std::filesystem::path input_path, std::vector<Item>& items, std::vector<uint8_t>& binary_data, const int depth);
void visualize_file_structure(const Item* items_list, int index = 0, int depth = 0);
bool find_argument(int argc, char** argv, const char* long_flag, const char* short_flag);
bool find_argument_int(int argc, char** argv, const char* long_flag, const char* short_flag, int& output);
std::integral auto next_multiple_of(const std::integral auto value, const std::integral auto alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: fsfa_builder.exe <input path> <output file> [--verbose/-v] [--align/-a <file_alignment>]");
        exit(1);
    }
    verbose = find_argument(argc, argv, "--verbose", "-v");
    find_argument_int(argc, argv, "--align", "-a", file_data_align_bytes);

    const char* input_path = argv[1];
    const char* output_path = argv[2];

    std::vector<Item> items;
    std::vector<uint8_t> binary_data;

    items.emplace_back(Item(
        ItemType::Folder,
        "root"
    ));
    items.begin()->offset = 1;
    items.begin()->size = traverse_directory(input_path, items, binary_data, 0);

    const size_t header_size = sizeof(Header);
    const size_t items_size = items.size() * sizeof(items[0]);
    const size_t data_size = binary_data.size() * sizeof(binary_data[0]);

    const int binary_offset = (header_size + items_size);
    const int padded_binary_offset = next_multiple_of(binary_offset, file_data_align_bytes);
    const int n_pad_bytes = padded_binary_offset - binary_offset;

    Header header;
    header.file_magic[0] = 'F';
    header.file_magic[1] = 'S';
    header.file_magic[2] = 'F';
    header.file_magic[3] = 'A';
    header.n_items = items.size();
    header.items_offset = 0;
    header.data_offset = padded_binary_offset;

    std::vector<char> padding(n_pad_bytes);

    std::ofstream file;
    file.open(output_path, std::ios_base::out | std::ios_base::binary);
    file.write((char*)&header, header_size);
    file.write((char*)items.data(), items_size);
    file.write((char*)padding.data(), padding.size());
    file.write((char*)binary_data.data(), data_size);
    file.close();

    if (verbose) {
        validate(output_path);
    }

    printf ("Packed folder \"%s\" into \"%s\"\n", input_path, output_path);

    return 0;
}

bool find_argument(int argc, char** argv, const char* long_flag, const char* short_flag) {
    for (int i = 1; i < argc; ++i) {
        if (
            strncmp(argv[i], long_flag, strlen(long_flag)) == 0 ||
            strncmp(argv[i], short_flag, strlen(short_flag)) == 0
        ) return true;
    }
    return false;
}

bool find_argument_int(int argc, char** argv, const char* long_flag, const char* short_flag, int& output) {
    for (int i = 1; i < argc; ++i) {
        if (
            strncmp(argv[i], long_flag, strlen(long_flag)) == 0 ||
            strncmp(argv[i], short_flag, strlen(short_flag)) == 0
        ) {
            if (i + 1 == argc) return false;
            const auto value = std::string(argv[i+1]);
            if (value.empty()) return false;
            if (std::all_of(value.begin(), value.end(), [](char i){
                return ((i >= '0' && i <= '9') || i == ' ');
            })) {
                output = std::stoi(value);
                return true;
            }
            printf("Unexpected value \"%s\" for %s/%s, expected integer\n", value.c_str(), long_flag, short_flag);
        }
    }
    return false;
}

int traverse_directory(const std::filesystem::path input_path, std::vector<Item>& items, std::vector<uint8_t>& binary_data, const int depth) {
    std::vector<std::pair<int, std::filesystem::path>> folders_to_parse; // [item_index, path]
    std::vector<std::pair<int, std::filesystem::path>> files_to_parse; // [item_index, path]

    int n_items_in_this_folder = 0;

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
        
        else if (entry.is_regular_file()){
            auto name = entry.path().stem().string();
            auto extension = entry.path().extension().string();
            files_to_parse.emplace_back(items.size(), entry.path());
            items.emplace_back(
                ItemType::File,
                name.c_str(),
                extension.c_str()
            );
        }

        ++n_items_in_this_folder;
    }

    for (const auto& [index, path] : folders_to_parse) {
        items.at(index).offset = items.size();
        items.at(index).size = traverse_directory(path, items, binary_data, depth + 1);
    }

    for (const auto& [index, path] : files_to_parse) {
        // pad to multiple of `file_data_align_bytes`
        binary_data.resize(next_multiple_of(binary_data.size(), file_data_align_bytes));

        // append file data
        auto& item = items.at(index);
        item.size = std::filesystem::file_size(path);
        item.offset = binary_data.size();
        binary_data.resize(binary_data.size() + item.size);
        std::ifstream file;
        file.open(path.string(), std::ios_base::in | std::ios_base::binary);
        file.read((char*)&binary_data[item.offset], item.size);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to load file " + path.string());
        }
        file.close();
    }

    return n_items_in_this_folder;
}

void validate(const char*& output_path) {
    const auto filesize = std::filesystem::file_size(output_path);
    std::vector<char> contents(filesize);
    {
        std::ifstream fsfa;
        fsfa.open(output_path, std::ios_base::in | std::ios_base::binary);
        fsfa.read(contents.data(), filesize);
        fsfa.close();

        Header* header = (Header*)contents.data();
        if (
            header->file_magic[0] != 'F' ||
            header->file_magic[1] != 'S' ||
            header->file_magic[2] != 'F' ||
            header->file_magic[3] != 'A')
        {
            throw std::runtime_error("Output file validation error: file magic invalid");
        }

        if (sizeof(Header) + header->items_offset >= filesize) {
            throw std::runtime_error("Output file validation error: items list starts after the end of the file");
        }

        if (sizeof(Header) + header->items_offset + (header->n_items * sizeof(Item)) > filesize) {
            throw std::runtime_error("Output file validation error: items list ends after the end of the file");
        }

        Item* items = (Item*)(&contents[sizeof(Header) + header->items_offset]);

        if (
            items[0].name[0] != 'r' ||
            items[0].name[1] != 'o' ||
            items[0].name[2] != 'o' ||
            items[0].name[3] != 't')
        {
            throw std::runtime_error("Output file validation error: first node should be a folder named \"root\", which is not the case!");
        }

        visualize_file_structure(items);
    }

    printf("\n");
    if (filesize <= 5*1024) printf("Total - %lli bytes", filesize);
    else if (filesize <= 5*1024*1024) printf("Total - %lli KB", filesize / 1024);
    else printf("Total - %lli MB", filesize / (1024 * 1024));
    printf("\n\n");
}

void visualize_file_structure(const Item* items_list, int index, int depth) {
    printf("\n");
    for (int i = 0; i < depth; ++i) printf(".   ");
    const Item* item = &items_list[index];
    char name[13] = {0};
    char extension[4] = {0};
    memset(name, 0, sizeof(name));
    memset(extension, 0, sizeof(extension));

    if (item->type == ItemType::File) {
        strncpy(name, item->name, sizeof(name));
        strncpy(extension, item->extension, sizeof(extension));
        name[sizeof(name)-1] = 0;
        extension[sizeof(extension)-1] = 0;
        printf("File %s.%s", name, extension);
        if (item->size <= 5*1024) printf(" - %i bytes", item->size);
        else if (item->size <= 5*1024*1024) printf(" - %i KB", item->size / 1024);
        else printf(" - %i MB", item->size / (1024 * 1024));
    }

    else if (item->type == ItemType::Folder) {
        strncpy(name, item->name, sizeof(name));
        name[sizeof(name)-1] = 0;
        printf("Folder %s: (%i items, %i-%i)", name, item->size, item->offset, item->offset + item->size - 1);

        for (uint32_t i = 0; i < item->size; ++i) {
            strncpy(name, items_list[item->offset + i].name, sizeof(name));
            strncpy(extension, item->extension, sizeof(extension));
            name[sizeof(name)-1] = 0;
            extension[sizeof(extension)-1] = 0;
            visualize_file_structure(items_list, item->offset + i, depth + 1);
        }
    }

    else {
        throw std::runtime_error("Output file validation error: invalid item type");
    }
}

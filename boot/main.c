#include "uefi.h"

// Simple helper to print UTF-16 strings
void Print(EFI_SYSTEM_TABLE *SystemTable, const uint16_t *String) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (uint16_t*)String);
}

// ELF Header Definitions
typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t  Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t  Elf64_Sxword;

#define EI_NIDENT 16

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    Elf64_Half    e_type;
    Elf64_Half    e_machine;
    Elf64_Word    e_version;
    Elf64_Addr    e_entry;
    Elf64_Off     e_phoff;
    Elf64_Off     e_shoff;
    Elf64_Word    e_flags;
    Elf64_Half    e_ehsize;
    Elf64_Half    e_phentsize;
    Elf64_Half    e_phnum;
    Elf64_Half    e_shentsize;
    Elf64_Half    e_shnum;
    Elf64_Half    e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    Elf64_Word  p_type;
    Elf64_Word  p_flags;
    Elf64_Off   p_offset;
    Elf64_Addr  p_vaddr;
    Elf64_Addr  p_paddr;
    Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;
    Elf64_Xword p_align;
} Elf64_Phdr;

#define PT_LOAD 1

// Boot Info Structure to pass to the kernel
typedef struct {
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Framebuffer;
    uint64_t FramebufferBase;
    uint64_t FramebufferSize;
    EFI_MEMORY_DESCRIPTOR *MemoryMap;
    uint64_t MemoryMapSize;
    uint64_t MemoryMapDescriptorSize;
    void* Rsdp;
} BootInfo;

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
    Print(SystemTable, L"MyOS UEFI Bootloader Started.\r\n");

    EFI_STATUS Status;

    // 1. Get Graphics Output Protocol (Framebuffer)
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    Status = SystemTable->BootServices->LocateProtocol(&gopGuid, NULL, (void**)&gop);
    if (Status != EFI_SUCCESS) {
        Print(SystemTable, L"Error: Could not locate GOP.\r\n");
        while (1);
    }
    Print(SystemTable, L"GOP Located successfully.\r\n");

    // 2. Locate the volume we booted from
    EFI_GUID loadedImageGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *loadedImage;
    Status = SystemTable->BootServices->HandleProtocol(ImageHandle, &loadedImageGuid, (void**)&loadedImage);
    if (Status != EFI_SUCCESS) {
        Print(SystemTable, L"Error: Could not locate Loaded Image Protocol.\r\n");
        while (1);
    }

    EFI_GUID sfsGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfs;
    Status = SystemTable->BootServices->HandleProtocol(loadedImage->DeviceHandle, &sfsGuid, (void**)&sfs);
    if (Status != EFI_SUCCESS) {
        Print(SystemTable, L"Error: Could not locate Simple File System Protocol.\r\n");
        while (1);
    }

    EFI_FILE_PROTOCOL *root;
    Status = sfs->OpenVolume(sfs, &root);
    if (Status != EFI_SUCCESS) {
        Print(SystemTable, L"Error: Could not open volume.\r\n");
        while (1);
    }

    // 3. Open the kernel file
    EFI_FILE_PROTOCOL *kernelFile;
    Status = root->Open(root, &kernelFile, L"kernel.elf", EFI_FILE_MODE_READ, 0);
    if (Status != EFI_SUCCESS) {
        Print(SystemTable, L"Error: Could not open kernel.elf.\r\n");
        while (1);
    }
    Print(SystemTable, L"kernel.elf opened.\r\n");

    // 4. Read ELF Header
    Elf64_Ehdr header;
    uint64_t headerSize = sizeof(header);
    kernelFile->Read(kernelFile, &headerSize, &header);

    if (header.e_ident[0] != 0x7F || header.e_ident[1] != 'E' || header.e_ident[2] != 'L' || header.e_ident[3] != 'F') {
        Print(SystemTable, L"Error: Invalid ELF header.\r\n");
        while (1);
    }

    // 5. Read Program Headers and Load Segments
    Elf64_Phdr *phdrs;
    SystemTable->BootServices->AllocatePool(EfiLoaderData, header.e_phnum * header.e_phentsize, (void**)&phdrs);
    
    kernelFile->SetPosition(kernelFile, header.e_phoff);
    uint64_t size = header.e_phnum * header.e_phentsize;
    kernelFile->Read(kernelFile, &size, phdrs);

    for (int i = 0; i < header.e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            int pages = (phdrs[i].p_memsz + 0xFFF) / 0x1000;
            uint64_t physAddr = phdrs[i].p_paddr; // Assuming p_paddr is where we want it loaded physically
            
            Status = SystemTable->BootServices->AllocatePages(AllocateAddress, EfiLoaderCode, pages, &physAddr);
            if (Status != EFI_SUCCESS) {
                Print(SystemTable, L"Warning: AllocateAddress failed for segment, trying AllocateAnyPages.\r\n");
                Status = SystemTable->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderCode, pages, &physAddr);
            }

            kernelFile->SetPosition(kernelFile, phdrs[i].p_offset);
            uint64_t segSize = phdrs[i].p_filesz;
            kernelFile->Read(kernelFile, &segSize, (void*)physAddr);

            // Zero BSS
            if (phdrs[i].p_memsz > phdrs[i].p_filesz) {
                uint8_t *bss = (uint8_t*)(physAddr + phdrs[i].p_filesz);
                uint64_t bssSize = phdrs[i].p_memsz - phdrs[i].p_filesz;
                for (uint64_t j = 0; j < bssSize; j++) {
                    bss[j] = 0;
                }
            }
        }
    }
    Print(SystemTable, L"Kernel loaded into memory.\r\n");

    // 6. Get Memory Map
    EFI_MEMORY_DESCRIPTOR *Map = NULL;
    uint64_t MapSize = 0;
    uint64_t MapKey = 0;
    uint64_t DescriptorSize = 0;
    uint32_t DescriptorVersion = 0;

    SystemTable->BootServices->GetMemoryMap(&MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
    uint64_t AllocatedMapSize = MapSize + 4096; // Add plenty of slack
    MapSize = AllocatedMapSize;
    SystemTable->BootServices->AllocatePool(EfiLoaderData, MapSize, (void**)&Map);
    Status = SystemTable->BootServices->GetMemoryMap(&MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
    if (Status != EFI_SUCCESS) {
        Print(SystemTable, L"Error: Second GetMemoryMap failed!\r\n");
        while(1);
    }

    // 7. Find ACPI RSDP in ConfigurationTables
    EFI_GUID acpiTableGuid = EFI_ACPI_TABLE_GUID;
    void *rsdp = NULL;
    for (uint64_t i = 0; i < SystemTable->NumberOfTableEntries; i++) {
        EFI_GUID *guid = &SystemTable->ConfigurationTable[i].VendorGuid;
        if (guid->Data1 == acpiTableGuid.Data1 &&
            guid->Data2 == acpiTableGuid.Data2 &&
            guid->Data3 == acpiTableGuid.Data3 &&
            guid->Data4[0] == acpiTableGuid.Data4[0] &&
            guid->Data4[1] == acpiTableGuid.Data4[1] &&
            guid->Data4[2] == acpiTableGuid.Data4[2] &&
            guid->Data4[3] == acpiTableGuid.Data4[3] &&
            guid->Data4[4] == acpiTableGuid.Data4[4] &&
            guid->Data4[5] == acpiTableGuid.Data4[5] &&
            guid->Data4[6] == acpiTableGuid.Data4[6] &&
            guid->Data4[7] == acpiTableGuid.Data4[7]) {
            rsdp = SystemTable->ConfigurationTable[i].VendorTable;
            break;
        }
    }

    if (!rsdp) {
        Print(SystemTable, L"Warning: ACPI RSDP not found!\r\n");
    }

    // 8. Prepare BootInfo
    BootInfo bootInfo;
    bootInfo.Framebuffer = gop->Mode->Info;
    bootInfo.FramebufferBase = gop->Mode->FrameBufferBase;
    bootInfo.FramebufferSize = gop->Mode->FrameBufferSize;
    bootInfo.MemoryMap = Map;
    bootInfo.MemoryMapSize = MapSize;
    bootInfo.MemoryMapDescriptorSize = DescriptorSize;
    bootInfo.Rsdp = rsdp;

    // 8. Exit Boot Services
    Status = SystemTable->BootServices->ExitBootServices(ImageHandle, MapKey);
    if (Status != EFI_SUCCESS) {
        // If it fails, the map might have changed. Retry once.
        MapSize = AllocatedMapSize; // Reset MapSize to the full allocated buffer size!
        Status = SystemTable->BootServices->GetMemoryMap(&MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
        if (Status != EFI_SUCCESS) {
            Print(SystemTable, L"Error: Third GetMemoryMap failed!\r\n");
            while(1);
        }
        bootInfo.MemoryMapSize = MapSize;
        Status = SystemTable->BootServices->ExitBootServices(ImageHandle, MapKey);
    }

    // 9. Jump to Kernel (Kernel uses System V ABI, we are in MS ABI)
    void (__attribute__((sysv_abi)) *KernelEntry)(BootInfo*) = (void (__attribute__((sysv_abi)) *)(BootInfo*))header.e_entry;
    KernelEntry(&bootInfo);

    // Should never reach here
    while (1);
    return EFI_SUCCESS;
}

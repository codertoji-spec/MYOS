#pragma once

#include <stdint.h>

// Graphics Output Mode Information (subset of UEFI's EFI_GRAPHICS_OUTPUT_MODE_INFORMATION)
typedef struct {
    uint32_t Version;
    uint32_t HorizontalResolution;
    uint32_t VerticalResolution;
    uint32_t PixelFormat;
    struct {
        uint32_t RedMask;
        uint32_t GreenMask;
        uint32_t BlueMask;
        uint32_t ReservedMask;
    } PixelInformation;
    uint32_t PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    uint32_t Type;
    uint32_t Pad;
    uint64_t PhysicalStart;
    uint64_t VirtualStart;
    uint64_t NumberOfPages;
    uint64_t Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef struct {
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Framebuffer;
    uint64_t FramebufferBase;
    uint64_t FramebufferSize;
    void* MemoryMap;
    uint64_t MemoryMapSize;
    uint64_t MemoryMapDescriptorSize;
    void* Rsdp;
} BootInfo;

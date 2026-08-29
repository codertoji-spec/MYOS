# Build script for Windows using Docker
Write-Host "Building OS using Docker..."

# Check if Docker is installed
if (Get-Command docker -ErrorAction SilentlyContinue) {
    docker build -t os-builder .
    docker run --rm -v "$($PWD.Path):/os" os-builder make iso
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Build successful. Launching QEMU..."
        # QEMU must be installed on Windows
        if (Get-Command qemu-system-x86_64 -ErrorAction SilentlyContinue) {
            # Need OVMF.fd for UEFI boot
            if (!(Test-Path "OVMF.fd")) {
                Write-Host "Downloading OVMF.fd for UEFI emulation..."
                Invoke-WebRequest -Uri "https://github.com/rust-osdev/ovmf-precompiled/raw/master/OVMF-pure-efi.fd" -OutFile "OVMF.fd"
            }
            qemu-system-x86_64 -m 256M -bios OVMF.fd -drive format=raw,file=build/fat.img -serial stdio
        } else {
            Write-Host "QEMU not found on host. Please install QEMU for Windows and add it to PATH."
        }
    } else {
        Write-Host "Build failed."
    }
} else {
    Write-Host "Docker is not installed or not in PATH. Please install Docker Desktop or run via WSL."
}

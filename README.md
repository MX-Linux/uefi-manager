# uefi-manager
Tool for managing UEFI boot entries

[![latest packaged version(s)](https://repology.org/badge/latest-versions/uefi-manager.svg)](https://repology.org/project/uefi-manager/versions)
[![build result](https://build.opensuse.org/projects/home:mx-packaging/packages/uefi-manager/badge.svg?type=default)](https://software.opensuse.org//download.html?project=home%3Amx-packaging&package=uefi-manager)
[![Continous Integration](https://github.com/AdrianTM/uefi-manager/actions/workflows/main.yml/badge.svg)](https://github.com/AdrianTM/uefi-manager/actions/workflows/main.yml)

![image](https://github.com/user-attachments/assets/c76f920b-33e8-424e-a885-7ad26925b508)

## Building

```bash
./build.sh
```

Use `./build.sh --debug` for debug builds or `./build.sh --clang` for clang builds.

## Functionality

**Manage UEFI entries** - View, add, modify, and delete UEFI boot entries using efibootmgr

**EFI stub installer** - Copy kernel and initrd to ESP and create direct UEFI boot entry bypassing GRUB

**Frugal EFI stub installer** - Create bootable entries for MX/antiX frugal installations with direct EFI boot

## AppImage

Download the x86-64 AppImage from a GitHub release’s **Assets**, make it executable with `chmod +x`, and run it as your desktop user. It bundles `efibootmgr`, its EFI libraries, Qt, translations, and the UEFI Manager helpers.

Viewing and editing real entries requires a UEFI-booted system with accessible EFI variables. Changes use the host’s `pkexec` and polkit authentication agent. The authorization step reopens the AppImage using FUSE. On a host without FUSE, extract it with `--appimage-extract` and run `squashfs-root/AppRun`. Disk operations use the host’s standard utilities and encrypted partitions require `cryptsetup`.

To build locally on Debian 13 (trixie), install the build dependencies and run the packaging script.

```sh
sudo apt-get install ca-certificates git build-essential cmake ninja-build dpkg-dev curl file \
    qt6-base-dev qt6-tools-dev qt6-tools-dev-tools qt6-svg-plugins qt6-translations-l10n \
    qt6-wayland efibootmgr
./make-appimage.sh
```

The script writes `build-appimage/uefi-manager-<version>-x86_64.AppImage` using the version in `debian/changelog`. GitHub Actions builds and attaches the image to each non-Arch tagged release.

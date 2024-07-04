# Command Table

- **Help** (`?`): Displays help information about how to use the program and its commands.
- **List bios** (`ls`): Lists information about the BIOS file, such as its size and the contents of its banks.
- **Split bios** (`split`): Splits a BIOS file into its constituent banks.
- **Combine banks** (`combine`): Combines multiple bank files into a single BIOS file.
- **Extract bios** (`extr`): Extracts information from a BIOS file, such as its public key or xcodes.
- **Build bios** (`bld`): Builds a BIOS file from a set of input files.
- **Simulate xcodes** (`xcode-sim`): Simulates the execution of xcodes, which are instructions for initializing the BIOS.
- **Decode xcodes** (`xcode-decode`): Decodes the xcodes from a BIOS file or an initialization table.
- **Decompress krnl** (`decomp-krnl`): Decompresses the kernel from a BIOS file.

Each command has a set of required and optional switches that modify that specific command's behavior. For example:
- The "Split bios" command requires the `-bios` and `-romsize` switches, which specify the BIOS file to split and the size of the ROM, respectively.
- The "list bios" command requires the `-bios` switch. but has a few optional switches to hone-down what you are actually interested in. (-xcodes, -2bl, -datatbl -inittbl)

# Terminology
**BIN SIZE**: This refers to the actual size of the BIOS binary file (with the `.bin` extension) on your disk. This size is determined by the total number of bytes that make up the file. You can think of it as the physical space that the file takes up on your hard drive.

**ROM SIZE**: This is used to determine how much space is available inside the BIN size. If the **ROM SIZE** is less than the BIN size, the ROM would be replicated up to the **BIN SIZE**. This means that the BIOS file will be repeated as many times as necessary to fill up the BIN size.
Example: you can have a 1MB bios.bin file and that file could contain `4x 256KB image` or `2x 512KB image` or a `1x 1MB image`. **ROM SIZE** should not exceed the bin (file) size of the bios.

# Examples

```xbios.exe -ls bios.bin```        - Would list bios infomation. (most info is encrypted, so you will have to provide a key to atleast decrypt the 2bl.

```xbios.exe -xcode-sim bios.bin``` - would simulate xc_mem_write (useful for viewing implementations of the visor hack (wrap-around) in the xcodes)

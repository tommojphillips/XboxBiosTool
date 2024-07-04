# Command Table

- `?` - Displays help information about how to use the program and its commands.
- `ls` - Lists information about the BIOS file, boot params, sizes, rc4 keys.
- `split` - Splits a BIOS file into banks based on romsize. Eg bios.bin 1Mb -> bios.bin 4x 256Kb
- `combine` - Combine multiple bank files into a single bios file. Eg bios.bin x4 256Kb -> bios.bin 1Mb
- `extr` - Extract the bootloader, compressed kernel, uncompressed kernel data and init table from a bios.
- `bld` - Builds a BIOS file from a set of input files.
- `xcode-sim` - Simulates mem-write xcodes and parse x86 opcodes. (visor sim)
- `xcode-decode` - Decodes the xcodes from a BIOS file or an extracted init table
- `decomp-krnl` - Decompresses the kernel from a BIOS file.

# Switches

- `enc-bldr` if flag is supplied the 2BL is assumed to be *unencrypted* within the bios file.
- `enc-krnl` if flag is supplied the kernel is assumed to be *unencrypted* within the bios file.
- `key-bldr` The path to the 16-byte .bin rc4 key file.
- `key-krnl` The path to the 16-byte .bin rc4 key file.
- `mcpx`     The path to the MCPX rom, used for en/decryption

# Notes / Comments
- Each command has a set of required and optional switches that modify that specific command's behavior. For example:
  - The "Split bios" command requires the `-bios` and `-romsize` switches, which specify the BIOS file to split and the size of the ROM, respectively.
  - The "list bios" command requires the `-bios` switch and has a few switches to hone-down what you are actually looking for. ( `-2bl`, `-nv2a`, `-datatbl` )

- If an argument is provided without a switch, it is inferred to be associated with the `-bios` switch. This means that you don't necessarily have to explicitly use the `-bios` switch when providing your BIOS file. You can simply provide the argument and the program will understand that it is meant to be used with the `-bios` switch.

- In order to decrypt parts of the BIOS, ( preldr, 2bl, krnl ) you will need to provide a 16-byte RC4 key file `key.bin` or the correct MCPX version specific to your BIOS. Without these, the decryption process cannot proceed.
  - Please note that this repository does not contain any crypto keys. **You will need to provide your own key file or a MCPX rom**.

- Supports all versions of the BIOS, However, to ensure correct decryption, you will need to supply the appropriate MCPX version for your specific BIOS.
  - Please note that while this program supports all versions of the BIOS, there is no guarantee that it will work with custom BIOS versions. The functionality and compatibility with custom BIOS versions may vary. Always ensure to have a backup and proceed with caution when working with custom BIOS versions.
# Terminology
**BIN SIZE**: This refers to the actual size of the BIOS binary file (with the `.bin` extension) on your disk. This size is determined by the total number of bytes that make up the file. You can think of it as the physical space that the file takes up on your hard drive.

**ROM SIZE**: This is used to determine how much space is available inside the BIN size. If the **ROM SIZE** is less than the BIN size, the ROM would be replicated up to the **BIN SIZE**. This means that the BIOS file will be repeated as many times as necessary to fill up the BIN size.
Example: you can have a 1MB bios.bin file and that file could contain `4x 256KB image` or `2x 512KB image` or a `1x 1MB image`. **ROM SIZE** should not exceed the bin (file) size of the bios.

# Examples

```xbios.exe -ls bios.bin``` - Would list bios infomation. (most info is encrypted, so you will have to provide a key to atleast decrypt the 2bl.

```xbios.exe -ls bios.bin -mcpx mcpx.bin``` - Would load the bios, decrypt using the mcpx rom and list bios infomation.

```xbios.exe -ls bios.bin -key-bldr rc4_key.bin``` - Would load the bios, decrypt using the 16-byte key file and list bios infomation.

```xbios.exe -xcode-sim bios.bin``` - would simulate xc_mem_write (useful for viewing implementations of the visor hack )

```xbios.exe -xcode-decode bios.bin``` - would interp/decode xcodes 

```xbios.exe -decomp-krnl bios.bin -mcpx mcpx.bin``` - Load the bios, decrypt it, and decompress the kernel to 'krnl.img'.

# Example output:
Xcode simulator (visor hack sim)

![xcode_sim__eg](https://github.com/tommojphillips/XboxBiosTool/assets/39871058/1cfcfafa-9574-498d-86df-d2c3002266ed)

Xcode decode:
![Screenshot 2024-07-04 161539](https://github.com/tommojphillips/XboxBiosTool/assets/39871058/cc215e40-0c8a-4a99-b889-11cac7e649f0)


# Command Table

| Command        | Description                                                                 |
| -------------- | --------------------------------------------------------------------------- |
| `-?`           | Displays help information about how to use the program and its commands.    |
| `-ls`          | Lists information about the bios file, boot params, sizes, rc4 keys.        |
| `-split`       | Splits a bios into banks based on romsize.                                  |
| `-combine`     | Combine multiple bios files into a single bios.                             |
| `-extr`        | Extract the 2bl, compressed kernel+data, init table from a bios.            |
| `-bld`         | Builds a bios from an extracted 2bl, compressed kernel+data, init table.    |
| `-xcode-sim`   | Simulates mem-write xcodes and parse x86 opcodes.                           |
| `-xcode-decode`| Decode xcodes from a bios or an extracted init table.                       |
| `-decomp-krnl` | Decompress the kernel from a bios.                                          |

# Switches
#### General switches
| Switch            | Description                                                  |
| ----------------- | ------------------------------------------------------------ |
| `-enc-bldr`       | If supplied, the 2BL is assumed to be *unencrypted*.         |
| `-enc-krnl`       | If supplied, the kernel is assumed to be *unencrypted*.      |
| `-key-bldr <path>`| The path to the 16-byte .bin RC4 key file.                   |
| `-key-krnl <path>`| The path to the 16-byte .bin RC4 key file.                   |
| `-mcpx <path>`    | The path to the MCPX ROM.                                    |
| `-d`              | The dump switch.                                             |

| List flag  | Description                                                  |
| ---------- | ------------------------------------------------------------ |
| `-nv2a`    | (List flag) Display NV2A init table magic values.            |
| `-datatbl` | (List flag) Display ROM drive / slew calibration table data. |
| `-2bl`     | (List flag) Display 2BL boot parameters, sizes.              |

# Notes / Comments

- In order to decrypt parts of the bios, ( preldr, 2bl, krnl ) you will need to provide a 16-byte RC4 key file `rc4_key.bin` or the correct MCPX rom specific to your bios. Without these, the decryption process will fail.
  - **You will need to provide your own key file or a MCPX rom**.

- Supports all versions of the bios, however, to ensure correct decryption, you will need to supply the appropriate 2bl rc4 key or MCPX rom for your specific bios.
  - Note that while this program supports all versions of the bios, there is *no guarantee* that it will work with custom bios versions.
# Terminology
**BIN SIZE** refers to the size of the bios file.

  **ROM SIZE** refers to the size of the bios image inside the bios file. If ROM SIZE is less than BIN SIZE, the bios image is replicated upto BIN SIZE.
  * EG: `1MB bios.bin` could contain `4x 256KB img` or `2x 512KB img` or  `1x 1MB img`. 
  * should not exceed BIN SIZE.

Build (-bld) command curently only supports *compressed kernel binaries*, not *decompressed kernel images*. You will need to supply a `compressed_krnl.bin` and a `uncompressed_krnl_data.bin`. `uncompressed_krnl_data.bin` is a section. specifically, `.tad`. copied from a kernel image. The `-extr` command will extract this out of a bios for you.

An original bios includes rc4 keys in the 2bl. To decrypt the 2bl, provide either an RC4 key file or an MCPX ROM file. This allows the kernel key to be located and the kernel to be decrypted.

If your bios uses a bootloader that is compatible with the original boot process and doesn't encrypt its bootloader or kernel, you can use `enc-bldr` and `enc-krnl` to prevent them from being decrypted.

# Examples

```xbios.exe -ls bios.bin``` - List bios info. (most info is encrypted)

```xbios.exe -ls bios.bin -mcpx mcpx.bin``` - Decrypt 2bl with mcpx rom and list bios info.

```xbios.exe -ls bios.bin -key-bldr rc4_key.bin``` - Decrypt 2bl with rc4 key file and list bios info.

```xbios.exe -xcode-sim bios.bin``` - Simulate xcode memory writes and parse x86 instructions

```xbios.exe -xcode-decode bios.bin``` - Decode xcode instructions. 

```xbios.exe -decomp-krnl bios.bin -mcpx mcpx.bin``` - Decrypt and decompress the kernel.

```xbios.exe -decomp-krnl bios.bin -enc-krnl``` - Decompress the *unencrypted* kernel.

```xbios.exe -split bios.bin -romsize 512``` - Split the bios into 512KB banks.

```xbios.exe -combine bank1.bin bank2.bin bank3.bin bank4.bin``` - Combine the banks into a single bios file.

# Example output:
Xcode simulator (visor hack sim)

![xcode_sim__eg](https://github.com/tommojphillips/XboxBiosTool/assets/39871058/1cfcfafa-9574-498d-86df-d2c3002266ed)

Xcode decode:
![Screenshot 2024-07-04 161539](https://github.com/tommojphillips/XboxBiosTool/assets/39871058/cc215e40-0c8a-4a99-b889-11cac7e649f0)


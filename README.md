# Command Table

| Command        | Description                                                                 |
| -------------- | --------------------------------------------------------------------------- |
| `-?`           | Displays help message                                                       |
| `-ls`          | Lists bios info. boot params, sizes, etc.                            |
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
| `-enc-bldr`       | If provided, the 2bl is assumed to be *unencrypted*.         |
| `-enc-krnl`       | If provided, the kernel is assumed to be *unencrypted*.      |
| `-key-bldr <path>`| The path to the 16-byte 2bl RC4 file                         |
| `-key-krnl <path>`| The path to the 16-byte kernel RC4 file                      |
| `-mcpx <path>`    | The path to the MCPX ROM.                                    |
| `-d`              | The dump switch.                                             |

| List flag  | Description                                                  |
| ---------- | ------------------------------------------------------------ |
| `-nv2a`    | (List flag) Display NV2A init table magic values.            |
| `-datatbl` | (List flag) Display ROM drive / slew calibration table data. |

# Notes / Comments

- Supports all versions of the bios, however, to ensure correct decryption, you will need to provide a 16-byte rc4 2bl key file `-key-bldr rc4_key.bin` or MCPX rom `-mcpx mcpx.bin` for your specific bios.

- While this program supports all versions of the bios, there is *no guarantee* that it will work with custom bios versions.
  
- An original bios includes its rc4 keys in the 2bl. provide either an RC4 key file or an MCPX ROM file. This allows the kernel key to be located and the kernel to be decrypted.

- If the kernel key is locatable in the 2bl, it will be used for decryption. Use `-enc-krnl` if you dont want the kernel de/encrypted. If a custom bios removes this pointer or the keys, the keys will not be locatable. You would have to provide a kernel key with `-key-krnl <path>`.

# Build command:
- With the build command, `-enc-krnl` basiclly works backwards. Explictly provide `-enc-krnl` if you want the kernel encrypted with the kernel key located in the 2bl.
- Curently only supports *compressed kernel files*. You will need to supply a `compressed_krnl.bin` and a `uncompressed_krnl_data.bin`. The `-extr` command can obtain these files.

# Extract command:
- The extract command extracts the kernel **compressed**. Thus it is possible to extract it either encrypted or decrypted. Make a note of what state it's in. If the kernel is decrypted when its extracted you will see a message like '*Decrypting Kernel..*'.

# Terminology
**BIN SIZE** refers to the size of the bios file.

  **ROM SIZE** refers to the size of the bios image inside the bios file. If ROM SIZE is less than BIN SIZE, the bios image is replicated upto BIN SIZE.
  * EG: `1MB bios.bin` could contain `4x 256KB img` or `2x 512KB img` or  `1x 1MB img`. 
  * should not exceed BIN SIZE.

# Example commands:

```xbios.exe -ls bios.bin``` - List bios info. (most info is encrypted)

```xbios.exe -ls bios.bin -mcpx mcpx.bin``` - Decrypt 2bl with mcpx rom and list bios info.

```xbios.exe -ls bios.bin -key-bldr rc4_key.bin``` - Decrypt 2bl with rc4 key file and list bios info.

```xbios.exe -xcode-sim bios.bin``` - Simulate xcode memory writes and parse x86 instructions

```xbios.exe -xcode-sim bios.bin -d``` - Simulate xcode memory writes and parse x86 instructions. Dump mem sim to mem_dump.bin

```xbios.exe -xcode-decode bios.bin``` - Decode xcode instructions to console.

```xbios.exe -xcode-decode bios.bin -d``` - Decode xcode instructions to xcodes.txt

```xbios.exe -decomp-krnl bios.bin -mcpx mcpx.bin -enc-krnl``` - Decompress *unencrypted* kernel.

```xbios.exe -decomp-krnl bios.bin -mcpx mcpx.bin``` - Decrypt kernel if kernel key can be located in 2bl, Decompress kernel.

```xbios.exe -decomp-krnl bios.bin -mcpx mcpx.bin -key-krnl rc4_key.bin``` - Decrypt kernel using rc4_key.bin, Decompress kernel.

```xbios.exe -split bios.bin -romsize 512``` - Split the bios into 512KB banks.

```xbios.exe -combine bank1.bin bank2.bin bank3.bin bank4.bin``` - Combine the banks into a single bios file.

# Example output:
Xcode simulator (visor hack sim)

![xcode_sim__eg](https://github.com/tommojphillips/XboxBiosTool/assets/39871058/1cfcfafa-9574-498d-86df-d2c3002266ed)

Xcode decode:
![Screenshot 2024-07-04 161539](https://github.com/tommojphillips/XboxBiosTool/assets/39871058/cc215e40-0c8a-4a99-b889-11cac7e649f0)


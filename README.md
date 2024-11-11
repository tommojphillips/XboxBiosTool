
A command-line tool for extracting components of An Original Xbox BIOS.

## Table of Contents

  - [Encryption / Decryption](#encryption-decryption)
  - [What MCPX ROM do i use?](#what-mcpx-rom-do-i-use)
  - [List BIOS command](#list-bios-command)
  - [Extract BIOS command](#extract-bios-command)
  - [Build BIOS command](#build-bios-command)
  - [X86 Encode command](#x86-encode-command)
  - [Xcode sim command](#xcode-visor-sim-command)
  - [Xcode decode command](#xcode-decode-command)
  - [Compress file command](#compress-file-command)
  - [Decompress file command](#decompress-file-command)
  - [Credits / Resources](#credits-resources)

## Commands
| Command                                  | Desc                                       |
| ---------------------------------------- | ------------------------------------------ |
| `-?`                                     | Displays help message                      |
| [`-ls`](#list-bios-command)              | Dump BIOS info.                            |
| [`-extr`](#extract-bios-command)         | Extract components from a BIOS.            |
| [`-bld`](#build-bios-command)            | Build a BIOS.                              |
| `-split`                                 | Split a BIOS into banks.                   |
| `-combine`                               | Combine multiple BIOSes into a single BIOS.|
| `-replicate`                             | replicate a BIOS.                          |
| [`-xcode-sim`](#xcode-visor-sim-command) | Simulate xcodes and Decode x86 (visor sim) |
| [`-xcode-decode`](#xcode-decode-command) | Decode Xcodes from a BIOS or init table.   |
| [`-x86-encode`](#x86-encode-command)     | Encode x86 as xcodes. (visor)              |
| [`-compress`](#compress-file)            | compress a file using lzx.                 |
| [`-decompress`](#decompress-file)        | decompress a file using lzx.               |


## Switches
| Switch            | Description                                                       |
| ----------------- | ----------------------------------------------------------------- |
| `-?`              | Get help about a specific command.                                |
| `-enc-bldr`       | Assume the 2BL is unencrypted. Decryption will be skipped.        |
| `-enc-krnl`       | Assume the kernel is unencrypted. Decryption will be skipped.     |
| `-key-bldr <path>`| 16-byte 2BL RC4 file.                                             |
| `-key-krnl <path>`| 16-byte kernel RC4 file.                                          |
| `-mcpx <path>`    | MCPX ROM file. Used for en/decrypting the 2BL.                    |
| `-romsize`        | How much space is available for the BIOS in kb, (256, 512, 1024). |
| `-binsize`        | Total space of the file or flash in kb. (256, 512, 1024).         |

## Notes / Comments
- Supports all Original Xbox BIOSes.
- There is *no guarantee* that this program will work correctly with modified BIOSes.

## Encryption / Decryption  
In Order to locate, decrypt and decompress the kernel image,
the 2BL needs to be decrypted first to calculate the offsets to the kernel image.

1. **Provide a RC4 2BL key file or MCPX ROM to decrypt the 2BL**:
    - Use `-key-bldr <path>` to specify the key from a file. (*16-byte file*) 
    - Use `-mcpx <path>` to specify the key from the MCPX ROM file. (*512-byte file*)

If the 2BL has been decrypted, you will see a message like `Decrypting 2BL..`

---

2. **Locate or provide a RC4 Kernel Key to decrypt the kernel image**:
    - The `RC4 Kernel Key` can be found in the decrypted 2BL automaticly.
    - Use `-key-krnl <path>` to specify a kernel key from a file. (*16-byte file*) 
    - Use `-enc-krnl` if you do not want the kernel decrypted.

If the kernel has been decrypted, you will see a message like `Decrypting Kernel..`

---

## What MCPX ROM do i use?
It depends on the BIOS version.

| Version          | BIOS Range | MD5 Hash                           |
| ---------------- | ---------- |----------------------------------- |
| MCPX v1.0        | < 4817     | `d49c52a4102f6df7bcf8d0617ac475ed` |
| MCPX v1.1        | > 4627     | `2870d58a459c745d7cc4c6122ceb3dcb` |
| M.O.U.S.E rev. 0 | < 4817     | `58f414016093f289c46d21639435701e` |
| M.O.U.S.E rev. 1 | > 4627     | `06b227adbefc4dd55fb127c33590b735` |

---

## List BIOS command
Dump infomation about the BIOS.

The list command has some flags to dump out infomation in a BIOS.

| Switch        | Desc                                             |
| ------------- | ------------------------------------------------ |
| `-img`        | Dump kernel image header info.                   |
| `-nv2a`       | Dump init table magic values.                    |
| `-datatbl`    | Dump ROM drive / slew calibration table data.    |
| `-keys`       | Dump rc4, rsa keys.                              |
| `-bootable`   | Run checks to see if and how a BIOS gets to 2BL. |

```
xbios.exe -ls <bios_file> -mcpx <mcpx_file> <extra_flags>
```

## Extract BIOS command
Extract components from a BIOS file 
- `Bldr (2BL)`
- `Preldr (FBL)`
- `Init table (magic numbers, xcodes)`
- `Compressed kernel image (.bin)`
- `Uncompressed kernel section data`
- `Decompressed kernel image (pe/coff .img, really its a .exe)`
- `Encrypted rom digest`
- `RC4 keys`
- `RSA keys`

| Switch              | Desc                                      |
| ------------------- | ---------------------                     |
| `-bldr <path>`      | Output 2BL file                           |
| `-inittbl <path>`   | Output init table file                    |
| `-krnl <path>`      | Output compressed kernel file             |
| `-krnldata  <path>` | Output uncompressed data section file     |
| `-preldr <path>`    | Output preldr file                        |
| `-eepromkey <path>` | Output eeprom key file                    |
| `-certkey <path>`   | Output cert key file                      |
| `-dir <path>`       | Set output directory                      |
| `-keys`             | Extract keys                              |
| `-nobootparams`     | Dont restore 2BL boot params (FBL BIOSes) |

```
xbios.exe -extr <bios_file> -mcpx <mcpx_file> <extra_flags>
```

## Build BIOS command
Build a BIOS from a 2BL, compressed kernel, uncompressed data section, init table.

| Switch              | Desc                                                    |
| ------------------- | ------------------------------------------------------- |
| `-bldr <path>`      | 2BL file (req)                                          |
| `-inittbl <path>`   | Init table file (req)                                   |
| `-krnl <path>`      | Compressed kernel file (req)                            |
| `-krnldata  <path>` | Uncompressed data section file (req)                    |
| `-preldr <path>`    | Preldr (FBL) file                                       |
| `-xcodes <path>`    | Inject xcodes file                                      |
| `-romsize <size>`   | romsize in kb.                                          |
| `-binsize <size>`   | binsize in kb.                                          |
| `-enc-krnl`         | Locate the kernel key in 2BL and use it for encryption. |
| `-bfm`              | Patch BIOS to boot from media. ( bfm )                  |
| `-hackinittbl`      | Hack initbl size (size = 0)                             |
| `-hacksignature`    | Hack 2BL boot signature (signature = 0xFFFFFFFF)        |
| `-nobootparams`     | Dont update boot params.                                |

The switch, `-enc-krnl` works different with this command. Provide the flag 
*if you want the kernel encrypted* with the kernel key located in the 2BL.

The switch, `-xcodes` injects the xcodes at the end of the xcode table. 
If no space is available, (no zero space) the exit xcode is replaced with
a jump to free space where the xcodes will be injected.

```
xbios.exe -bld -bldr <bldr> -inittbl <inittbl> -krnl <krnl> -krnldata <krnl_data> <flags>
```

## X86 encode command
Encode x86 *machine code* as xcode *byte code* that writes to RAM.

| Switch        | Desc                                       |
| ------------- | ------------------------------------------ |
| `-in <path>`  | Input file.                                |
| `-out <path>` | Output file. Defaults to xcodes.bin        |

  - Start address of mem-write is 0. Each write is increment by 4 bytes.
  - Code size increases by a factor of x2.25.

```
xbios.exe -x86-encode <code_file> -out <output_xcodes>
```

<details><summary>Example</summary>

```
X86:            --->    Xcodes:
 mov eax, 0xfff00bed      xc_mem_write 0x00, 0xf00bedb8
 jmp eax                  xc_mem_write 0x04, 0x90e0ffff
 nop

Machine code:   --->    Byte code:
  0000: B8 ED 0B F0       0000: 03 00 00 00
  0004: FF FF E0 90       0004: 00 F0 0B ED
                          0008: B8 03 00 00
                          000C: 00 04 90 E0
                          0010: FF FF
```

</details>


## Xcode (visor) sim command
Simulate mem-write xcodes and parse x86 machine code. (visor sim)

| Switch           | Desc                                                | Default |
| ---------------- | --------------------------------------------------- | ------- |
| `-base <addr>`   | Base address of xcodes.                             | `0x80`  |
| `-offset <addr>` | xcode address start offset.                         | `0x00`  |
| `-nomaxsize`     | Dont limit the size of the xcode file.              | `true`  |
| `-d`             | Write to a file. Use `-out` to specify output file. | `false` |
| `-simsize`       | Size of the sim space in bytes.                     | `0x20`  |

If simulating a file other than a BIOS or extracted init table, 
The `base` of the xcodes might be different. Use `-base <addr>` to specify.

```
xbios.exe -xcode-sim <bios_file> <extra_flags>
```

<details><summary>Example output</summary>

```
Xcodes:
  1a9f: xc_mem_write 0x00, 0x7fc900be
  1aa8: xc_mem_write 0x04, 0x0000bfff
  1ab1: xc_mem_write 0x08, 0x00b90009
  1aba: xc_mem_write 0x0c, 0xfc000018
  1ac3: xc_mem_write 0x10, 0x1d8ba5f3
  1acc: xc_mem_write 0x14, 0x00090000
  1ad5: xc_mem_write 0x18, 0x0000e3ff
  1ade: xc_mem_write 0x1c, 0x00000000

Assembly:
  mov esi, dword 0xff7fc900
  mov edi, dword 0x90000
  mov ecx, dword 0x1800
  cld
  rep movsd
  mov ebx, dword ptr [0x90000]
  jmp ebx
```

</details>


## Xcode decode command
Decode Xcodes from a BIOS file, extracted init table.

| Switch         | Desc                                                | Default |
| -------------- | --------------------------------------------------- | ------- |
| `-base <addr>` | Base address for xcodes.                            | `0x80`  |
| `-nomaxsize`   | Dont limit the size of the xcode file.              | `true`  |
| `-d`           | Write to a file. Use `-out` to specify output file. | `false` |
| `-ini <path>`  | Decode settings file.                               | Default settings |

If decoding a file other than a BIOS or extracted init table, 
The `base` of the xcodes might be different. Use `-base <addr>` to specify.

See: [Xcode Decode Settings](./DecodeSettings.md) for more infomation.

```
xbios.exe -xcode-decode <bios_file> <extra_flags>
```

## Compress file command
Compress a file using lzx

| Switch         | Desc                |
| -------------- | ------------------- |
| `-in <path> `  | Input file          |
| `-out <path>`  | Output file         |

```
xbios.exe -compress <in_file> <out_file>
```

## Decompress file command
Decompress a file using lzx

| Switch         | Desc                |
| -------------- | ------------------- |
| `-in <path> `  | Input file          |
| `-out <path>`  | Output file         |

```
xbios.exe -decompress <in_file> <out_file>
```

## Credits / Resources

 - https://github.com/XboxDev/xbedump - XboxDev sha1.c implementation
 - https://github.com/WulfyStylez/XBOverclock - WulfyStylez GPU clock caluclations
 - https://xboxdevwiki.net/Boot_Process - Boot process



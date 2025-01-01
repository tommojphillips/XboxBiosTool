
# Xbox Bios Tools

A command-line tool for extracting and decrypting components of An Original Xbox BIOS.

## Table of Contents

  - [Commands](#commands)
  - [Encryption / Decryption](#encryption-decryption)
  - [What MCPX ROM do i use?](#what-mcpx-rom-do-i-use)
  - [Examples](#example-commands)
  - [Credits / Resources](#credits-resources)

## Commands
| Command                                  | Desc                                       |
| ---------------------------------------- | ------------------------------------------ |
| [`/?`](#help-command)                    | Displays help message                      |
| [`/ls`](#list-bios-command)              | Display BIOS infomation                    |
| [`/extr`](#extract-bios-command)         | Extract components from a BIOS             |
| [`/bld`](#build-bios-command)            | Build a BIOS                               |
| [`/split`](#split-bios-command)          | Split a BIOS into banks                    |
| [`/combine`](#combine-bios-command)      | Combine multiple banks into a single BIOS  |
| `/replicate`                             | replicate a BIOS                           |
| [`/xcode-sim`](#xcode-visor-sim-command) | Simulate xcodes and Decode x86             |
| [`/xcode-decode`](#xcode-decode-command) | Decode Xcodes from a BIOS or init table    |
| [`/x86-encode`](#x86-encode-command)     | Encode x86 as xcodes                       |
| [`/compress`](#compress-file-command)    | Compress a file using lzx                  |
| [`/decompress`](#decompress-file-command)| Decompress a file using lzx                |


## Switches
| Switch            | Description                                                       |
| ----------------- | ----------------------------------------------------------------- |
| [`/?`](#help-command) | Get help about a specific command                                 |
| `/enc-bldr`       | Assume the 2BL is unencrypted (Decryption will be skipped)        |
| `/enc-krnl`       | Assume the kernel is unencrypted  (Decryption will be skipped)    |
| `/key-bldr <path>`| 16-byte 2BL RC4 file                                              |
| `/key-krnl <path>`| 16-byte kernel RC4 file                                           |
| `/mcpx <path>`    | MCPX ROM file. Used for en/decrypting the 2BL                     |
| `/romsize <size>` | How much space is available for the BIOS in kb, (256, 512, 1024)  |
| `/binsize <size>` | Total space of the file or flash in kb  (256, 512, 1024)          |

## Notes / Comments
- Supports all Original Xbox BIOSes.
- There is *no guarantee* that this program will work correctly with modified BIOSes.

## Encryption / Decryption  
The 2BL needs to be decrypted to calculate the offsets to the kernel image.
You need to provide the `RC4 2BL key` in the form of a *MCPX rom* or a *16-byte file*.

If the 2BL isn't encrypted, you can ignore these switches.

 - Use `/key-bldr <path>` to specify the key from a file. (*16-byte file*) 
- Use `/mcpx <path>` to specify the key from the MCPX ROM file. (*512-byte file*)

If the 2BL has been decrypted, you will see a message like `decrypting 2BL`

---
The Kernel needs to be decrypted before decompressing it. The `RC4 Kernel Key`
can be found in the decrypted 2BL or you can provide it.

If the Kernel isn't encrypted, Use `/enc-krnl` to specify do not want the kernel decrypted.

- Use `/key-krnl <path>` to specify a kernel key from a file. (*16-byte file*) 
- Use `/enc-krnl` if you do not want the kernel decrypted.

If the kernel has been decrypted, you will see a message like `decrypting kernel`

---

## What MCPX ROM do i use?
It depends on the BIOS version. The different revisions of the MCPX have
different keys and different hashing algorithms

| Rev. 0           | MD5 Hash                           |
| ---------------- |----------------------------------- |
| MCPX v1.0        | `d49c52a4102f6df7bcf8d0617ac475ed` |
| M.O.U.S.E rev. 0 | `58f414016093f289c46d21639435701e` |

| Rev. 1           | MD5 Hash                           |
| ---------------- | ----------------------------       |
| MCPX v1.1        | `2870d58a459c745d7cc4c6122ceb3dcb` |
| M.O.U.S.E rev. 1 | `06b227adbefc4dd55fb127c33590b735` |


Typically, if the BIOS contains a Preldr (FBL), Use a ` Rev. 1  MCPX` .
This is only true if the FBL TEA Attack hasn't been applied. in that instance
it's likely that the BIOS is not encrypted at all, and the startup format maybe different.

```
xbios.exe /<command> /mcpx <mcpx_file>
```

---

## Help command
Display command/switch list:
```
xbios.exe /?
```

Display help about a specific command:
```
xbios.exe /? <command>
```

Display help about Encryption:
```
xbios.exe /? /help-enc
```

## List BIOS command
Display infomation about the BIOS.

The list command has some flags to display specific infomation.

| Switch        | Desc                                                |
| ------------- | ------------------------------------------------    |
| `/in <path> ` | BIOS file (req)                                     |
| `/img`        | Display kernel image header info                    |
| `/nv2a`       | Display init table magic values                     |
| `/datatbl`    | Display ROM drive / slew calibration table data     |
| `/keys`       | Display rc4, rsa keys                               |

```
xbios.exe /ls <bios_file> /mcpx <mcpx_file> <extra_flags>
```

## Extract BIOS command
Extract components from a BIOS file 
- `Bldr (2BL)`
- `Preldr (FBL)`
- `Init table (magic numbers, xcodes)`
- `Compressed & Decompressed kernel image (.bin) (.img)`
- `Uncompressed kernel section data`
- `RC4, RSA keys`

| Switch              | Desc                                      |
| ------------------- | ---------------------                     |
| `/in <path> `       | BIOS file (req)                           |
| `/dir <path>`       | Set output directory                      |
| `/keys`             | Extract keys                              |
| `/nobootparams`     | Dont restore 2BL boot params (FBL BIOSes) |

| Output file         | Desc                                      |
| ------------------- | ---------------------                     |
| `/bldr <path>`      | Output 2BL file                           |
| `/inittbl <path>`   | Output init table file                    |
| `/krnl <path>`      | Output compressed kernel file             |
| `/krnldata  <path>` | Output uncompressed data section file     |
| `/preldr <path>`    | Output preldr file                        |
| `/eepromkey <path>` | Output eeprom key file                    |
| `/certkey <path>`   | Output cert key file                      |

```
xbios.exe /extr <bios_file> /mcpx <mcpx_file> <extra_flags>
```

## Build BIOS command
Build a BIOS from a 2BL, compressed kernel, uncompressed data section, init table.

| Switch              | Desc                                                    |
| ------------------- | ------------------------------------------------------- |
| `/out <path>`       | Output BIOS file; defaults to bios.bin                  |
| `/xcodes <path>`    | Inject xcodes file                                      |
| `/romsize <size>`   | romsize in kb (256, 512, 1024)                          |
| `/binsize <size>`   | binsize in kb (256, 512, 1024)                          |
| `/enc-krnl`         | Locate the kernel key in 2BL and use it for encryption  |
| `/bfm`              | Patch BIOS to boot from media ( bfm )                   |
| `/hackinittbl`      | Hack initbl size (size = 0)                             |
| `/hacksignature`    | Hack 2BL boot signature (signature = 0xFFFFFFFF)        |
| `/nobootparams`     | Dont update boot params                                 |

| Input file          | Desc                                                    |
| ------------------- | ------------------------------------------------------- |
| `/bldr <path>`      | Input 2BL file (req)                                    |
| `/inittbl <path>`   | Input Init table file (req)                             |
| `/krnl <path>`      | Input Compressed kernel file (req)                      |
| `/krnldata  <path>` | Input Uncompressed data section file (req)              |
| `/preldr <path>`    | Input Preldr (FBL) file                                 |

The switch, `-enc-krnl` works different with this command. Provide the flag 
*if you want the kernel encrypted* with the kernel key located in the 2BL.

The switch, `-xcodes` injects the xcodes at the end of the xcode table. 
If no space is available, (no zero space) the exit xcode is replaced with
a jump to free space where the xcodes will be injected.

```
xbios.exe /bld /bldr <bldr> /inittbl <inittbl> /krnl <krnl> /krnldata <krnl_data> <extra__flags>
```

## Split BIOS command
Split a BIOS into banks.

| Switch         | Desc                                            |
| -------------  | ----------------------------------------------- |
| `/in <path> `  | BIOS file (req)                                 |
| `/romsize`     | Bank size (256, 512, 1024) ; defaults to 256kb  |

```
xbios.exe /split <bios_file> /romsize <bank_size>
```

## Combine BIOS command
Combine multiple banks into a single BIOS file. 
Provide atleast 2 banks to combine.

The `-bank[1-4]` switches are inferred with this command. 
 - The first file provided without a switch (`/<switch>`) is bank[1]
 - The second file provided without a switch (`/<switch>`) is bank[2]
 - etc

| Switch              | Desc                                            |
| -------------       | ----------------------------------------------- |
| `-bank[1-4] <path>` | Bank file                                       |
| `/out <path> `      | BIOS file; defaults to bios.bin                 |

```
xbios.exe /combine <bank1_file> <bank2_file> <bank3_file> <bank4_file>
```


## X86 encode command
Encode x86 *machine code* as xcode *byte code* that writes to RAM.

| Switch        | Desc                                       |
| ------------- | ------------------------------------------ |
| `/in <path>`  | Input file                                 |
| `/out <path>` | Output file; defaults to xcodes.bin        |

  - Start address of mem-write is 0. Each write is increment by 4 bytes.
  - Code size increases by a factor of x2.25.

```
xbios.exe /x86-encode <code_file> /out <output_xcodes>
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

## Xcode sim command
Simulate mem-write xcodes and disassemble x86 machine code. (visor sim)

| Switch           | Desc                                                | Default |
| ---------------- | --------------------------------------------------- | ------- |
| `/in <path> `    | Input file (req)                                    |
| `/base <addr>`   | Base address of xcodes                              | `0x80`  |
| `/offset <addr>` | Address of start offset                             | `0x00`  |
| `/simsize <size>`| Size of the sim space in bytes                      | `0x20`  |
| `/d`             | Write to a file; Use `-out` to specify output file  | `false` |

If simulating a file other than a BIOS or extracted init table, 
The `base` of the xcodes might be different. Use `-base <addr>` to specify.

`-offset` is used to change what mem-writes are recorded starting from some 
`offset` extending to `offset + simsize`. Only mem-writes in this range are recorded.

```
xbios.exe /xcode-sim <bios_file> <extra_flags>
```

<details><summary>Example 1 output</summary>

```
Xcodes:
        1a9f: xc_mem_write 0x00, 0x7FC900BE
        1aa8: xc_mem_write 0x04, 0x0000BFFF
        1ab1: xc_mem_write 0x08, 0x00B90009
        1aba: xc_mem_write 0x0c, 0xFC000018
        1ac3: xc_mem_write 0x10, 0x1D8BA5F3
        1acc: xc_mem_write 0x14, 0x00090000
        1ad5: xc_mem_write 0x18, 0x0000E3FF
        1ade: xc_mem_write 0x1c, 0x00000000

Assembly:
        mov esi, 0xff7fc900
        mov edi, 0x90000
        mov ecx, 0x1800
        cld
        rep movsd
        mov ebx, [0x90000]
        jmp ebx

Mem dump: ( 26 bytes )
        0000: BE 00 C9 7F FF BF 00 00
        0008: 09 00 B9 00 18 00 00 FC
        0010: F3 A5 8B 1D 00 00 09 00
        0018: FF E3
```

</details>

<details><summary>Example 2 output</summary>

```
Xcodes:
    0869: xc_mem_write 0x00, 0xFC1000EA
    0872: xc_mem_write 0x04, 0x000008FF

Assembly:
    jmp 0xfffc1000:0x08

Mem dump: ( 7 bytes )
    0000: EA 00 10 FC FF 08 00

```

</details>


## Xcode decode command
Decode Xcodes from a BIOS file, extracted init table.

| Switch         | Desc                                                | Default   |
| -------------- | --------------------------------------------------- | -------   |
| `/in <path> `  | Input file (req)                                    |           |
| `/base <addr>` | Base address for xcodes                             | `0x80`    |
| `/ini <path>`  | Decode settings file                                | `default` |
| `/branch`      | Take unbranchable jumps                             | `false`   |
| `/d`           | Write to a file; Use `-out` to specify output file. | `false`   |

If decoding a file other than a BIOS or extracted init table, 
The `base` of the xcodes might be different. Use `/base <addr>` to specify.

See: [Xcode Decode Settings](./DecodeSettings.md) for more infomation.

```
xbios.exe /xcode-decode <bios_file> <extra_flags>
```

## Compress file command
Compress a file using lzx

| Switch         | Desc                |
| -------------- | ------------------- |
| `/in <path> `  | Input file (req)    |
| `/out <path>`  | Output file (req)   |

```
xbios.exe /compress <in_file> /out <out_file>
```

## Decompress file command
Decompress a file using lzx

| Switch         | Desc                |
| -------------- | ------------------- |
| `/in <path> `  | Input file (req)    |
| `/out <path>`  | Output file (req)   |

```
xbios.exe /decompress <in_file> /out <out_file>
```

## Example Commands

Extract BIOS
```
xbios.exe /extr /mcpx <mcpx_file> <bios_file>
```

Extract BIOS + Keys
```
xbios.exe /extr /keys /mcpx <mcpx_file> <bios_file>
```
Extracts: 
 - 2BL
 - FBL (if applicable)
 - Init table (magic numbers, xcodes)
 - Compressed Kernel (.bin)
 - Decompressed Kernel (.img)

List BIOS infomation
```
xbios.exe /ls /mcpx <mcpx_file> <bios_file>
```

List BIOS Keys
```
xbios.exe /ls /keys /mcpx <mcpx_file> <bios_file>
```

## Credits / Resources

 - https://github.com/XboxDev/xbedump - XboxDev sha1.c implementation
 - https://github.com/WulfyStylez/XBOverclock - WulfyStylez GPU clock calculations
 - https://xboxdevwiki.net/Boot_Process - Boot process



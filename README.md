# Commands
| Command        | Description                                                                       |
| -------------- | --------------------------------------------------------------------------------- |
| `-?`           | Displays help message                                                             |
| `-ls`          | Dump BIOS infomation. Eg: boot params, sizes, signatures, keys, tables, etc.      |
| `-split`       | Split a BIOS into banks.                                                          |
| `-combine`     | Combine multiple BIOSes into a single BIOS.                                       |
| `-extr`        | Extract the 2BL, compressed kerrnel, kernel data, init table from a BIOS          |
| `-bld`         | Build a BIOS from a 2BL, compressed kernel, uncompressed data section, init table |
| `-decomp-krnl` | Decompress and extract the kernel from a BIOS.                                    |
| `-xcode-sim`   | Simulate mem-write xcodes and parse x86 machine code. (*visor sim*)               |
| `-xcode-decode`| Decode Xcodes from a BIOS file, extracted init table.                             |
| `-x86-encode`  | Encode x86 machine code as Xcode mem-writes. (*visor*)                            |
| `-dump-img`    | Dump PE image header info. Eg: A decompressed kernel, xbox exe. etc               |


# Switches
#### General switches
| Switch            | Description                                                  |
| ----------------- | ------------------------------------------------------------ |
| `-?`              | Get help about a specific command.                           |
| `-enc-bldr`       | Assume the 2bl is unencrypted. Decryption will be skipped.   |
| `-enc-krnl`       | Assume the kernel is unencrypted. Decryption will be skipped.|
| `-key-bldr <path>`| The path to the 16-byte 2bl RC4 file                         |
| `-key-krnl <path>`| The path to the 16-byte kernel RC4 file                      |
| `-mcpx <path>`    | The path to the MCPX ROM. Used for en/decrypting the 2BL.    |
| `-romsize`        | How much space is available for the BIOS, (256, 512, 1024)   |


# Notes / Comments
Supports all Original Xbox BIOSes.

There is *no guarantee* that all commands will work with modified BIOSes. However, The Init table and Xcodes are at a hardcoded offset in every BIOS. So, commands such as, `-xcode-decode` and `-xcode-sim` should work with all BIOSes. The `-xcode-sim` command is specifically designed for modified BIOSes.

The preldr is currently mangled during the process of 2BL decryption.

# Encryption / Decryption  
An Original Xbox BIOS contains RC4 keys in the 2BL.

Provide a key file or a MCPX ROM to decrypt the 2BL.
- Use  `-key-bldr <path>` to specify the 2BL key.
- Use  `-mcpx <path>` to specify the MCPX ROM.

The kernel key can then be located in the decrypted 2BL and used to decrypt the compressed kernel.
- Use `-key-krnl <path>` to specify a kernel key from file.
- Use `-enc-krnl` if you dont want the kernel decrypted.

If the kernel has been decrypted, you will see a message like `Decrypting Kernel..`

## What MCPX ROM do i use?
It depends on the version. The decryption process differs between BIOS versions.
- Use MCPX v1.0 for < 4817 kernel
- Use MCPX v1.1 for >= 4817 kernel


# List BIOS command
The list command has some flags to dump out certain infomation in a BIOS.

| flag          | Description                                   |
| ------------- | --------------------------------------------- |
| `-nv2a`       | Dump init table magic values.                 |
| `-datatbl`    | Dump ROM drive / slew calibration table data. |
| `-dump-krnl`  | Dump kernel header info.                      |
| `-keys`       | Dump 2BL rc4 keys.                            |


# Extract BIOS command
The extract command extracts the kernel **compressed**. Thus it is possible to extract it either *encrypted* or *decrypted*. Make a note of what state it's in.

| flag          | Description                                   |
| ------------- | --------------------------------------------- |
| `-keys`       | Extract 2BL rc4 keys to a file.               |


# Build BIOS command
Build a BIOS from a 2BL, compressed kernel, uncompressed data section, init table.

| switches            | Description                                               |
| ----------          | --------------------------------------------------------- |
| `-bldr <path>`      | The boot loader file (req)                                |
| `-inittbl <path>`   | The init table file (req)                                 |
| `-krnl <path>`      | The compressed kernel file (req)                          |
| `-krnldata  <path>` | The uncompressed data section file (req)                  |
| `-preldr <path>`    | The pre boot loader file                                  |
| `-patchkeys`        | Use key switches to patch keys.                           |
| `-eepromkey <path>` | eeprom key to patch into the 2BL                          |
| `-certkey <path>`   | cert key to patch into the 2BL                            |
| `-binsize <size>`   | The file size. The BIOS is replicated upto this value.    |
| `-bfm`              | Build a boot from media BIOS;                             |
| `-enc-krnl`         | Locate the kernel key in 2BL and use it for encryption.   |
---
`-enc-krnl` works backwards with this command. Explictly provide the flag *if you want the kernel encrypted* with the kernel key located in the 2BL.

This command currently only supports *compressed kernel files* because an *LZ compression* algorithm has not been implemented.

You will need to supply a *compressed kernel file* and an *uncompressed data section file*. The `-extr` command can obtain these files.


# Decompress kernel command
Decompress and extract the kernel from a BIOS.

| switches            | Description                 |
| ----------          | --------------------------- |
| `-patchkeys`        | Patch keys flag.            |
| `-pubkey <path>`    | The path to the public key. |

### Extract public key
If `-pubkey <path>` is provided, extract the *RSA1 PUBLIC KEY* from the decompressed kernel.

### Patch public key
If `-patchkeys` and `-pubkey <path>` is provided, overwrite the *RSA1 PUBLIC KEY* using the file.


# X86 encode command
Encode x86 *machine code* as Xcodes that write to RAM.

| switches            | Description                                                   |
| ----------          | ------------------------------------------------------------- |
| `-in <path>`        | Path to x86 byte code. eg. custom_bldr_x86.bin                |
| `-out <path>`       | Specify output file. Defaults to xcodes.bin                   |
---
  - Start address of mem-write is 0. Each write is increment by 4 bytes.
  - Code size increases by a factor of x2.25.
```
Assembly:
 mov eax, 0xfff00bed
 jmp eax
 nop

Machine code:   --->    Xcodes:
  0000: B8 ED 0B F0       0000: xc_mem_write 0x00, 0xf00bedb8
  0004: FF FF E0 90       0009: xc_mem_write 0x04, 0x90e0ffff
```


# Xcode (visor) sim command
Simulate mem-write xcodes and parse x86 machine code. (visor sim)
| switches            | Description                                                   | Default |
| ----------          | ------------------------------------------------------------- | ------- |
| `-simsize`          | Size of the sim space in bytes.                               | `32`    | 
| `-xcodebase <addr>` | Base address of xcodes.                                       | `128`   |
| `-nomaxsize`        | Do not limit the size of the xcode file.                      |         |
| `-d`                | Write to a file. Use `-out` to specify output file.           |         |

<details><summary>Example output:</summary>

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

---

# Xcode decode command
Decode Xcodes from a BIOS file, extracted init table.

| switches            | Description                                                 | Default |
| ----------          | ----------------------------------------------------------- | ------- |
| `-ini <path>`       | Decode settings file.                                       |         |
| `-xcodebase <addr>` | Base address for xcodes.                                    | `128`   |
| `-nomaxsize`        | Do not limit the size of the xcode file.                    |         |
| `-d`                | Write xcodes to a file. Use `-out` to specify output file.  |         |
---
If decoding a file other than a BIOS or extracted init table, This might be different. Use `-xcodebase <addr>` to specify.
 ## Decode settings
 Use decode settings to specify the decode format.

--- 
| `format_str=` |                     |
| ------------  | ------------------- |
| `{offset}`    | the offset          |
| `{op}`	| the opcode string   |
| `{addr}`	| the address string  |
| `{data}`	| the data string     |
| `{comment}`   | the xcode comment   |
 - EG: `format_str={offset}: {op} {addr} {data} {comment}`
 - EG: `format_str=xcode<{op}, {addr}, {data}> {comment}`
--- 
| `jmp_str=`  |               |
| ---------   | ------------- |
| `{label}`   | the label     |
 - EG: `jmp_str={label}`
 - EG: `jmp_str={label}-$-4`
--- 
| `num_str=`|         |
| --------- | ------- |
| `{hex}`   | hex     |
| `{hex8}`  | hex8    |
| `{HEX}`   | HEX     |
| `{HEX8}`  | HEX8    |
 - EG: `num_str=0x{HEX}`  -> 0xA
 - EG: `num_str=0x{hex8}` -> 0x0000000a
--- 
| `label_on_new_line=` |      |
| --------| ----------------- |
| `true`  | label on new line |
| `false` | label in front    |
--- 
| `pad=`  |                   |
| --------| ----------------- |
| `true`  | padding           |
| `false` | no padding        |
---
<details><summary>Settings example:</summary>

```ini
; decode.ini:
; This file is used to configure the xcode decoder.
; ------------------------------------------------------------------------------

; format xcode string
format_str=xcode<{op}, {addr}, {data}> {comment}

; jump xcodes string
jmp_str={label}-$-4

; no operand string.
no_operand_str=0h

; number string, the number format.
num_str=0{hex}h

; label on new line
label_on_new_line=true

; pad
pad=true

; OPCODES
xc_nop=nop
xc_nop_80=nop
xc_nop_f5=nop
xc_mem_write=mem_write
xc_mem_read=mem_read
xc_io_write=out_byte
xc_io_read=in_byte
xc_pci_write=pci_cfg_write
xc_pci_read=pci_cfg_read
xc_jne=cmp_jne
xc_jmp=xjmp
xc_accum=accum
xc_and_or=and_or
xc_use_result=use_result
xc_exit=quit
```
</details>

---

<details><summary>Example output:</summary>

```ini
inittbl file: cromwell.bin
xcode base: 0x80
settings file: decode.ini
xcodes: 228 ( 2052 bytes )
        xcode<pci_cfg_write, 080000884h, 000008001h> ; set io bar (C03) MCPX v1.1
        xcode<pci_cfg_write, 080000810h, 000008001h> ; set io bar (B02) MCPX v1.0
        xcode<pci_cfg_write, 080000804h, 000000003h> ; enable io space
       
        ....

        xcode<pci_cfg_write, 08000f024h, 0f7f0f000h>
        xcode<pci_cfg_write, 080010010h, 00f000000h> ; set nv reg base
        
        ....

        xcode<pci_cfg_write, 080000918h, 00000c201h>
        xcode<out_byte, 00000c200h, 000000070h>
        xcode<out_byte, 00000c004h, 00000008ah> ; 871 encoder slave addr
        xcode<out_byte, 00000c008h, 0000000bah> ; smbus set cmd
        xcode<out_byte, 00000c006h, 00000003fh> ; smbus set val
        xcode<out_byte, 00000c002h, 00000000ah> ; smbus kickoff
lb_05:
        xcode<in_byte, 00000c000h, 0h> ; smbus read status
        xcode<cmp_jne, 000000010h, lb_03-$-4>
        xcode<xjmp, 0h, lb_04-$-4>
lb_03:
        xcode<and_or, 000000008h, 000000000h>
        xcode<cmp_jne, 000000000h, lb_05-$-4>
        xcode<xjmp, 0h, lb_06-$-4>
lb_04:
        xcode<out_byte, 00000c000h, 000000010h> ; smbus clear status
        xcode<out_byte, 00000c008h, 00000006ch> ; smbus set cmd
        xcode<out_byte, 00000c006h, 000000046h> ; smbus set val
        xcode<out_byte, 00000c002h, 00000000ah> ; smbus kickoff
        
        ....

        xcode<xjmp, 0h, lb_26-$-4> ; 15ns delay by performing jmps
lb_26:
        xcode<xjmp, 0h, lb_27-$-4>
lb_27:
        xcode<mem_write, 00f100200h, 003070103h> ;  set extbank bit (00000F00)
        xcode<mem_write, 00f100204h, 011448000h>
        xcode<pci_cfg_write, 08000103ch, 000000000h> ; clear scratch pad (mem type)
        xcode<out_byte, 00000c000h, 000000010h> ; smbus clear status
        xcode<out_byte, 00000c004h, 000000020h> ; smc slave write addr
        xcode<out_byte, 00000c008h, 000000013h> ; smbus set cmd
        xcode<out_byte, 00000c006h, 00000000fh> ; smbus set val
        xcode<out_byte, 00000c002h, 00000000ah> ; smbus kickoff

        ....

        xcode<pci_cfg_write, 08000f020h, 0fdf0fd00h> ; reload nv reg base
        xcode<pci_cfg_write, 080010010h, 0fd000000h>
        xcode<mem_write, 000000000h, 0fc1000b8h> ; visor
        xcode<mem_write, 000000004h, 090e0ffffh>
        xcode<quit, 000000806h, 0h> ; quit xcodes
```

</details>

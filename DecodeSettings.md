### Table of Contents

- [Decode Command](./README.md#xcode-decode-command)
- [Example Settings (decode_settings.ini)](./decode_settings.ini)
- [Example Output](#example-output)

### Xcode Decode settings

Specify the xcode format

| `format_str=` |  Desc               |
| ------------  | ------------------- |
| `{offset}`    | The offset          |
| `{op}`	    | The opcode string   |
| `{addr}`	    | The address string  |
| `{data}`	    | The data string     |
| `{comment}`   | The xcode comment   |

```
format_str={offset}: {op} {addr} {data} {comment}
``` 

`> 0080: xc_mem_write, 0x0, 0xF4 ; <comment>`

```
format_str=xcode<{op}, {addr}, {data}> {comment}
```

`> xcode<xc_mem_write, 0x0, 0xF4> ; <comment>`

--- 

Specify the address format in a jump instruction.

| `jmp_str=<expression>`  |  Desc         |
| ---------               | ------------- |
| `{label}`               | The label     |

```
jmp_str={label}
```

`xcode<xc_jmp, label_1, 0x0>`

```
jmp_str={label}-$-4
```

`xcode<xc_jmp, labe1-$-4, 0x0>`

--- 

Specify the number format.

| `num_str=`|  Desc   |
| --------- | ------- |
| `{hex}`   | hex     |
| `{hex8}`  | hex8    |
| `{HEX}`   | HEX     |
| `{HEX8}`  | HEX8    |

`xcode<xc_mem_write, 0x0, 0xf4>`

`xcode<xc_mem_write, 0x00000000, 0x000000f4>`

`xcode<xc_mem_write, 0x0, 0xF4>`

`xcode<xc_mem_write, 0x00000000, 0x000000F4>`

--- 

Specify if a label is on the same line as the xcode.

| `label_on_new_line=` | Desc |
| --------| ----------------- |
| `true`  | Label on new line |
| `false` | Label in front    |

```
label1: xcode<xc_mem_write, 0x0, 0xF4>
```

```
label1:    
        xcode<xc_mem_write, 0x0, 0xF4>
```

--- 

Specify if the xcode ouput is aligned/padded.

| `pad=`  | Desc                |
| --------| ------------------- |
| `true`  | Align decode ouput. |
| `false` | Dont align.         |

```
    xcode<xc_mem_write, 0x0, 0xF4>
    xcode<xc_mem_read, 0x0, 0x0>
    xcode<xc_jmp, lb1, 0x0>
```

```
    xcode<xc_mem_write, 0x0, 0xF4>
    xcode<xc_mem_read,  0x0, 0x0>
    xcode<xc_jmp,       lb1, 0x0>
```

---

See: [decode_settings.ini](./decode_settings.ini)

### Example Output
<details><summary>Example output:</summary>

```ini
xcode count: 228
xcode size: 2052 bytes
xcode base: 0x80
       0080: xc_pci_write 80000884 00008001 ; set io bar (C03) MCPX v1.1
       0089: xc_pci_write 80000810 00008001 ; set io bar (B02) MCPX v1.0
       0092: xc_pci_write 80000804 00000003 ; enable io space
       009b: xc_io_write  00008049 00000008 ; disable the tco timer
       00a4: xc_io_write  000080d9 00000000 ; KBDRSTIN# in gpio mode
       00ad: xc_io_write  00008026 00000001 ; disable PWRBTN#
       00b6: xc_pci_write 8000f04c 00000001 ; enable internal graphics
       00bf: xc_pci_write 8000f018 00010100 ; setup secondary bus 1
       00c8: xc_pci_write 80000084 07ffffff ; set memory size 128 Mb

       00d1: xc_pci_write 8000f020 0ff00f00
       00da: xc_pci_write 8000f024 f7f0f000
       00e3: xc_pci_write 80010010 0f000000 ; set nv reg base
       00ec: xc_pci_write 80010014 f0000000
       00f5: xc_pci_write 80010004 00000007
       00fe: xc_pci_write 8000f004 00000007
       0107: xc_mem_write 0f0010b0 07633461 ; ctrim_A2
       0110: xc_mem_write 0f0010cc 66660000
       0119: xc_mem_read  0f101000
       0122: xc_and_or    000c0000 00000000
       012b: xc_jne       00000000 lb_00:
       0134: xc_mem_read  0f101000
       013d: xc_and_or    e1f3ffff 80000000
       0146: xc_result    00000003 0f101000
       014f: xc_mem_write 0f0010b8 eeee0000 ; set ctrim2 ( micron )
       0158: xc_jmp       lb_01:
lb_00: 0161: xc_jne       000c0000 lb_02:
       016a: xc_mem_read  0f101000
       0173: xc_and_or    e1f3ffff 860c0000
       017c: xc_result    00000003 0f101000
       0185: xc_mem_write 0f0010b8 ffff0000 ; set ctrim2 ( samsung )
       018e: xc_jmp       lb_01:
lb_02: 0197: xc_mem_read  0f101000
       01a0: xc_and_or    e1f3ffff 820c0000
       01a9: xc_result    00000003 0f101000
       01b2: xc_mem_write 0f0010b8 11110000
lb_01: 01bb: xc_mem_write 0f0010d4 00000009 ; ctrim continue
       01c4: xc_mem_write 0f0010b4 00000000 ; ctrim common
       01cd: xc_mem_write 0f0010bc 00005866
       01d6: xc_mem_write 0f0010c4 0351c858
       01df: xc_mem_write 0f0010c8 30007d67
       01e8: xc_mem_write 0f0010d8 00000000
       01f1: xc_mem_write 0f0010dc a0423635
       01fa: xc_mem_write 0f0010e8 0c6558c6
       0203: xc_mem_write 0f100200 03070103 ; set extbank bit (00000F00)
       020c: xc_mem_write 0f100410 11000016
       0215: xc_mem_write 0f100330 84848888
       021e: xc_mem_write 0f10032c ffffcfff
       0227: xc_mem_write 0f100328 00000001
       0230: xc_mem_write 0f100338 000000df
       0239: xc_pci_write 80000904 00000001
       0242: xc_pci_write 80000914 0000c001
       024b: xc_pci_write 80000918 0000c201
       0254: xc_io_write  0000c200 00000070
       025d: xc_io_write  0000c004 0000008a ; CX871 slave address
       0266: xc_io_write  0000c008 000000ba ; CX871 0xBA = 0x3F
       026f: xc_io_write  0000c006 0000003f ; smbus set val
       0278: xc_io_write  0000c002 0000000a ; smbus kickoff
lb_05: 0281: xc_io_read   0000c000          ; smbus read status
       028a: xc_jne       00000010 lb_03:   ; spin until smbus is ready
       0293: xc_jmp       lb_04:
lb_03: 029c: xc_and_or    00000008 00000000
       02a5: xc_jne       00000000 lb_05:
       02ae: xc_jmp       lb_06:
lb_04: 02b7: xc_io_write  0000c000 00000010 ; smbus clear status

       02c0: xc_io_write  0000c008 0000006c ; CX871 0x6C = 0x46
       02c9: xc_io_write  0000c006 00000046 ; smbus set val
       02d2: xc_io_write  0000c002 0000000a ; smbus kickoff
lb_07: 02db: xc_io_read   0000c000          ; smbus read status
       02e4: xc_jne       00000010 lb_07:   ; spin until smbus is ready
       02ed: xc_io_write  0000c000 00000010 ; smbus clear status

       02f6: xc_io_write  0000c008 000000b8 ; CX871 0xB8 = 0x00
       02ff: xc_io_write  0000c006 00000000 ; smbus set val
       0308: xc_io_write  0000c002 0000000a ; smbus kickoff
lb_08: 0311: xc_io_read   0000c000          ; smbus read status
       031a: xc_jne       00000010 lb_08:   ; spin until smbus is ready
       0323: xc_io_write  0000c000 00000010 ; smbus clear status

       032c: xc_io_write  0000c008 000000ce ; CX871 0xCE = 0x19
       0335: xc_io_write  0000c006 00000019 ; smbus set val
       033e: xc_io_write  0000c002 0000000a ; smbus kickoff
lb_09: 0347: xc_io_read   0000c000          ; smbus read status
       0350: xc_jne       00000010 lb_09:   ; spin until smbus is ready
       0359: xc_io_write  0000c000 00000010 ; smbus clear status

       0362: xc_io_write  0000c008 000000c6 ; CX871 0xC6 = 0x9C
       036b: xc_io_write  0000c006 0000009c ; smbus set val
       0374: xc_io_write  0000c002 0000000a ; smbus kickoff
lb_10: 037d: xc_io_read   0000c000          ; smbus read status
       0386: xc_jne       00000010 lb_10:   ; spin until smbus is ready
       038f: xc_io_write  0000c000 00000010 ; smbus clear status

       0398: xc_io_write  0000c008 00000032 ; CX871 0x32 = 0x08
       03a1: xc_io_write  0000c006 00000008 ; smbus set val
       03aa: xc_io_write  0000c002 0000000a ; smbus kickoff
lb_11: 03b3: xc_io_read   0000c000          ; smbus read status
       03bc: xc_jne       00000010 lb_11:   ; spin until smbus is ready
       03c5: xc_io_write  0000c000 00000010 ; smbus clear status

       03ce: xc_io_write  0000c008 000000c4 ; CX871 0xC4 = 0x01
       03d7: xc_io_write  0000c006 00000001 ; smbus set val
       03e0: xc_io_write  0000c002 0000000a ; smbus kickoff
lb_12: 03e9: xc_io_read   0000c000          ; smbus read status
       03f2: xc_jne       00000010 lb_12:   ; spin until smbus is ready
       03fb: xc_io_write  0000c000 00000010 ; smbus clear status

       0404: xc_jmp       lb_13:
lb_06: 040d: xc_io_write  0000c000 000000ff
       0416: xc_io_write  0000c000 00000010 ; smbus clear status

       041f: xc_io_write  0000c004 000000d4 ; focus slave address
       0428: xc_io_write  0000c008 0000000c ; smbus set cmd
       0431: xc_io_write  0000c006 00000000 ; smbus set val
       043a: xc_io_write  0000c002 0000000a ; smbus kickoff
lb_16: 0443: xc_io_read   0000c000          ; smbus read status
       044c: xc_jne       00000010 lb_14:   ; spin until smbus is ready
       0455: xc_jmp       lb_15:
lb_14: 045e: xc_and_or    00000008 00000000
       0467: xc_jne       00000000 lb_16:
       0470: xc_jmp       lb_17:
lb_15: 0479: xc_io_write  0000c000 00000010 ; smbus clear status

       0482: xc_io_write  0000c008 0000000d ; smbus set cmd
       048b: xc_io_write  0000c006 00000020 ; smbus set val
       0494: xc_io_write  0000c002 0000000a ; smbus kickoff
lb_18: 049d: xc_io_read   0000c000          ; smbus read status
       04a6: xc_jne       00000010 lb_18:   ; spin until smbus is ready
       04af: xc_io_write  0000c000 00000010 ; smbus clear status

       04b8: xc_jmp       lb_13:
lb_17: 04c1: xc_io_write  0000c000 000000ff
       04ca: xc_io_write  0000c000 00000010 ; smbus clear status

       04d3: xc_io_write  0000c004 000000e0
       04dc: xc_io_write  0000c008 00000000 ; smbus set cmd
       04e5: xc_io_write  0000c006 00000000 ; smbus set val
       04ee: xc_io_write  0000c002 0000000a ; smbus kickoff
lb_19: 04f7: xc_io_read   0000c000          ; smbus read status
       0500: xc_jne       00000010 lb_19:   ; spin until smbus is ready
       0509: xc_io_write  0000c000 00000010 ; smbus clear status

       0512: xc_io_write  0000c008 000000b8 ; CX871 0xB8 = 0x00
       051b: xc_io_write  0000c006 00000000 ; smbus set val
       0524: xc_io_write  0000c002 0000000a ; smbus kickoff
lb_20: 052d: xc_io_read   0000c000          ; smbus read status
       0536: xc_jne       00000010 lb_20:   ; spin until smbus is ready
       053f: xc_io_write  0000c000 00000010 ; smbus clear status

lb_13: 0548: xc_io_write  0000c004 00000020 ; smc slave write address
       0551: xc_io_write  0000c008 00000001 ; smbus read revision register
       055a: xc_io_write  0000c006 00000000 ; smbus set val
       0563: xc_io_write  0000c002 0000000a ; smbus kickoff
lb_21: 056c: xc_io_read   0000c000          ; smbus read status
       0575: xc_jne       00000010 lb_21:   ; spin until smbus is ready
       057e: xc_io_write  0000c000 00000010 ; smbus clear status

       0587: xc_io_write  0000c004 00000021 ; smc slave read address
       0590: xc_io_write  0000c008 00000001 ; smbus read revision register
       0599: xc_io_write  0000c002 0000000a ; smbus kickoff
lb_22: 05a2: xc_io_read   0000c000          ; smbus read status
       05ab: xc_jne       00000010 lb_22:   ; spin until smbus is ready
       05b4: xc_io_write  0000c000 00000010 ; smbus clear status

       05bd: xc_io_read   0000c006
       05c6: xc_mem_write 0f680500 00011c01 ; set nv clk 233MHz (@ 16.667MHz)
       05cf: xc_mem_write 0f68050c 000a0400 ; pll_select
       05d8: xc_mem_write 0f001220 00000000
       05e1: xc_mem_write 0f001228 00000000
       05ea: xc_mem_write 0f001264 00000000
       05f3: xc_mem_write 0f001210 00000010
       05fc: xc_mem_read  0f101000
       0605: xc_and_or    06000000 00000000
       060e: xc_jne       00000000 lb_23:
       0617: xc_mem_write 0f001214 48480848
       0620: xc_mem_write 0f00122c 88888888
       0629: xc_jmp       lb_24:
lb_23: 0632: xc_jne       06000000 lb_25:
       063b: xc_mem_write 0f001214 09090909 ; configure pad for samsung
       0644: xc_mem_write 0f00122c aaaaaaaa
       064d: xc_jmp       lb_24:
lb_25: 0656: xc_mem_write 0f001214 09090909 ; configure pad for samsung
       065f: xc_mem_write 0f00122c aaaaaaaa
lb_24: 0668: xc_mem_write 0f001230 ffffffff ; memory pad configuration
       0671: xc_mem_write 0f001234 aaaaaaaa
       067a: xc_mem_write 0f001238 aaaaaaaa
       0683: xc_mem_write 0f00123c 8b8b8b8b
       068c: xc_mem_write 0f001240 ffffffff
       0695: xc_mem_write 0f001244 8b8b8b8b
       069e: xc_mem_write 0f001248 8b8b8b8b
       06a7: xc_mem_write 0f1002d4 00000001
       06b0: xc_mem_write 0f1002c4 00100042
       06b9: xc_mem_write 0f1002cc 00100042
       06c2: xc_mem_write 0f1002c0 00000011
       06cb: xc_mem_write 0f1002c8 00000011
       06d4: xc_mem_write 0f1002c0 00000032
       06dd: xc_mem_write 0f1002c8 00000032
       06e6: xc_mem_write 0f1002c0 00000132
       06ef: xc_mem_write 0f1002c8 00000132
       06f8: xc_mem_write 0f1002d0 00000001
       0701: xc_mem_write 0f1002d0 00000001
       070a: xc_mem_write 0f100210 80000000
       0713: xc_mem_write 0f00124c aa8baa8b
       071c: xc_mem_write 0f001250 0000aa8b
       0725: xc_mem_write 0f100228 081205ff
       072e: xc_mem_write 0f001218 00010000
       0737: xc_pci_read  80000860
       0740: xc_and_or    ffffffff 00000400
       0749: xc_result    00000004 80000860 ; don't gen INIT# on powercycle
       0752: xc_pci_write 8000084c 0000fdde
       075b: xc_pci_write 8000089c 871cc707
       0764: xc_pci_read  800008b4
       076d: xc_and_or    fffff0ff 00000f00
       0776: xc_result    00000004 800008b4
       077f: xc_pci_write 80000340 f0f0c0c0
       0788: xc_pci_write 80000344 00c00000
       0791: xc_pci_write 8000035c 04070000
       079a: xc_pci_write 8000036c 00230801
       07a3: xc_pci_write 8000036c 01230801
; took unbranchable jmp!!
       07ac: xc_jmp       lb_26:            ; 15ns delay by performing jmps
; took unbranchable jmp!!
lb_26: 07b5: xc_jmp       lb_27:
lb_27: 07be: xc_mem_write 0f100200 03070103 ; set extbank bit (00000F00)
       07c7: xc_mem_write 0f100204 11448000
       07d0: xc_pci_write 8000103c 00000000 ; clear scratch pad (mem type)
       07d9: xc_io_write  0000c000 00000010 ; smbus clear status

       07e2: xc_io_write  0000c004 00000020 ; report memory type
       07eb: xc_io_write  0000c008 00000013 ; smbus set cmd
       07f4: xc_io_write  0000c006 0000000f ; smbus set val
       07fd: xc_io_write  0000c002 0000000a ; smbus kickoff
lb_28: 0806: xc_io_read   0000c000          ; smbus read status
       080f: xc_jne       00000010 lb_28:   ; spin until smbus is ready
       0818: xc_io_write  0000c000 00000010 ; smbus clear status

       0821: xc_io_write  0000c008 00000012 ; smbus set cmd
       082a: xc_io_write  0000c006 000000f0 ; smbus set val
       0833: xc_io_write  0000c002 0000000a ; smbus kickoff
lb_29: 083c: xc_io_read   0000c000          ; smbus read status
       0845: xc_jne       00000010 lb_29:   ; spin until smbus is ready
       084e: xc_io_write  0000c000 00000010 ; smbus clear status

       0857: xc_pci_write 8000f020 fdf0fd00 ; reload nv reg base
       0860: xc_pci_write 80010010 fd000000
       0869: xc_mem_write 00000000 fc1000b8 ; visor attack prep
       0872: xc_mem_write 00000004 90e0ffff
       087b: xc_exit      00000806          ; quit xcodes
```

</details>

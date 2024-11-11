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

`format_str={offset} {op} {addr} {data} {comment}` 
 - `0080:` `xc_mem_write` `0x0` `0xF4` `; write hlt instr to addr 0x0`

`format_str=xcode<{op}, {addr}, {data}> {comment}` 
- `xcode<` `xc_mem_write` `,` `0x0` `,` `0xF4` `>` `; write hlt instr to addr 0x0`

--- 

Specify the address format in a jump instruction.

| `jmp_str=`  |  Desc         |
| ---------   | ------------- |
| `{label}`   | The label     |

- `xcode<xc_jmp, lb1, 0x0>`
- `xcode<xc_jmp, lb1-$-4, 0x0>`

--- 

Specify the number format.

| `num_str=`|  Desc   |
| --------- | ------- |
| `{hex}`   | hex     |
| `{hex8}`  | hex8    |
| `{HEX}`   | HEX     |
| `{HEX8}`  | HEX8    |

- `xcode<xc_mem_write, 0x0, 0xf4>`
- `xcode<xc_mem_write, 0x00000000, 0x000000f4>`
- `xcode<xc_mem_write, 0x0, 0xF4>`
- `xcode<xc_mem_write, 0x00000000, 0x000000F4>`

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

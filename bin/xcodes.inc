	xcode<xc_nop_80, 000081000h, 000000001h> 
	xcode<xc_nop_f5, 000000114h, 000000228h> 
	xcode<xc_pci_write, 080000884h, 000008001h> ; set io bar (C03) MCPX v1.1
	xcode<xc_pci_write, 080000810h, 000008001h> ; set io bar (B02) MCPX v1.0
	xcode<xc_pci_write, 080000804h, 000000003h> ; enable io space
	xcode<xc_io_write, 000008049h, 000000008h> ; disable the tco timer
	xcode<xc_io_write, 0000080d9h, 000000000h> ; KBDRSTIN# in gpio mode
	xcode<xc_io_write, 000008026h, 000000001h> ; disable PWRBTN#
	xcode<xc_pci_write, 08000f04ch, 000000001h> ; enable internal graphics
	xcode<xc_pci_write, 08000f018h, 000010100h> ; setup secondary bus 1
	xcode<xc_pci_write, 080000084h, 007ffffffh> ; set memory size 128 Mb
	xcode<xc_pci_write, 08000f020h, 00ff00f00h> 
	xcode<xc_pci_write, 08000f024h, 0f7f0f000h> 
	xcode<xc_pci_write, 080010010h, 00f000000h> ; set nv reg base
	xcode<xc_pci_write, 080010014h, 0f0000000h> 
	xcode<xc_pci_write, 080010004h, 000000007h> 
	xcode<xc_pci_write, 08000f004h, 000000007h> 
	xcode<xc_pci_write, 080000904h, 000000001h> 
	xcode<xc_pci_write, 080000914h, 00000c001h> 
	xcode<xc_pci_write, 080000918h, 00000c201h> 
	xcode<xc_io_write, 00000c200h, 000000070h> 
	xcode<xc_io_write, 00000c004h, 0000000e1h> ; xcalibur encoder slave addr
	xcode<xc_io_write, 00000c008h, 000000000h> ; smbus set cmd
	xcode<xc_io_write, 00000c006h, 000000000h> ; smbus set val
	xcode<xc_io_write, 00000c002h, 00000000dh> 
lb_02:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_jne, 000000010h, lb_00-$-4> 
	xcode<xc_jmp, 0h, lb_01-$-4> 
lb_00:
	xcode<xc_and_or, 000000008h, 000000000h> 
	xcode<xc_jne, 000000000h, lb_02-$-4> 
	xcode<xc_io_write, 00000c000h, 0000000ffh> 
	xcode<xc_jmp, 0h, lb_03-$-4> 
lb_01:
	xcode<xc_io_read, 00000c009h, 0h> 
	xcode<xc_io_read, 00000c009h, 0h> 
	xcode<xc_io_read, 00000c009h, 0h> 
	xcode<xc_io_read, 00000c009h, 0h> 
	xcode<xc_io_write, 00000c000h, 0000000ffh> 
	xcode<xc_mem_write, 00f0010b0h, 001000010h> 
	xcode<xc_mem_write, 00f0010cch, 066660000h> 
	xcode<xc_jmp, 0h, lb_04-$-4> 
lb_26:
	xcode<xc_mem_write, 00f0010d4h, 00000000fh> 
	xcode<xc_mem_write, 00f0010b4h, 000000000h> 
	xcode<xc_mem_write, 00f0010bch, 000005860h> 
	xcode<xc_mem_write, 00f0010c4h, 0aaaa0000h> 
	xcode<xc_mem_write, 00f0010c8h, 000007d67h> 
	xcode<xc_mem_write, 00f0010d8h, 000000000h> 
	xcode<xc_mem_write, 00f0010dch, 000000000h> 
	xcode<xc_mem_write, 00f0010e8h, 004000000h> 
	xcode<xc_mem_write, 00f100200h, 003070103h> ;  set extbank bit (00000F00)
	xcode<xc_mem_write, 00f100410h, 011000016h> 
	xcode<xc_mem_write, 00f100330h, 084848888h> 
	xcode<xc_mem_write, 00f10032ch, 0ffffcfffh> 
	xcode<xc_mem_write, 00f100328h, 000000001h> 
	xcode<xc_mem_write, 00f100338h, 0000000dfh> 
	xcode<xc_io_write, 00000c004h, 000000020h> ; smc slave write addr
	xcode<xc_io_write, 00000c008h, 000000001h> ; smbus read revision register
	xcode<xc_io_write, 00000c006h, 000000000h> ; smbus set val
	xcode<xc_io_write, 00000c002h, 00000000ah> ; smbus kickoff
lb_05:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_jne, 000000010h, lb_05-$-4> ; spin until smbus is ready
	xcode<xc_io_write, 00000c000h, 000000010h> ; smbus clear status
	xcode<xc_io_write, 00000c004h, 000000021h> ; smc slave read addr
	xcode<xc_io_write, 00000c008h, 000000001h> ; smbus read revision register
	xcode<xc_io_write, 00000c002h, 00000000ah> ; smbus kickoff
lb_06:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_jne, 000000010h, lb_06-$-4> ; spin until smbus is ready
	xcode<xc_io_write, 00000c000h, 000000010h> ; smbus clear status
	xcode<xc_io_read, 00000c006h, 0h> 
	xcode<xc_jne, 000000050h, lb_07-$-4> 
	xcode<xc_jmp, 0h, lb_08-$-4> 
lb_07:
	xcode<xc_pci_write, 08000036ch, 001000000h> ; smbus is bad, flatline clks
lb_08:
	xcode<xc_mem_write, 00f680500h, 000011c01h> ; set nv clk 233MHz (@ 16.667MHz)
	xcode<xc_mem_write, 00f68050ch, 0000a0400h> 
	xcode<xc_mem_write, 00f001220h, 000000000h> 
	xcode<xc_mem_write, 00f001228h, 000000000h> 
	xcode<xc_mem_write, 00f001264h, 000000000h> 
	xcode<xc_mem_write, 00f001210h, 000000000h> 
	xcode<xc_jmp, 0h, lb_09-$-4> 
lb_04:
	xcode<xc_pci_write, 08000183ch, 0000000c3h> 
lb_09:
	xcode<xc_io_write, 00000c004h, 0000000a9h> 
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_use_result, xc_io_write, 00000c008h> 
	xcode<xc_io_write, 00000c000h, 000000000h> 
	xcode<xc_io_write, 00000c002h, 00000000ah> ; smbus kickoff
lb_11:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_use_result, xc_pci_write, 08000103ch> 
	xcode<xc_and_or, 000000008h, 000000000h> 
	xcode<xc_jne, 000000008h, lb_10-$-4> 
	xcode<xc_jmp, 0h, lb_11-$-4> 
lb_10:
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0000000ffh, 000000000h> 
	xcode<xc_jne, 000000010h, lb_09-$-4> 
	xcode<xc_io_read, 00000c006h, 0h> 
	xcode<xc_and_or, 0000000ffh, 000000000h> 
	xcode<xc_use_result, xc_mem_write, 00f001248h> 
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0000000ffh, 000000000h> 
	xcode<xc_jne, 0000000c3h, lb_12-$-4> 
	xcode<xc_mem_read, 00f001248h, 0h> 
	xcode<xc_jne, 000000042h, lb_13-$-4> 
	xcode<xc_mem_read, 00f101000h, 0h> 
	xcode<xc_and_or, 0e1f3ffffh, 0800c0000h> 
	xcode<xc_jmp, 0h, lb_14-$-4> 
lb_13:
	xcode<xc_jne, 000000040h, lb_15-$-4> 
	xcode<xc_mem_read, 00f101000h, 0h> 
	xcode<xc_and_or, 0e1f3ffffh, 080080000h> 
	xcode<xc_jmp, 0h, lb_14-$-4> 
lb_15:
	xcode<xc_jne, 000000002h, lb_16-$-4> 
	xcode<xc_mem_read, 00f101000h, 0h> 
	xcode<xc_and_or, 0e1f3ffffh, 080040000h> 
	xcode<xc_jmp, 0h, lb_14-$-4> 
lb_16:
	xcode<xc_mem_read, 00f101000h, 0h> 
	xcode<xc_and_or, 0e1f3ffffh, 080000000h> 
lb_14:
	xcode<xc_use_result, xc_mem_write, 00f101000h> 
	xcode<xc_pci_write, 08000183ch, 0000000c2h> 
	xcode<xc_jmp, 0h, lb_09-$-4> 
lb_12:
	xcode<xc_mem_read, 00f001248h, 0h> 
	xcode<xc_and_or, 000000001h, 000000000h> 
	xcode<xc_jne, 000000000h, lb_17-$-4> 
	xcode<xc_mem_read, 00f001248h, 0h> 
lb_29:
	xcode<xc_and_or, 000000002h, 000000000h> 
	xcode<xc_jne, 000000000h, lb_18-$-4> 
	xcode<xc_mem_read, 00f001248h, 0h> 
lb_30:
	xcode<xc_and_or, 000000004h, 000000000h> 
	xcode<xc_jne, 000000000h, lb_19-$-4> 
	xcode<xc_mem_read, 00f001248h, 0h> 
lb_31:
	xcode<xc_and_or, 000000008h, 000000000h> 
	xcode<xc_jne, 000000000h, lb_20-$-4> 
	xcode<xc_mem_read, 00f001248h, 0h> 
lb_32:
	xcode<xc_and_or, 000000010h, 000000000h> 
	xcode<xc_jne, 000000000h, lb_21-$-4> 
	xcode<xc_mem_read, 00f001248h, 0h> 
lb_33:
	xcode<xc_and_or, 000000020h, 000000000h> 
	xcode<xc_jne, 000000000h, lb_22-$-4> 
	xcode<xc_mem_read, 00f001248h, 0h> 
lb_34:
	xcode<xc_and_or, 000000040h, 000000000h> 
	xcode<xc_jne, 000000000h, lb_23-$-4> 
	xcode<xc_mem_read, 00f001248h, 0h> 
lb_35:
	xcode<xc_and_or, 000000080h, 000000000h> 
	xcode<xc_jne, 000000000h, lb_24-$-4> 
lb_36:
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0000000ffh, 000000000h> 
	xcode<xc_jne, 0000000c2h, lb_25-$-4> 
	xcode<xc_mem_read, 00f001248h, 0h> 
	xcode<xc_and_or, 0ffff0000h, 000000000h> 
	xcode<xc_use_result, xc_mem_write, 00f0010b8h> 
	xcode<xc_pci_write, 08000183ch, 0000000c1h> 
	xcode<xc_jmp, 0h, lb_26-$-4> 
lb_25:
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0000000ffh, 000000000h> 
	xcode<xc_jne, 0000000c1h, lb_27-$-4> 
	xcode<xc_mem_read, 00f001248h, 0h> 
	xcode<xc_use_result, xc_mem_write, 00f00122ch> 
	xcode<xc_pci_write, 08000183ch, 0000000c0h> 
	xcode<xc_jmp, 0h, lb_09-$-4> 
lb_27:
	xcode<xc_mem_read, 00f001248h, 0h> 
	xcode<xc_use_result, xc_mem_write, 00f001214h> 
	xcode<xc_pci_write, 08000183ch, 0000000c3h> 
	xcode<xc_jmp, 0h, lb_28-$-4> 
lb_17:
	xcode<xc_mem_read, 00f001248h, 0h> 
	xcode<xc_and_or, 0ffffffffh, 001010100h> 
	xcode<xc_use_result, xc_mem_write, 00f001248h> 
	xcode<xc_jmp, 0h, lb_29-$-4> 
lb_18:
	xcode<xc_mem_read, 00f001248h, 0h> 
	xcode<xc_and_or, 0ffffffffh, 002020200h> 
	xcode<xc_use_result, xc_mem_write, 00f001248h> 
	xcode<xc_jmp, 0h, lb_30-$-4> 
lb_19:
	xcode<xc_mem_read, 00f001248h, 0h> 
	xcode<xc_and_or, 0ffffffffh, 004040400h> 
	xcode<xc_use_result, xc_mem_write, 00f001248h> 
	xcode<xc_jmp, 0h, lb_31-$-4> 
lb_20:
	xcode<xc_mem_read, 00f001248h, 0h> 
	xcode<xc_and_or, 0ffffffffh, 008080800h> 
	xcode<xc_use_result, xc_mem_write, 00f001248h> 
	xcode<xc_jmp, 0h, lb_32-$-4> 
lb_21:
	xcode<xc_mem_read, 00f001248h, 0h> 
	xcode<xc_and_or, 0ffffffffh, 010101000h> 
	xcode<xc_use_result, xc_mem_write, 00f001248h> 
	xcode<xc_jmp, 0h, lb_33-$-4> 
lb_22:
	xcode<xc_mem_read, 00f001248h, 0h> 
	xcode<xc_and_or, 0ffffffffh, 020202000h> 
	xcode<xc_use_result, xc_mem_write, 00f001248h> 
	xcode<xc_jmp, 0h, lb_34-$-4> 
lb_23:
	xcode<xc_mem_read, 00f001248h, 0h> 
	xcode<xc_and_or, 0ffffffffh, 040404000h> 
	xcode<xc_use_result, xc_mem_write, 00f001248h> 
	xcode<xc_jmp, 0h, lb_35-$-4> 
lb_24:
	xcode<xc_mem_read, 00f001248h, 0h> 
	xcode<xc_and_or, 0ffffffffh, 080808000h> 
	xcode<xc_use_result, xc_mem_write, 00f001248h> 
	xcode<xc_jmp, 0h, lb_36-$-4> 
lb_28:
	xcode<xc_mem_write, 00f001248h, 000000000h> 
	xcode<xc_mem_write, 00f001230h, 0ffffffffh> 
	xcode<xc_mem_write, 00f001234h, 0aaaaaaaah> 
	xcode<xc_mem_write, 00f001238h, 0aaaaaaaah> 
	xcode<xc_mem_write, 00f00123ch, 08b8b8b8bh> 
	xcode<xc_mem_write, 00f001240h, 0ffffffffh> 
	xcode<xc_mem_write, 00f001244h, 08b8b8b8bh> 
	xcode<xc_mem_write, 00f001248h, 08b8b8b8bh> 
	xcode<xc_mem_write, 00f1002d4h, 000000001h> 
	xcode<xc_mem_write, 00f1002c4h, 000100000h> 
	xcode<xc_mem_write, 00f1002cch, 000100000h> 
	xcode<xc_mem_write, 00f1002c0h, 000000011h> 
	xcode<xc_mem_write, 00f1002c8h, 000000011h> 
	xcode<xc_mem_write, 00f1002c0h, 000000032h> 
	xcode<xc_mem_write, 00f1002c8h, 000000032h> 
	xcode<xc_mem_write, 00f1002c0h, 000000132h> 
	xcode<xc_mem_write, 00f1002c8h, 000000132h> 
	xcode<xc_mem_write, 00f1002d0h, 000000001h> 
	xcode<xc_mem_write, 00f1002d0h, 000000001h> 
	xcode<xc_mem_write, 00f100210h, 080000000h> 
	xcode<xc_mem_write, 00f00124ch, 0aa8baa8bh> 
	xcode<xc_mem_write, 00f001250h, 000005474h> 
	xcode<xc_mem_write, 00f100228h, 0081205ffh> 
	xcode<xc_mem_write, 00f001218h, 000000000h> 
	xcode<xc_pci_read, 080000860h, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000400h> 
	xcode<xc_use_result, xc_pci_write, 080000860h> ; don't gen INIT# on powercycle
	xcode<xc_pci_write, 08000084ch, 00000fddeh> 
	xcode<xc_pci_write, 08000089ch, 0871cc707h> 
	xcode<xc_pci_read, 0800008b4h, 0h> 
	xcode<xc_and_or, 0fffff0ffh, 000000f00h> 
	xcode<xc_use_result, xc_pci_write, 0800008b4h> 
	xcode<xc_pci_write, 080000340h, 0f0f0c0c0h> 
	xcode<xc_pci_write, 080000344h, 000000000h> 
	xcode<xc_pci_write, 08000035ch, 000000000h> 
	xcode<xc_pci_write, 08000036ch, 000230801h> 
	xcode<xc_pci_write, 08000036ch, 001230801h> 
	xcode<xc_jmp, 0h, lb_37-$-4> ; 15ns delay by performing jmps
lb_37:
	xcode<xc_jmp, 0h, lb_38-$-4> 
lb_38:
	xcode<xc_mem_write, 00f100200h, 003070103h> ;  set extbank bit (00000F00)
	xcode<xc_mem_write, 00f100204h, 011448000h> 
	xcode<xc_pci_write, 08000103ch, 000000000h> ; clear scratch pad (mem type)
	xcode<xc_pci_write, 08000183ch, 000000000h> ; clear scratch pad (mem result)
	xcode<xc_mem_write, 000555548h, 0aaaaaaaah> ; mem test pattern 1
	xcode<xc_mem_write, 002555548h, 05a5a5a5ah> ; mem test pattern 2
	xcode<xc_mem_read, 002555548h, 0h> 
	xcode<xc_jne, 05a5a5a5ah, lb_39-$-4> ; does dram exist?
	xcode<xc_mem_read, 000555548h, 0h> 
	xcode<xc_jne, 0aaaaaaaah, lb_40-$-4> 
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000001h> 
	xcode<xc_use_result, xc_pci_write, 08000103ch> 
lb_40:
	xcode<xc_mem_write, 000555548h, 055555555h> ; mem test pattern 3
	xcode<xc_mem_read, 000555548h, 0h> 
	xcode<xc_jne, 055555555h, lb_39-$-4> 
	xcode<xc_jmp, 0h, lb_41-$-4> 
lb_39:
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000001h> 
	xcode<xc_use_result, xc_pci_write, 08000183ch> 
lb_41:
	xcode<xc_mem_write, 000555558h, 0aaaaaaaah> ; mem test pattern 1
	xcode<xc_mem_write, 002555558h, 05a5a5a5ah> ; mem test pattern 2
	xcode<xc_mem_read, 002555558h, 0h> 
	xcode<xc_jne, 05a5a5a5ah, lb_42-$-4> ; does dram exist?
	xcode<xc_mem_read, 000555558h, 0h> 
	xcode<xc_jne, 0aaaaaaaah, lb_43-$-4> 
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000002h> 
	xcode<xc_use_result, xc_pci_write, 08000103ch> 
lb_43:
	xcode<xc_mem_write, 000555558h, 055555555h> ; mem test pattern 3
	xcode<xc_mem_read, 000555558h, 0h> 
	xcode<xc_jne, 055555555h, lb_42-$-4> 
	xcode<xc_jmp, 0h, lb_44-$-4> 
lb_42:
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000002h> 
	xcode<xc_use_result, xc_pci_write, 08000183ch> 
lb_44:
	xcode<xc_mem_write, 000555568h, 0aaaaaaaah> ; mem test pattern 1
	xcode<xc_mem_write, 002555568h, 05a5a5a5ah> ; mem test pattern 2
	xcode<xc_mem_read, 002555568h, 0h> 
	xcode<xc_jne, 05a5a5a5ah, lb_45-$-4> ; does dram exist?
	xcode<xc_mem_read, 000555568h, 0h> 
	xcode<xc_jne, 0aaaaaaaah, lb_46-$-4> 
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000004h> 
	xcode<xc_use_result, xc_pci_write, 08000103ch> 
lb_46:
	xcode<xc_mem_write, 000555568h, 055555555h> ; mem test pattern 3
	xcode<xc_mem_read, 000555568h, 0h> 
	xcode<xc_jne, 055555555h, lb_45-$-4> 
	xcode<xc_jmp, 0h, lb_47-$-4> 
lb_45:
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000004h> 
	xcode<xc_use_result, xc_pci_write, 08000183ch> 
lb_47:
	xcode<xc_mem_write, 000555578h, 0aaaaaaaah> ; mem test pattern 1
	xcode<xc_mem_write, 002555578h, 05a5a5a5ah> ; mem test pattern 2
	xcode<xc_mem_read, 002555578h, 0h> 
	xcode<xc_jne, 05a5a5a5ah, lb_48-$-4> ; does dram exist?
	xcode<xc_mem_read, 000555578h, 0h> 
	xcode<xc_jne, 0aaaaaaaah, lb_49-$-4> 
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000008h> 
	xcode<xc_use_result, xc_pci_write, 08000103ch> 
lb_49:
	xcode<xc_mem_write, 000555578h, 055555555h> ; mem test pattern 3
	xcode<xc_mem_read, 000555578h, 0h> 
	xcode<xc_jne, 055555555h, lb_48-$-4> 
	xcode<xc_jmp, 0h, lb_50-$-4> 
lb_48:
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000008h> 
	xcode<xc_use_result, xc_pci_write, 08000183ch> 
lb_50:
	xcode<xc_mem_write, 004555548h, 0aaaaaaaah> ; mem test pattern 1
	xcode<xc_mem_write, 006555548h, 05a5a5a5ah> ; mem test pattern 2
	xcode<xc_mem_read, 006555548h, 0h> 
	xcode<xc_jne, 05a5a5a5ah, lb_51-$-4> ; does dram exist?
	xcode<xc_mem_read, 004555548h, 0h> 
	xcode<xc_jne, 0aaaaaaaah, lb_52-$-4> 
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000010h> 
	xcode<xc_use_result, xc_pci_write, 08000103ch> 
lb_52:
	xcode<xc_mem_write, 004555548h, 055555555h> ; mem test pattern 3
	xcode<xc_mem_read, 004555548h, 0h> 
	xcode<xc_jne, 055555555h, lb_51-$-4> 
	xcode<xc_jmp, 0h, lb_53-$-4> 
lb_51:
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000010h> 
	xcode<xc_use_result, xc_pci_write, 08000183ch> 
lb_53:
	xcode<xc_mem_write, 004555558h, 0aaaaaaaah> ; mem test pattern 1
	xcode<xc_mem_write, 006555558h, 05a5a5a5ah> ; mem test pattern 2
	xcode<xc_mem_read, 006555558h, 0h> 
	xcode<xc_jne, 05a5a5a5ah, lb_54-$-4> ; does dram exist?
	xcode<xc_mem_read, 004555558h, 0h> 
	xcode<xc_jne, 0aaaaaaaah, lb_55-$-4> 
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000020h> 
	xcode<xc_use_result, xc_pci_write, 08000103ch> 
lb_55:
	xcode<xc_mem_write, 004555558h, 055555555h> ; mem test pattern 3
	xcode<xc_mem_read, 004555558h, 0h> 
	xcode<xc_jne, 055555555h, lb_54-$-4> 
	xcode<xc_jmp, 0h, lb_56-$-4> 
lb_54:
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000020h> 
	xcode<xc_use_result, xc_pci_write, 08000183ch> 
lb_56:
	xcode<xc_mem_write, 004555568h, 0aaaaaaaah> ; mem test pattern 1
	xcode<xc_mem_write, 006555568h, 05a5a5a5ah> ; mem test pattern 2
	xcode<xc_mem_read, 006555568h, 0h> 
	xcode<xc_jne, 05a5a5a5ah, lb_57-$-4> ; does dram exist?
	xcode<xc_mem_read, 004555568h, 0h> 
	xcode<xc_jne, 0aaaaaaaah, lb_58-$-4> 
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000040h> 
	xcode<xc_use_result, xc_pci_write, 08000103ch> 
lb_58:
	xcode<xc_mem_write, 004555568h, 055555555h> ; mem test pattern 3
	xcode<xc_mem_read, 004555568h, 0h> 
	xcode<xc_jne, 055555555h, lb_57-$-4> 
	xcode<xc_jmp, 0h, lb_59-$-4> 
lb_57:
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000040h> 
	xcode<xc_use_result, xc_pci_write, 08000183ch> 
lb_59:
	xcode<xc_mem_write, 004555578h, 0aaaaaaaah> ; mem test pattern 1
	xcode<xc_mem_write, 006555578h, 05a5a5a5ah> ; mem test pattern 2
	xcode<xc_mem_read, 006555578h, 0h> 
	xcode<xc_jne, 05a5a5a5ah, lb_60-$-4> ; does dram exist?
	xcode<xc_mem_read, 004555578h, 0h> 
	xcode<xc_jne, 0aaaaaaaah, lb_61-$-4> 
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000080h> 
	xcode<xc_use_result, xc_pci_write, 08000103ch> 
lb_61:
	xcode<xc_mem_write, 004555578h, 055555555h> ; mem test pattern 3
	xcode<xc_mem_read, 004555578h, 0h> 
	xcode<xc_jne, 055555555h, lb_60-$-4> 
	xcode<xc_jmp, 0h, lb_62-$-4> 
lb_60:
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000080h> 
	xcode<xc_use_result, xc_pci_write, 08000183ch> 
lb_62:
	xcode<xc_mem_write, 00f100200h, 003070003h> ;  clear extbank bit (00000F00)
	xcode<xc_pci_write, 080000084h, 003ffffffh> ; set memory size 64 Mb
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0000000ffh, 000000000h> 
	xcode<xc_use_result, xc_io_write, 00000c006h> 
	xcode<xc_io_read, 00000c006h, 0h> 
	xcode<xc_jne, 000000000h, lb_63-$-4> 
	xcode<xc_mem_write, 00f100200h, 003070103h> ;  set extbank bit (00000F00)
	xcode<xc_mem_write, 00f100204h, 011338000h> 
	xcode<xc_jmp, 0h, lb_64-$-4> 
lb_63:
	xcode<xc_io_read, 00000c006h, 0h> 
	xcode<xc_jne, 0000000ffh, lb_64-$-4> 
	xcode<xc_mem_write, 00f100200h, 003070103h> ;  set extbank bit (00000F00)
	xcode<xc_pci_write, 080000084h, 007ffffffh> ; set memory size 128 Mb
lb_64:
	xcode<xc_io_write, 00000c004h, 000000020h> ; smc slave write addr
	xcode<xc_io_write, 00000c008h, 000000013h> ; smbus set cmd
	xcode<xc_io_write, 00000c002h, 00000000ah> ; smbus kickoff
lb_65:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_jne, 000000010h, lb_65-$-4> ; spin until smbus is ready
	xcode<xc_io_write, 00000c000h, 000000010h> ; smbus clear status
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0000000ffh, 000000000h> 
	xcode<xc_use_result, xc_io_write, 00000c006h> 
	xcode<xc_io_write, 00000c008h, 000000012h> ; smbus set cmd
	xcode<xc_io_write, 00000c002h, 00000000ah> ; smbus kickoff
lb_66:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_jne, 000000010h, lb_66-$-4> ; spin until smbus is ready
	xcode<xc_io_write, 00000c000h, 000000010h> ; smbus clear status
	xcode<xc_pci_write, 08000f020h, 0fdf0fd00h> ; reload nv reg base
	xcode<xc_pci_write, 080010010h, 0fd000000h> 
	xcode<xc_jmp, 0h, lb_67-$-4> 
lb_03:
	xcode<xc_pci_write, 080000884h, 000008001h> ; set io bar (C03) MCPX v1.1
	xcode<xc_pci_write, 080000810h, 000008001h> ; set io bar (B02) MCPX v1.0
	xcode<xc_pci_write, 080000804h, 000000003h> ; enable io space
	xcode<xc_io_write, 000008049h, 000000008h> ; disable the tco timer
	xcode<xc_io_write, 0000080d9h, 000000000h> ; KBDRSTIN# in gpio mode
	xcode<xc_io_write, 000008026h, 000000001h> ; disable PWRBTN#
	xcode<xc_pci_write, 08000f04ch, 000000001h> ; enable internal graphics
	xcode<xc_pci_write, 08000f018h, 000010100h> ; setup secondary bus 1
	xcode<xc_pci_write, 080000084h, 007ffffffh> ; set memory size 128 Mb
	xcode<xc_pci_write, 08000f020h, 00ff00f00h> 
	xcode<xc_pci_write, 08000f024h, 0f7f0f000h> 
	xcode<xc_pci_write, 080010010h, 00f000000h> ; set nv reg base
	xcode<xc_pci_write, 080010014h, 0f0000000h> 
	xcode<xc_pci_write, 080010004h, 000000007h> 
	xcode<xc_pci_write, 08000f004h, 000000007h> 
	xcode<xc_mem_write, 00f0010b0h, 007633461h> 
	xcode<xc_mem_write, 00f0010cch, 066660000h> 
	xcode<xc_mem_read, 00f101000h, 0h> 
	xcode<xc_and_or, 0000c0000h, 000000000h> 
	xcode<xc_jne, 000000000h, lb_68-$-4> 
	xcode<xc_mem_read, 00f101000h, 0h> 
	xcode<xc_and_or, 0e1f3ffffh, 080000000h> 
	xcode<xc_use_result, xc_mem_write, 00f101000h> 
	xcode<xc_mem_write, 00f0010b8h, 0eeee0000h> 
	xcode<xc_jmp, 0h, lb_69-$-4> 
lb_68:
	xcode<xc_jne, 0000c0000h, lb_06-$-4> 
	xcode<xc_mem_read, 00f101000h, 0h> 
	xcode<xc_and_or, 0e1f3ffffh, 0860c0000h> 
	xcode<xc_use_result, xc_mem_write, 00f101000h> 
	xcode<xc_mem_write, 00f0010b8h, 0ffff0000h> 
	xcode<xc_jmp, 0h, lb_69-$-4> 
	xcode<xc_mem_read, 00f101000h, 0h> 
	xcode<xc_and_or, 0e1f3ffffh, 0820c0000h> 
	xcode<xc_use_result, xc_mem_write, 00f101000h> 
	xcode<xc_mem_write, 00f0010b8h, 011110000h> 
lb_69:
	xcode<xc_mem_write, 00f0010d4h, 000000009h> 
	xcode<xc_mem_write, 00f0010b4h, 000000000h> 
	xcode<xc_mem_write, 00f0010bch, 000005866h> 
	xcode<xc_mem_write, 00f0010c4h, 00351c858h> 
	xcode<xc_mem_write, 00f0010c8h, 030007d67h> 
	xcode<xc_mem_write, 00f0010d8h, 000000000h> 
	xcode<xc_mem_write, 00f0010dch, 0a0423635h> 
	xcode<xc_mem_write, 00f0010e8h, 00c6558c6h> 
	xcode<xc_mem_write, 00f100200h, 003070103h> ;  set extbank bit (00000F00)
	xcode<xc_mem_write, 00f100410h, 011000016h> 
	xcode<xc_mem_write, 00f100330h, 084848888h> 
	xcode<xc_mem_write, 00f10032ch, 0ffffcfffh> 
	xcode<xc_mem_write, 00f100328h, 000000001h> 
	xcode<xc_mem_write, 00f100338h, 0000000dfh> 
	xcode<xc_pci_write, 080000904h, 000000001h> 
	xcode<xc_pci_write, 080000914h, 00000c001h> 
	xcode<xc_pci_write, 080000918h, 00000c201h> 
	xcode<xc_io_write, 00000c200h, 000000070h> 
	xcode<xc_io_write, 00000c004h, 0000000d4h> ; focus encoder slave addr
	xcode<xc_io_write, 00000c008h, 00000000ch> ; smbus set cmd
	xcode<xc_io_write, 00000c006h, 000000000h> ; smbus set val
	xcode<xc_io_write, 00000c002h, 00000000ah> ; smbus kickoff
lb_72:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_jne, 000000010h, lb_70-$-4> 
	xcode<xc_jmp, 0h, lb_71-$-4> 
lb_70:
	xcode<xc_and_or, 000000008h, 000000000h> 
	xcode<xc_jne, 000000000h, lb_72-$-4> 
	xcode<xc_io_write, 00000c000h, 0000000ffh> 
	xcode<xc_jmp, 0h, lb_73-$-4> 
lb_71:
	xcode<xc_io_write, 00000c000h, 000000010h> ; smbus clear status
	xcode<xc_io_write, 00000c008h, 00000000dh> ; smbus set cmd
	xcode<xc_io_write, 00000c006h, 000000020h> ; smbus set val
	xcode<xc_io_write, 00000c002h, 00000000ah> ; smbus kickoff
lb_74:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_jne, 000000010h, lb_74-$-4> ; spin until smbus is ready
	xcode<xc_io_write, 00000c000h, 000000010h> ; smbus clear status
	xcode<xc_jmp, 0h, lb_75-$-4> 
lb_73:
	xcode<xc_io_write, 00000c004h, 00000008ah> ; 871 encoder slave addr
	xcode<xc_io_write, 00000c008h, 0000000bah> ; smbus set cmd
	xcode<xc_io_write, 00000c006h, 00000003fh> ; smbus set val
	xcode<xc_io_write, 00000c002h, 00000000ah> ; smbus kickoff
lb_76:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_jne, 000000010h, lb_76-$-4> ; spin until smbus is ready
	xcode<xc_io_write, 00000c000h, 000000010h> ; smbus clear status
	xcode<xc_io_write, 00000c008h, 00000006ch> ; smbus set cmd
	xcode<xc_io_write, 00000c006h, 000000046h> ; smbus set val
	xcode<xc_io_write, 00000c002h, 00000000ah> ; smbus kickoff
lb_77:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_jne, 000000010h, lb_77-$-4> ; spin until smbus is ready
	xcode<xc_io_write, 00000c000h, 000000010h> ; smbus clear status
	xcode<xc_io_write, 00000c008h, 0000000b8h> ; smbus set cmd
	xcode<xc_io_write, 00000c006h, 000000000h> ; smbus set val
	xcode<xc_io_write, 00000c002h, 00000000ah> ; smbus kickoff
lb_78:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_jne, 000000010h, lb_78-$-4> ; spin until smbus is ready
	xcode<xc_io_write, 00000c000h, 000000010h> ; smbus clear status
	xcode<xc_io_write, 00000c008h, 0000000ceh> ; smbus set cmd
	xcode<xc_io_write, 00000c006h, 000000019h> ; smbus set val
	xcode<xc_io_write, 00000c002h, 00000000ah> ; smbus kickoff
lb_79:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_jne, 000000010h, lb_79-$-4> ; spin until smbus is ready
	xcode<xc_io_write, 00000c000h, 000000010h> ; smbus clear status
	xcode<xc_io_write, 00000c008h, 0000000c6h> ; smbus set cmd
	xcode<xc_io_write, 00000c006h, 00000009ch> ; smbus set val
	xcode<xc_io_write, 00000c002h, 00000000ah> ; smbus kickoff
lb_80:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_jne, 000000010h, lb_80-$-4> ; spin until smbus is ready
	xcode<xc_io_write, 00000c000h, 000000010h> ; smbus clear status
	xcode<xc_io_write, 00000c008h, 000000032h> ; smbus set cmd
	xcode<xc_io_write, 00000c006h, 000000008h> ; smbus set val
	xcode<xc_io_write, 00000c002h, 00000000ah> ; smbus kickoff
lb_81:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_jne, 000000010h, lb_81-$-4> ; spin until smbus is ready
	xcode<xc_io_write, 00000c000h, 000000010h> ; smbus clear status
	xcode<xc_io_write, 00000c008h, 0000000c4h> ; smbus set cmd
	xcode<xc_io_write, 00000c006h, 000000001h> ; smbus set val
	xcode<xc_io_write, 00000c002h, 00000000ah> ; smbus kickoff
lb_82:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_jne, 000000010h, lb_82-$-4> ; spin until smbus is ready
	xcode<xc_io_write, 00000c000h, 000000010h> ; smbus clear status
lb_75:
	xcode<xc_io_write, 00000c004h, 000000020h> ; smc slave write addr
	xcode<xc_io_write, 00000c008h, 000000001h> ; smbus read revision register
	xcode<xc_io_write, 00000c006h, 000000000h> ; smbus set val
	xcode<xc_io_write, 00000c002h, 00000000ah> ; smbus kickoff
lb_83:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_jne, 000000010h, lb_83-$-4> ; spin until smbus is ready
	xcode<xc_io_write, 00000c000h, 000000010h> ; smbus clear status
	xcode<xc_io_write, 00000c004h, 000000021h> ; smc slave read addr
	xcode<xc_io_write, 00000c008h, 000000001h> ; smbus read revision register
	xcode<xc_io_write, 00000c002h, 00000000ah> ; smbus kickoff
lb_84:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_jne, 000000010h, lb_84-$-4> ; spin until smbus is ready
	xcode<xc_io_write, 00000c000h, 000000010h> ; smbus clear status
	xcode<xc_io_read, 00000c006h, 0h> 
	xcode<xc_jne, 000000050h, lb_85-$-4> 
	xcode<xc_jmp, 0h, lb_86-$-4> 
lb_85:
	xcode<xc_pci_write, 08000036ch, 001000000h> ; smbus is bad, flatline clks
lb_86:
	xcode<xc_mem_write, 00f680500h, 000011c01h> ; set nv clk 233MHz (@ 16.667MHz)
	xcode<xc_mem_write, 00f68050ch, 0000a0400h> 
	xcode<xc_mem_write, 00f001220h, 000000000h> 
	xcode<xc_mem_write, 00f001228h, 000000000h> 
	xcode<xc_mem_write, 00f001264h, 000000000h> 
	xcode<xc_mem_write, 00f001210h, 000000010h> 
	xcode<xc_mem_read, 00f101000h, 0h> 
	xcode<xc_and_or, 006000000h, 000000000h> 
	xcode<xc_jne, 000000000h, lb_87-$-4> 
	xcode<xc_mem_write, 00f001214h, 048480848h> 
	xcode<xc_mem_write, 00f00122ch, 088888888h> 
	xcode<xc_jmp, 0h, lb_88-$-4> 
lb_87:
	xcode<xc_jne, 006000000h, lb_89-$-4> 
	xcode<xc_mem_write, 00f001214h, 009090909h> ; configure pad for samsung
	xcode<xc_mem_write, 00f00122ch, 0aaaaaaaah> 
	xcode<xc_jmp, 0h, lb_88-$-4> 
lb_89:
	xcode<xc_mem_write, 00f001214h, 009090909h> ; configure pad for samsung
	xcode<xc_mem_write, 00f00122ch, 0aaaaaaaah> 
lb_88:
	xcode<xc_mem_write, 00f001230h, 0ffffffffh> 
	xcode<xc_mem_write, 00f001234h, 0aaaaaaaah> 
	xcode<xc_mem_write, 00f001238h, 0aaaaaaaah> 
	xcode<xc_mem_write, 00f00123ch, 08b8b8b8bh> 
	xcode<xc_mem_write, 00f001240h, 0ffffffffh> 
	xcode<xc_mem_write, 00f001244h, 08b8b8b8bh> 
	xcode<xc_mem_write, 00f001248h, 08b8b8b8bh> 
	xcode<xc_mem_write, 00f1002d4h, 000000001h> 
	xcode<xc_mem_write, 00f1002c4h, 000100042h> 
	xcode<xc_mem_write, 00f1002cch, 000100042h> 
	xcode<xc_mem_write, 00f1002c0h, 000000011h> 
	xcode<xc_mem_write, 00f1002c8h, 000000011h> 
	xcode<xc_mem_write, 00f1002c0h, 000000032h> 
	xcode<xc_mem_write, 00f1002c8h, 000000032h> 
	xcode<xc_mem_write, 00f1002c0h, 000000132h> 
	xcode<xc_mem_write, 00f1002c8h, 000000132h> 
	xcode<xc_mem_write, 00f1002d0h, 000000001h> 
	xcode<xc_mem_write, 00f1002d0h, 000000001h> 
	xcode<xc_mem_write, 00f100210h, 080000000h> 
	xcode<xc_mem_write, 00f00124ch, 0aa8baa8bh> 
	xcode<xc_mem_write, 00f001250h, 00000aa8bh> 
	xcode<xc_mem_write, 00f100228h, 0081205ffh> 
	xcode<xc_mem_write, 00f001218h, 000010000h> 
	xcode<xc_pci_read, 080000860h, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000400h> 
	xcode<xc_use_result, xc_pci_write, 080000860h> ; don't gen INIT# on powercycle
	xcode<xc_pci_write, 08000084ch, 00000fddeh> 
	xcode<xc_pci_write, 08000089ch, 0871cc707h> 
	xcode<xc_pci_read, 0800008b4h, 0h> 
	xcode<xc_and_or, 0fffff0ffh, 000000f00h> 
	xcode<xc_use_result, xc_pci_write, 0800008b4h> 
	xcode<xc_pci_write, 080000340h, 0f0f0c0c0h> 
	xcode<xc_pci_write, 080000344h, 000c00000h> 
	xcode<xc_pci_write, 08000035ch, 004070000h> 
	xcode<xc_pci_write, 08000036ch, 000230801h> 
	xcode<xc_pci_write, 08000036ch, 001230801h> 
	xcode<xc_jmp, 0h, lb_90-$-4> ; 15ns delay by performing jmps
lb_90:
	xcode<xc_jmp, 0h, lb_91-$-4> 
lb_91:
	xcode<xc_mem_write, 00f100200h, 003070103h> ;  set extbank bit (00000F00)
	xcode<xc_mem_write, 00f100204h, 011448000h> 
	xcode<xc_pci_write, 08000103ch, 000000000h> ; clear scratch pad (mem type)
	xcode<xc_pci_write, 08000183ch, 000000000h> ; clear scratch pad (mem result)
	xcode<xc_mem_write, 000555548h, 0aaaaaaaah> ; mem test pattern 1
	xcode<xc_mem_write, 002555548h, 05a5a5a5ah> ; mem test pattern 2
	xcode<xc_mem_read, 002555548h, 0h> 
	xcode<xc_jne, 05a5a5a5ah, lb_92-$-4> ; does dram exist?
	xcode<xc_mem_read, 000555548h, 0h> 
	xcode<xc_jne, 0aaaaaaaah, lb_93-$-4> 
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000001h> 
	xcode<xc_use_result, xc_pci_write, 08000103ch> 
lb_93:
	xcode<xc_mem_write, 000555548h, 055555555h> ; mem test pattern 3
	xcode<xc_mem_read, 000555548h, 0h> 
	xcode<xc_jne, 055555555h, lb_92-$-4> 
	xcode<xc_jmp, 0h, lb_94-$-4> 
lb_92:
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000001h> 
	xcode<xc_use_result, xc_pci_write, 08000183ch> 
lb_94:
	xcode<xc_mem_write, 000555558h, 0aaaaaaaah> ; mem test pattern 1
	xcode<xc_mem_write, 002555558h, 05a5a5a5ah> ; mem test pattern 2
	xcode<xc_mem_read, 002555558h, 0h> 
	xcode<xc_jne, 05a5a5a5ah, lb_95-$-4> ; does dram exist?
	xcode<xc_mem_read, 000555558h, 0h> 
	xcode<xc_jne, 0aaaaaaaah, lb_96-$-4> 
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000002h> 
	xcode<xc_use_result, xc_pci_write, 08000103ch> 
lb_96:
	xcode<xc_mem_write, 000555558h, 055555555h> ; mem test pattern 3
	xcode<xc_mem_read, 000555558h, 0h> 
	xcode<xc_jne, 055555555h, lb_95-$-4> 
	xcode<xc_jmp, 0h, lb_97-$-4> 
lb_95:
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000002h> 
	xcode<xc_use_result, xc_pci_write, 08000183ch> 
lb_97:
	xcode<xc_mem_write, 000555568h, 0aaaaaaaah> ; mem test pattern 1
	xcode<xc_mem_write, 002555568h, 05a5a5a5ah> ; mem test pattern 2
	xcode<xc_mem_read, 002555568h, 0h> 
	xcode<xc_jne, 05a5a5a5ah, lb_98-$-4> ; does dram exist?
	xcode<xc_mem_read, 000555568h, 0h> 
	xcode<xc_jne, 0aaaaaaaah, lb_99-$-4> 
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000004h> 
	xcode<xc_use_result, xc_pci_write, 08000103ch> 
lb_99:
	xcode<xc_mem_write, 000555568h, 055555555h> ; mem test pattern 3
	xcode<xc_mem_read, 000555568h, 0h> 
	xcode<xc_jne, 055555555h, lb_98-$-4> 
	xcode<xc_jmp, 0h, lb_100-$-4> 
lb_98:
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000004h> 
	xcode<xc_use_result, xc_pci_write, 08000183ch> 
lb_100:
	xcode<xc_mem_write, 000555578h, 0aaaaaaaah> ; mem test pattern 1
	xcode<xc_mem_write, 002555578h, 05a5a5a5ah> ; mem test pattern 2
	xcode<xc_mem_read, 002555578h, 0h> 
	xcode<xc_jne, 05a5a5a5ah, lb_101-$-4> ; does dram exist?
	xcode<xc_mem_read, 000555578h, 0h> 
	xcode<xc_jne, 0aaaaaaaah, lb_102-$-4> 
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000008h> 
	xcode<xc_use_result, xc_pci_write, 08000103ch> 
lb_102:
	xcode<xc_mem_write, 000555578h, 055555555h> ; mem test pattern 3
	xcode<xc_mem_read, 000555578h, 0h> 
	xcode<xc_jne, 055555555h, lb_101-$-4> 
	xcode<xc_jmp, 0h, lb_103-$-4> 
lb_101:
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000008h> 
	xcode<xc_use_result, xc_pci_write, 08000183ch> 
lb_103:
	xcode<xc_mem_write, 004555548h, 0aaaaaaaah> ; mem test pattern 1
	xcode<xc_mem_write, 006555548h, 05a5a5a5ah> ; mem test pattern 2
	xcode<xc_mem_read, 006555548h, 0h> 
	xcode<xc_jne, 05a5a5a5ah, lb_104-$-4> ; does dram exist?
	xcode<xc_mem_read, 004555548h, 0h> 
	xcode<xc_jne, 0aaaaaaaah, lb_105-$-4> 
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000010h> 
	xcode<xc_use_result, xc_pci_write, 08000103ch> 
lb_105:
	xcode<xc_mem_write, 004555548h, 055555555h> ; mem test pattern 3
	xcode<xc_mem_read, 004555548h, 0h> 
	xcode<xc_jne, 055555555h, lb_104-$-4> 
	xcode<xc_jmp, 0h, lb_106-$-4> 
lb_104:
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000010h> 
	xcode<xc_use_result, xc_pci_write, 08000183ch> 
lb_106:
	xcode<xc_mem_write, 004555558h, 0aaaaaaaah> ; mem test pattern 1
	xcode<xc_mem_write, 006555558h, 05a5a5a5ah> ; mem test pattern 2
	xcode<xc_mem_read, 006555558h, 0h> 
	xcode<xc_jne, 05a5a5a5ah, lb_107-$-4> ; does dram exist?
	xcode<xc_mem_read, 004555558h, 0h> 
	xcode<xc_jne, 0aaaaaaaah, lb_108-$-4> 
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000020h> 
	xcode<xc_use_result, xc_pci_write, 08000103ch> 
lb_108:
	xcode<xc_mem_write, 004555558h, 055555555h> ; mem test pattern 3
	xcode<xc_mem_read, 004555558h, 0h> 
	xcode<xc_jne, 055555555h, lb_107-$-4> 
	xcode<xc_jmp, 0h, lb_109-$-4> 
lb_107:
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000020h> 
	xcode<xc_use_result, xc_pci_write, 08000183ch> 
lb_109:
	xcode<xc_mem_write, 004555568h, 0aaaaaaaah> ; mem test pattern 1
	xcode<xc_mem_write, 006555568h, 05a5a5a5ah> ; mem test pattern 2
	xcode<xc_mem_read, 006555568h, 0h> 
	xcode<xc_jne, 05a5a5a5ah, lb_110-$-4> ; does dram exist?
	xcode<xc_mem_read, 004555568h, 0h> 
	xcode<xc_jne, 0aaaaaaaah, lb_111-$-4> 
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000040h> 
	xcode<xc_use_result, xc_pci_write, 08000103ch> 
lb_111:
	xcode<xc_mem_write, 004555568h, 055555555h> ; mem test pattern 3
	xcode<xc_mem_read, 004555568h, 0h> 
	xcode<xc_jne, 055555555h, lb_110-$-4> 
	xcode<xc_jmp, 0h, lb_112-$-4> 
lb_110:
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000040h> 
	xcode<xc_use_result, xc_pci_write, 08000183ch> 
lb_112:
	xcode<xc_mem_write, 004555578h, 0aaaaaaaah> ; mem test pattern 1
	xcode<xc_mem_write, 006555578h, 05a5a5a5ah> ; mem test pattern 2
	xcode<xc_mem_read, 006555578h, 0h> 
	xcode<xc_jne, 05a5a5a5ah, lb_113-$-4> ; does dram exist?
	xcode<xc_mem_read, 004555578h, 0h> 
	xcode<xc_jne, 0aaaaaaaah, lb_114-$-4> 
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000080h> 
	xcode<xc_use_result, xc_pci_write, 08000103ch> 
lb_114:
	xcode<xc_mem_write, 004555578h, 055555555h> ; mem test pattern 3
	xcode<xc_mem_read, 004555578h, 0h> 
	xcode<xc_jne, 055555555h, lb_113-$-4> 
	xcode<xc_jmp, 0h, lb_115-$-4> 
lb_113:
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0ffffffffh, 000000080h> 
	xcode<xc_use_result, xc_pci_write, 08000183ch> 
lb_115:
	xcode<xc_mem_write, 00f100200h, 003070003h> ;  clear extbank bit (00000F00)
	xcode<xc_pci_write, 080000084h, 003ffffffh> ; set memory size 64 Mb
	xcode<xc_pci_read, 08000103ch, 0h> 
	xcode<xc_and_or, 0000000ffh, 000000000h> 
	xcode<xc_use_result, xc_io_write, 00000c006h> 
	xcode<xc_io_read, 00000c006h, 0h> 
	xcode<xc_jne, 000000000h, lb_116-$-4> 
	xcode<xc_mem_write, 00f100200h, 003070103h> ;  set extbank bit (00000F00)
	xcode<xc_mem_write, 00f100204h, 011338000h> 
	xcode<xc_jmp, 0h, lb_117-$-4> 
lb_116:
	xcode<xc_io_read, 00000c006h, 0h> 
	xcode<xc_jne, 0000000ffh, lb_117-$-4> 
	xcode<xc_mem_write, 00f100200h, 003070103h> ;  set extbank bit (00000F00)
	xcode<xc_pci_write, 080000084h, 007ffffffh> ; set memory size 128 Mb
lb_117:
	xcode<xc_io_write, 00000c004h, 000000020h> ; smc slave write addr
	xcode<xc_io_write, 00000c008h, 000000013h> ; smbus set cmd
	xcode<xc_io_write, 00000c002h, 00000000ah> ; smbus kickoff
lb_118:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_jne, 000000010h, lb_118-$-4> ; spin until smbus is ready
	xcode<xc_io_write, 00000c000h, 000000010h> ; smbus clear status
	xcode<xc_pci_read, 08000183ch, 0h> 
	xcode<xc_and_or, 0000000ffh, 000000000h> 
	xcode<xc_use_result, xc_io_write, 00000c006h> 
	xcode<xc_io_write, 00000c008h, 000000012h> ; smbus set cmd
	xcode<xc_io_write, 00000c002h, 00000000ah> ; smbus kickoff
lb_119:
	xcode<xc_io_read, 00000c000h, 0h> ; smbus read status
	xcode<xc_jne, 000000010h, lb_119-$-4> ; spin until smbus is ready
	xcode<xc_io_write, 00000c000h, 000000010h> ; smbus clear status
	xcode<xc_pci_write, 08000f020h, 0fdf0fd00h> ; reload nv reg base
	xcode<xc_pci_write, 080010010h, 0fd000000h> 
lb_67:
	xcode<xc_mem_write, 000000000h, 07fc900beh> ; visor
	xcode<xc_mem_write, 000000004h, 00000bfffh> 
	xcode<xc_mem_write, 000000008h, 000b90009h> 
	xcode<xc_mem_write, 00000000ch, 0fc000018h> 
	xcode<xc_mem_write, 000000010h, 01d8ba5f3h> 
	xcode<xc_mem_write, 000000014h, 000090000h> 
	xcode<xc_mem_write, 000000018h, 00000e3ffh> 
	xcode<xc_mem_write, 00000001ch, 000000000h> 
	xcode<xc_exit, 000000806h, 0h> ; quit xcodes

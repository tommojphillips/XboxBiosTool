// XcodeInterp.cpp: defines functions for interpreting XCODE instructions

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

// std incl
#include <cstring>

// user incl
#include "XcodeInterp.h"
#include "util.h"

#define XC_WRITE_COMMENT(xc_cmd, xc_addr, xc_data, comment) \
{ \
	if ((_ptr->opcode == xc_cmd) && (_ptr->addr == xc_addr) && (xc_data == NULL ? true : (xc_data == _ptr->data))) \
	{ \
		strcat(str, comment); \
		return 0; \
	} \
}

int XcodeInterp::load(UCHAR* data, UINT size)
{
	if (_data != NULL)
	{
		error("Error: XCODE data is already loaded\n");
		return 1;
	}

	_data = (UCHAR*)xb_alloc(size);
	if (_data == NULL)
	{
		return 1;
	}

	xb_cpy(_data, data, size);

	_size = size;

	reset();

	return 0;
}

void XcodeInterp::unload()
{
	_size = 0;
	reset();

	if (_data != NULL)
	{
		xb_free(_data);
		_data = NULL;
	}
}

int XcodeInterp::interpretNext(XCODE*& xcode)
{
	if (_status == INTERP_STATUS::EXIT_OP_FOUND) // last xcode was an exit. dont interpret anymore xcodes after this
		return 1;

	if (_offset + sizeof(XCODE) > _size) // check if offset is within bounds
	{
		_status = INTERP_STATUS::DATA_ERROR;
		error("Error: EXIT opcode '%X' not found. End of data reached\n", XC_EXIT);
		return 1;
	}

	_ptr = (XCODE*)((UCHAR*)_data + _offset);
	
	xcode = _ptr;

	if (xcode->opcode == XC_EXIT)
	{
		_status = INTERP_STATUS::EXIT_OP_FOUND;
	}
	else
	{
		_status = INTERP_STATUS::DATA_OK;
	}
	
	_offset += sizeof(XCODE);

	return 0;
}

int XcodeInterp::isOpcodeValid(UCHAR opcode)
{
	return (opcode >= XC_MEM_READ && opcode <= XC_EXIT) == 0;
}

int XcodeInterp::getOpcodeStr(char* str)
{
	const UCHAR opcode = _ptr->opcode;

	switch (opcode)
	{
	case XC_MEM_READ:
		strcpy(str, "mem_read");
		break;
	case XC_MEM_WRITE:
		strcpy(str, "mem_write");
		break;
	case XC_PCI_WRITE:
		strcpy(str, "pci_write");
		break;
	case XC_PCI_READ:
		strcpy(str, "pci_read");
		break;
	case XC_AND_OR:
		strcpy(str, "and_or");
		break;
	case XC_USE_RESULT:
		strcpy(str, "use_rslt");
		break;						
	case XC_JNE:
		strcpy(str, "jne");
		break;
	case XC_JMP:
		strcpy(str, "jmp");
		break;
	case XC_ACCUM:
		strcpy(str, "accum");
		break;
	case XC_IO_WRITE:
		strcpy(str, "io_write");
		break;
	case XC_IO_READ:
		strcpy(str, "io_read");
		break;
	case XC_EXIT:
		strcpy(str, "exit");
		break;

	case XC_NOP:
	default:
		format(str, "nop_%X", opcode);
		break;
	}

	return 0;
}

int XcodeInterp::getAddressStr(char* str)
{
	const UCHAR opcode = _ptr->opcode;

	// opcodes that dont use addr field
	switch (opcode)
	{
		case XC_JMP:
			str[0] = '\0';
			return 1;
	}

	format(str, "0x%X", _ptr->addr);

	return 0;
}

int XcodeInterp::getDataStr(char* str)
{
	switch (_ptr->opcode)
	{
		// opcodes that dont use data field
		case XC_MEM_READ:
		case XC_IO_READ:
		case XC_PCI_READ:
		case XC_EXIT:
			str[0] = '\0';
			break;

		case XC_JNE:
		case XC_JMP:
			break;

		default:
			format(str, "0x%X", _ptr->data);
			break;
	}

	return 0;
}

int XcodeInterp::getCommentStr(char* str)
{
	XC_WRITE_COMMENT(XC_JNE, 0x10, 0xFFFFFFEE, "\t ; spin until smbus is ready");

	XC_WRITE_COMMENT(XC_IO_READ, SMB_BASE + 0x00, NULL, "smbus read status");
	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x00, 0x10, "smbus clear status");
	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x08, NULL, "smbus set cmd");
	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x06, NULL, "smbus set val");

	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x02, 0x0A, "smbus kickoff");

	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x04, 0x20, "smc slave write addr");
	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x04, 0x21, "smc slave read addr");
	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x08, 0x01, "smc read revision register");
	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x04, 0x8A, "871 slave addr");

	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x80000884, 0x8001, "setup io bar (rev >= C03)");
	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x80000810, 0x8001, "setup io bar (rev < C03)");
	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x80000804, 0x03, "enable io space");
	XC_WRITE_COMMENT(XC_IO_WRITE, 0x8049, 0x08, "disable the tco timer");

	XC_WRITE_COMMENT(XC_IO_WRITE, 0x80D9, 0x00, "KBDRSTIN# in gpio mode");
	XC_WRITE_COMMENT(XC_IO_WRITE, 0x8026, 0x01, "disable PWRBTN#");

	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x8000F04C, 0x01, "enable internal graphics");
	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x8000F018, 0x10100, "setup secondary bus 1");
	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x8000036C, 0x1000000, "smbus is bad, flatline clks");

	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x80010010, NV2A_BASE, "set NV20 register base");
	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x8000F020, 0xFDF0FD00, "reload NV20 register base");

	XC_WRITE_COMMENT(XC_MEM_READ, NV2A_BASE, NULL, "PCM_BOOT_0");

	XC_WRITE_COMMENT(XC_MEM_WRITE, NV2A_BASE+0x101000, 0x803D4401, "PEXTDEV_BOOT_0");
	XC_WRITE_COMMENT(XC_MEM_READ, NV2A_BASE + 0x101000, NULL, "PEXTDEV_BOOT_0");

	XC_WRITE_COMMENT(XC_MEM_WRITE, NV2A_BASE + 0x1214, 0x28282828, "configure for micron");
	XC_WRITE_COMMENT(XC_MEM_WRITE, NV2A_BASE + 0x1214, 0x09090909, "configure for samsung");

	XC_WRITE_COMMENT(XC_EXIT, 0x806, NULL, "end of xcodes");

	return 0;
}

void XcodeInterp::reset()
{
	_offset = 0;
	_status = INTERP_STATUS::UNK;
	_ptr = NULL;
}

int XcodeInterp::decodeXcode(XCODE*& xcode, std::list<LABEL>& labels)
{
	const UINT BASE_OFFSET = sizeof(INIT_TBL);

	bool found = false;
	UINT len = 0;
	UINT offset = _offset - sizeof(XCODE);

	char str_label[10 + 1] = { 0 };
	char str_opcode[10 + 1] = { 0 };
	char str_addr[10 + 1] = { 0 };
	char str_data[10 + 1] = { 0 };
	char str_comment[64] = { 0 };

	// check if offset should be a label
	for (LABEL& label : labels)
	{
		if (label.offset == offset)
		{
			format(str_label, "%s:", label.name);
			len = strlen(str_label);
			break;
		}
	}
	xb_set(str_label + len, ' ', sizeof(str_label) - len - 1);

	// get the opcode string
	getOpcodeStr(str_opcode);
	len = strlen(str_opcode);
	xb_set(str_opcode + len, ' ', sizeof(str_opcode) - len - 1);

	// get the address string
	getAddressStr(str_addr);
	len = strlen(str_addr);
	xb_set(str_addr + len, ' ', sizeof(str_addr) - len - 1);

	// get the data string
	switch (xcode->opcode)
	{
	case XC_JMP:
	case XC_JNE:
		found = false;
		for (LABEL& label : labels)
		{
			if (label.offset == xcode->data)
			{
				found = true;
				if (xcode->opcode == XC_JNE)
				{
					format(str_data, "%s:", label.name);
				}
				else
				{
					// jmp does not use addr, so set it to the label name
					format(str_addr, "%s:", label.name);
				}
				break;
			}
		}
		if (!found)
		{
			error("Error: Label not found for offset %04x\n", xcode->data);
		}
		break;

	default:
		getDataStr(str_data);
		break;
	}

	// format the data string
	len = strlen(str_data);
	xb_set(str_data + len, ' ', sizeof(str_data) - len - 1);

	// get the comment string
	xb_zero(str_comment, sizeof(str_comment));
	getCommentStr(str_comment + 2);
	if (str_comment[2] != '\0')
	{
		xb_cpy(str_comment, "; ", 2);
	}

	// print the xcode
	print("%s %04x: %s %s %s %s\n", &str_label, (BASE_OFFSET + offset), &str_opcode, &str_addr, &str_data, &str_comment);

	return 0;
}

int XcodeInterp::decodeXcodes()
{
	XCODE* xcode;
	std::list<LABEL> labels;

	// decode phase 1
	while (SUCCESS(interpretNext(xcode)))
	{
		// go through the xcodes and fix up jmp offsets.

		// if the xcode is a jmp, then calculate the offset and set the data to the offset.
		if (xcode->opcode == XC_JMP || xcode->opcode == XC_JNE)
		{
			bool isLabel = false;
			UINT labelOffset = _offset + xcode->data;

			// Fix up the xcode data to the offset for phase 2 decoding
			xcode->data = labelOffset;

			// check if offset is already a label
			for (LABEL& label : labels)
			{
				if (label.offset == labelOffset)
				{
					isLabel = true;
					break;
				}
			}

			// label does not exist. create one.
			if (!isLabel)
			{
				char str[16] = { 0 };
				format(str, "lb_%02d", labels.size());

				labels.push_back(LABEL(labelOffset, str));
			}
		}
	}

	if (_status == XcodeInterp::DATA_ERROR)
	{
		error("Error: Failed to decode xcodes\n");
		return 1;
	}

	print("xcodes: %d ( %ld bytes )\n", _offset / sizeof(XCODE), _offset);

	// decode phase 2
		
	reset(); // reset the interpreter to the start of the xcodes

	while (interpretNext(xcode) == 0)
	{
		decodeXcode(xcode, labels);		
	}

	labels.clear();

	return 0;
}

/*void Bios::printXcodes()
{
	char str_opcode[16] = { 0 };
	char str_addr[16] = { 0 };
	char str_data[64] = { 0 };

	UCHAR* xcodes = (_bios + sizeof(INIT_TBL));
	XcodeInterp interp = XcodeInterp();
	
	int result;

	print("\nXCODES:\n");

	UINT size = bootParams->inittblSize;
	if (size <= 0 || size > xbtool.params.romsize)
	{
		size = xbtool.params.romsize - sizeof(INIT_TBL); // max possible sizes
	}
	
	result = interp.load(xcodes, size);
	if (!SUCCESS(result))
	{
		error("Error: Failed to load xcodes\n");
		return;
	}

	XCODE* xcode;
	UINT offset = 0;
	UINT len = 0;

	while (interp.interpretNext(xcode) == 0)
	{
		xb_zero(str_data, sizeof(str_data));

		interp.getOpcodeStr(str_opcode);
		interp.getAddressStr(str_addr);
		interp.getDataStr(str_data);
		
		len = strlen(str_opcode);
		xb_set(str_opcode + len, ' ', sizeof(str_opcode) - len - 1);

		len = strlen(str_addr);
		xb_set(str_addr + len, ' ', sizeof(str_addr) - len - 1);
		
		str_opcode[sizeof(str_opcode) - 1] = '\0';
		str_addr[sizeof(str_addr) - 1] = '\0';

		print("%04x: %s %s %s\n", sizeof(INIT_TBL)+offset, &str_opcode, &str_addr, &str_data);

		offset = interp.getOffset();
	}

	print("xcodes: %d ( %ld bytes )\n", offset / sizeof(XCODE), offset);
}*/

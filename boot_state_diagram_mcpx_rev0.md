## MCPX X3 1.0
```mermaid
flowchart TB

PWR_ON[Power On]-->SB_XCODE_INTERP
SB_XCODE_INTERP[Run Xcode Interpreter]-->SB_DECRYPT_2BL

SB_DECRYPT_2BL[RC4 Decrypt 2BL]-->SB_LOAD_STORED_HASH
SB_LOAD_STORED_HASH[Load Expected Hash]-->SB_IS_2BL_VALID{Does the hash match?}
SB_IS_2BL_VALID-->|Yes|2BL_ENTRY
SB_IS_2BL_VALID-->|No|SB_SHUTDOWN

2BL_ENTRY[Jump to 2BL Entry]
SB_SHUTDOWN[SB Shutdown]

click 2BL_ENTRY "https://github.com/tommojphillips/XboxBiosTool/blob/master/boot_state_diagram_2bl.md"
```

- [MCPX X3 1.1](boot_state_diagram_mcpx_rev1.md)

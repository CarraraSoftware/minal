/* This file is an adaption of Xterm's documentation on ANSI Escape Sequences, 
 * with the intention of organizing it in a more parsing friendly way
 * (a bunch of nested switch statements).
 * Xterm home page:             https://xterm.dev/
 * Xterm other home page:       https://invisible-island.net/xterm/.
 * Xterm ANSI Escape Sequences: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
 * Xterm License:
=============  XTERM LINCENSE ==================================================
  MIT License

Copyright (c) 1997-2024 by Thomas E. Dickey et al.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
=============  XTERM LINCENSE ==================================================
*/


/*-----------------------------------------------------------------------------/
/                        ASCII/C0 Control Codes                                /
/                                                                              /
/  Single Character Functions                                                  /
/-----------------------------------------------------------------------------*/
//                                 Byte       Description
#define NULL_                      0x00 	// (NUL is Ctrl-@)  
#define START_OF_HEADING 	       0x01 	// (SOH is Ctrl-A)  
#define START_OF_TEXT 	           0x02 	// (STX is Ctrl-B)  
#define END_OF_TEXT                0x03 	// (ETX is Ctrl-C)  
#define END_OF_TRANSMISSION 	   0x04 	// (EOT is Ctrl-D)  
#define ENQUIRY  	               0x05 	// (ENQ is Ctrl-E)  
#define ACKNOWLEDGE 	           0x06 	// (ACK is Ctrl-F)  
#define BELL                       0x07     // (BEL is Ctrl-G)              
#define BACKSPACE                  0x08     // (BS  is Ctrl-H)               
#define TAB                        0x09     // (HT  is Ctrl-I)              
#define LINEFEED                   0x0A     // (NL  is Ctrl-J)
#define VERTTAB                    0x0B     // (VT  is Ctrl-K)               
#define FORMFEED                   0x0C     // (NP  is Ctrl-L)                         
#define CARRIAGERET                0x0D     // (CR  is Ctrl-M)               
#define SHIFT_OUT                  0x0E     // (SO  is Ctrl-N)
#define SHIFT_IN                   0x0F     // (SI  is Ctrl-O)
#define DATA_LINK_ESCAPE      	   0x10     // (DLE is Ctrl+P)
#define DEVICE_CONTROL_ONE    	   0x11     // (DC1 is Ctrl+Q)
#define DEVICE_CONTROL_TWO    	   0x12     // (DC2 is Ctrl+R)
#define DEVICE_CONTROL_THREE  	   0x13     // (DC3 is Ctrl+S)
#define DEVICE_CONTROL_FOUR   	   0x14     // (DC4 is Ctrl+T)
#define NEGATIVE_ACKNOWLEDGE  	   0x15     // (NAK is Ctrl+U)
#define SYNCHRONOUS_IDLE      	   0x16     // (SYN is Ctrl+V)
#define END_TRANSMISSION_BLOCK	   0x17     // (ETB is Ctrl+W)
#define CANCEL                	   0x18     // (CAN is Ctrl+X)
#define END_OF_MEDIUM         	   0x19     // (EM  is Ctrl+Y)
#define SUBSTITUTE            	   0x1A     // (SUB is Ctrl+Z)
#define ESC                        0x1B     // (ESC is Ctrl-[)               
#define FILE_SEPARATOR   	       0x1C     // (FS  is Ctrl+\)
#define GROUP_SEPARATOR  	       0x1D     // (GS  is Ctrl+]) 
#define RECORD_SEPARATOR 	       0x1E     // (RS  is Ctrl+^) 
#define UNIT_SEPARATOR   	       0x1F     // (US  is Ctrl+_) 
//--
#define SP                         ' '      //
#define DEL                        0x7F     //

/*-----------------------------------------------------------------------------/
/                        C1(?) Control Codes                                   /
/                                                                              /
/  Codes that have the format ESC + <byte>.                                    /
/  The byte may be followed by other bytes to form some other command          /
/  or the byte itself may be a command.(Needs confirmation)                    /
/-----------------------------------------------------------------------------*/
//                                  Byte      Description
#define INDEX	                    'D'    // (ID    is 0x84) 
#define NEXT_LINE                   'E'    // (NL    is 0x85) 
#define TAB_SET                     'H'    // (HT    is 0x88) 
#define REVERSE_INDEX	            'M'    // (RI    is 0x8d) 
#define SINGLE_SHIFT_TWO            'N'    // (SS2   is 0x8e)
#define SINGLE_SHIFT_THREE          'O'    // (SS3   is 0x8f)
#define DEVICE_CONTROL_STRING       'P'    // (DCS   is 0x90)
#define START_OF_GUARDED_AREA       'V'    // (SPA   is 0x96)
#define END_OF_GUARDED_AREA         'W'    // (EPA   is 0x97)
#define START_OF_STRING             'X'    // (SOS   is 0x98)
#define RETURN_TERMINAL_ID          'Z'    // (DECID is 0x9a)
#define CONTROL_SEQUENCE_INTRODUCER '['    // (CSI   is 0x9b)
#define STRING_TERMINATOR           '\\'   // (ST    is 0x9c) 
#define OPERATING_SYSTEM_COMMAND    ']'    // (OSC   is 0x9d)
#define PRIVACY_MESSAGE             '^'    // (PM    is 0x9e) 
#define APPLICATION_PROGRAM_COMMAND '_'    // (APC   is 0x9f)

/*-----------------------------------------------------------------------------/
/                        Commands Starting With ESC                            /
/                                                                              /
/ These are grouped together because the first byte after ESC is not a category/
/ by itself. So it will be interpreted as a command.                           /
/                                                                              /
/ Some of these commands have arguments. When these arguments have to be one of/
/ a set of options, the options will be listed right bellow the command.       /
/ Otherwise, the argument is arbitrary and specific conventions for each       /
/ command may apply.                                                           /
/                                                                              /
/-----------------------------------------------------------------------------*/
//                                  Byte         Usage                                    | Description
// { CONFORMANCE_CMDS
#define SET_CONFORMANCE             SP     //    --                                       | Prefix to SetConformance commands.
#define SET_CONFORMANCE_7BIT        'F'    //    ESC SET_CONFORMANCE F                    | 7-bit controls (S7C1T), VT220.        
#define SET_CONFORMANCE_8BIT        'G'    //    ESC SET_CONFORMANCE G                    | 8-bit controls (S8C1T), VT220.        
#define SET_CONFORMANCE_L1          'L'    //    ESC SET_CONFORMANCE L                    | Set ANSI conformance level 1, ECMA-43.
#define SET_CONFORMANCE_L2          'M'    //    ESC SET_CONFORMANCE M                    | Set ANSI conformance level 2, ECMA-43.
#define SET_CONFORMANCE_L3          'N'    //    ESC SET_CONFORMANCE N                    | Set ANSI conformance level 3, ECMA-43.
// } CONFORMANCE_CMDS
// { CHAR_SIZE_CMDS
#define DEC_CHAR_SIZE               '#'    //    --                                       | Prefix to CharSize commands.
#define DEC_CHAR_SIZE_2xH_TOP       '3'    //    ESC DEC_CHAR_SIZE 3                      | DEC double-height line, top half (DECDHL), VT100.    
#define DEC_CHAR_SIZE_2xH_BOT       '4'    //    ESC DEC_CHAR_SIZE 4                      | DEC double-height line, bottom half (DECDHL), VT100. 
#define DEC_CHAR_SIZE_1xW_LINE      '5'    //    ESC DEC_CHAR_SIZE 5                      | DEC single-width line (DECSWL), VT100.               
#define DEC_CHAR_SIZE_2xW_LINE      '6'    //    ESC DEC_CHAR_SIZE 6                      | DEC double-width line (DECDWL), VT100.               
#define DEC_SCREEN_ALIGNMENT        '8'    //    ESC DEC_CHAR_SIZE 8                      | DEC Screen Alignment Test (DECALN), VT100.           
// } CHAR_SIZE_CMDS
// { SELECT_CHARSET_CMDS
#define SELECT_CHARSET              '%'    //    --                                       | Prefix to SelectCharacterSet commands.
#define SELECT_CHARSET_DEFAULT      '@'    //    ESC SELECT_CHARSET @                     | Select default character set.         
#define SELECT_CHARSET_UTF8         'G'    //    ESC SELECT_CHARSET G                     | Select UTF-8 character set, ISO 2022. 
// } SELECT_CHARSET_CMDS
// { DESIGNATE_G0
#define DESIGNATE_G0_CHARSET        '('    //    --                                       | Prefix to Designate G0 Character Set, VT100, ISO 2022.
#define G0CHSET_UK                  'A'    //    ESC DESIGNATE_G0_CHARSET A               | United Kingdom (UK), VT100.
#define G0CHSET_US                  'B'    //    ESC DESIGNATE_G0_CHARSET B               | United States (USASCII), VT100.
#define G0CHSET_FINNISH_1           'C'    //    ESC DESIGNATE_G0_CHARSET C               | Finnish, VT200.
#define G0CHSET_FINNISH_2           '5'    //    ESC DESIGNATE_G0_CHARSET 5               | Finnish, VT200.
#define G0CHSET_SWEDISH_1           'H'    //    ESC DESIGNATE_G0_CHARSET H               | Swedish, VT200.
#define G0CHSET_SWEDISH_2           '7'    //    ESC DESIGNATE_G0_CHARSET 7               | Swedish, VT200.
#define G0CHSET_GERMAN              'K'    //    ESC DESIGNATE_G0_CHARSET K               | German, VT200.
#define G0CHSET_FR_CANADIAN_1       'Q'    //    ESC DESIGNATE_G0_CHARSET Q               | French Canadian, VT200.
#define G0CHSET_FR_CANADIAN_2       '9'    //    ESC DESIGNATE_G0_CHARSET 9               | French Canadian, VT200.
#define G0CHSET_FRENCH_1            'R'    //    ESC DESIGNATE_G0_CHARSET R               | French, VT200.
#define G0CHSET_FRENCH_2            'f'    //    ESC DESIGNATE_G0_CHARSET f               | French, VT200.
#define G0CHSET_ITALIAN             'Y'    //    ESC DESIGNATE_G0_CHARSET Y               | Italian, VT200.
#define G0CHSET_SPANISH             'Z'    //    ESC DESIGNATE_G0_CHARSET Z               | Spanish, VT200.
#define G0CHSET_DUTCH               '4'    //    ESC DESIGNATE_G0_CHARSET 4               | Dutch, VT200.
#define G0CHSET_SWISS               '='    //    ESC DESIGNATE_G0_CHARSET =               | Swiss, VT200.
#define G0CHSET_NORWEGIAN_DANISH_1  '`'    //    ESC DESIGNATE_G0_CHARSET `               | Norwegian/Danish, VT200.
#define G0CHSET_NORWEGIAN_DANISH_2  'E',   //    ESC DESIGNATE_G0_CHARSET E               | Norwegian/Danish, VT200.
#define G0CHSET_NORWEGIAN_DANISH_3  '6',   //    ESC DESIGNATE_G0_CHARSET 6               | Norwegian/Danish, VT200.
#define G0CHSET_DEC_SPECIAL         '0'    //    ESC DESIGNATE_G0_CHARSET 0               | DEC Special Character and Line Drawing Set, VT100.
#define G0CHSET_SUPPLEMENTAL        '<'    //    ESC DESIGNATE_G0_CHARSET <               | DEC Supplemental, VT200.
#define G0CHSET_USER_PREFERRED      '<'    //    ESC DESIGNATE_G0_CHARSET <               | User Preferred Selection Set, VT300.
#define G0CHSET_DEC_TECHNICAL       '>'    //    ESC DESIGNATE_G0_CHARSET >               | DEC Technical, VT300.
#define G0CHSET_JIS_KATAKANA        'I'    //    ESC DESIGNATE_G0_CHARSET I               | JIS-Katakana, VT382.
#define G0CHSET_JIS_ROMAN           'J'    //    ESC DESIGNATE_G0_CHARSET J               | JIS-Roman, VT382.
// For some reason, a few G0 Character Set have 2 characters for their selection
// These were organized based on the prefix that starts the selection. Which can be one of ['"', '%', '&']
#define G0CHSET_PREFIX_1            '"'    //    --                                       | Prefix 1.
#define G0CHSET_GREEK               '>'    //    ESC DESIGNATE_G0_CHARSET G0_PREFIX_1 >   | Greek, VT500.
#define G0CHSET_DEC_HEBREW          '4'    //    ESC DESIGNATE_G0_CHARSET G0_PREFIX_1 4   | DEC Hebrew, VT500.
#define G0CHSET_DEC_GREEK           '?'    //    ESC DESIGNATE_G0_CHARSET G0_PREFIX_1 ?   | DEC Greek, VT500.
#define G0CHSET_PREFIX_2            '%'    //    --                                       | Prefix 2
#define G0CHSET_PORTUGUESE          '6'    //    ESC DESIGNATE_G0_CHARSET G0_PREFIX_2 6   | Portuguese, VT300.
#define G0CHSET_HEBREW_PREFIX       '='    //    ESC DESIGNATE_G0_CHARSET G0_PREFIX_2 =   | Hebrew, VT500.
#define G0CHSET_DEC_TURKISH         '0'    //    ESC DESIGNATE_G0_CHARSET G0_PREFIX_2 0   | DEC Turkish, VT500.
#define G0CHSET_DEC_SUP_GRAPHICS    '5'    //    ESC DESIGNATE_G0_CHARSET G0_PREFIX_2 5   | DEC Supplemental Graphics, VT300.
#define G0CHSET_SCS_NRCS            '3'    //    ESC DESIGNATE_G0_CHARSET G0_PREFIX_2 3   | SCS NRCS, VT500.
#define G0CHSET_PREFIX_3            '&'    //    --                                       | Prefix 3
#define G0CHSET_DEC_CYRILLIC        '4'    //    ESC DESIGNATE_G0_CHARSET G0_PREFIX_3 4   | DEC Cyrillic, VT500.
#define G0CHSET_DEC_RUSSIAN         '5'    //    ESC DESIGNATE_G0_CHARSET G0_PREFIX_3 5   | DEC Russian, VT500.
// } DESIGNATE_G0
// { DESIGNATE_G1
#define DESIGNATE_G1_CHARSET        '-'    //    --                                       | Prefix to Designate G1 Character Set, VT300.
#define G1CHSET_LATIN_1             'A'    //    ESC DESIGNATE_G1_CHARSET A               | ISO Latin-1 Supplemental, VT300. 
#define G1CHSET_LATIN_2             'B'    //    ESC DESIGNATE_G1_CHARSET B               | ISO Latin-2 Supplemental, VT500. 
#define G1CHSET_GREEK               'F'    //    ESC DESIGNATE_G1_CHARSET F               | ISO Greek Supplemental, VT500.   
#define G1CHSET_HEBREW              'H'    //    ESC DESIGNATE_G1_CHARSET H               | ISO Hebrew Supplemental, VT500.  
#define G1CHSET_LATIN_CYRILLIC      'L'    //    ESC DESIGNATE_G1_CHARSET L               | ISO Latin-Cyrillic, VT500.       
#define G1CHSET_LATIN_5             'M'    //    ESC DESIGNATE_G1_CHARSET M               | ISO Latin-5 Supplemental, VT500. 
// } DESIGNATE_G1
// { DESIGNATE_Gx_MISC
#define DESIGNATE_G1_CHARSET_2      ')'    //    --                                       | Prefix to Designate G1 Character Set, ISO 2022, VT100.
#define DESIGNATE_G2_CHARSET        '*'    //    --                                       | Prefix to Designate G2 Character Set, ISO 2022, VT220.
#define DESIGNATE_G2_CHARSET_2      '*'    //    --                                       | Prefix to Designate G2 Character Set, ISO 2022, VT220.
#define DESIGNATE_G3_CHARSET        '.'    //    --                                       | Prefix to Designate G2 Character Set, VT300. 
#define DESIGNATE_G3_CHARSET_2      '/'    //    --                                       | Prefix to Designate G3 Character Set, VT300. 
// NOTE: xterm doesn't list what are the options for these 5 prefixes.
// } DESIGNATE_Gx_MISC
#define DEC_BACK_INDEX              '6'    //    ESC 6                                    | Back Index (DECBI), VT420 and up.                
#define DEC_SAVE_CURSOR             '7'    //    ESC 7                                    | Save Cursor (DECSC), VT100.                      
#define DEC_RESTORE_CURSOR	        '8'    //    ESC 8                                    | Restore Cursor (DECRC), VT100.                   
#define DEC_FORWARD_INDEX           '9'    //    ESC 9                                    | Forward Index (DECFI), VT420 and up.                
#define DEC_KEYPAD_APPLICATION_MODE	'='    //    ESC =                                    | Application Keypad (DECKPAM).                       
#define DEC_KEYPAD_NORMAL_MODE	    '>'    //    ESC >                                    | Normal Keypad (DECKPNM), VT100.                     
#define CURSOR_BOT_LEFT             'F'    //    ESC F                                    | Cursor to lower left corner of screen.              
#define FULL_RESET	                'c'    //    ESC c                                    | Full Reset (RIS), VT100.                            
#define MEMORY_LOCK                 'l'    //    ESC l                                    | Memory Lock (per HP terminals).                     
#define MEMORY_UNLOCK               'm'    //    ESC m                                    | Memory Unlock (per HP terminals).                   
#define INVOKE_G2_CHARSET_GL        'n'    //    ESC n                                    | Invoke the G2 Character Set as GL (LS2).            
#define INVOKE_G3_CHARSET_GL        'o'    //    ESC o                                    | Invoke the G3 Character Set as GL (LS3).            
#define INVOKE_G1_CHARSET_GR        '~'    //    ESC ~                                    | Invoke the G3 Character Set as GR (LS3R).           
#define INVOKE_G2_CHARSET_GR        '}'    //    ESC }                                    | Invoke the G2 Character Set as GR (LS2R).           
#define INVOKE_G3_CHARSET_GR        '|'    //    ESC |                                    | Invoke the G1 Character Set as GR (LS1R), VT100.    

/*-----------------------------------------------------------------------------/
/                                                                              /
/                        Application Program Command (APC)                     /
/                        Commands Starting With ESC + APC                      /
/                                                                              /
/----------------------------------------------------------------------------- */
//                                  Byte         Usage                                    | Description
#define APC_NONE                    '\0'    //   ESC APC <t> ST                           | None.

/*-----------------------------------------------------------------------------/
/                                                                              /
/                        Device Control-Functions (DCS)                        /
/                        Commands Starting With ESC + DCS                      /
/                                                                              /
/ All the commands in the format ESC DCS ________.                             /
/ The bytes after the DCS may be grouped in different orders and quantities.   /
/ Each command that follows a specific convention will have its arguments,     /
/ separators, subcommands or any other relevant information listed right bellow/
/ itself.                                                                      /
/                                                                              /
/ Some of these commands have arguments. When these arguments have to be one of/
/ a set of options, the options will be listed right bellow the command.       /
/ Otherwise, the argument is arbitrary and specific conventions for each       /
/ command may apply.                                                           /
/                                                                              /
/ A group of subcommand is any command that requires a specific byte after the /
/ the DCS byte or before final byte. This is not an official terminology, and  /
/ this specific byte is referred as the PREFIX for the subcommand.             /
/ List of formats that begins with ESC + DCS:                                  /
/ ESC DCS _ |   <t> ST
/ ESC DCS   ! u <t> ST
/ ESC DCS _ $ _ <t> ST
/ ESC DCS   + _ <t> ST
/----------------------------------------------------------------------------- */
//                                  Byte         Usage                                    | Description
// { USER_DEFINED_KEYS (DECUDK)
#define USER_DEFINED_KEYS           '|'       // ESC DCS _ | <t> ST                       | User-Defined Keys (DECUDK), VT220 and up.
#define DECUDK_CLEAR_ALL             0        // ESC DCS _ 0 <t> ST                       | Clear all UDK definitions before starting (default).
#define DECUDK_ERASE_BELLOW          1        // ESC DCS _ 1 <t> ST                       | Erase Below (default).
// } USER_DEFINED_KEYS (DECUDK)
#define ASGN_USR_SUPLEM_SETS_PREFIX '!'       // ESC DCS ! u <t> ST                       | Assigning User-Preferred Supplemental Sets (DECAUPSS), VT320, VT510.
#define ASGN_USR_SUPLEM_SETS        'u'       // ESC DCS ! u <t> ST                       | Assigning User-Preferred Supplemental Sets (DECAUPSS), VT320, VT510.
// { BEGIN FORMAT: ESC DCS $ _ <t> ST
#define DCS_DOLLAR_PREFIX           '$'       // ESC DCS _ $ _ <t> ST
#define REQUEST_STATUS_STRING       'q'       // ESC DCS   $ q <t> ST                     | Request Status String (DECRQSS), VT420 and up.
//    { RESTORE_PRESENTATION_STATUS (DECRSPS)
#define RESTORE_PRESENTATION_STATUS 't'       // ESC DCS _ $ t <t> ST                     | Restore presentation status (DECRSPS), VT320 and up. 
#define DECRSPS_DECCIR               1        // ESC DCS 1 $ t <t> ST                     | DECCIR                                               
#define DECRSPS_DECTABSR             2        // ESC DCS 2 $ t <t> ST                     | DECTABSR                                             
//    } RESTORE_PRESENTATION_STATUS (DECRSPS)
// } END FORMAT: ESC DCS $ _ <t> ST
// { BEGIN FORMAT: ESC DCS + _ <t> ST
#define DCS_PLUS_PREFIX             '+'       // ESC DCS + _ <t> ST
#define REQUEST_RESOURCE_VALUES     'Q'       // ESC DCS + Q <t> ST                       | Request resource values (XTGETXRES), xterm.         
#define SET_TERMCAPINFO_DATA        'p'       // ESC DCS + p <t> ST                       | Set Termcap/Terminfo Data (XTSETTCAP), xterm.       
#define REQUEST_TERMPCAPINFO_STRING 'q'       // ESC DCS + q <t> ST                       | Request Termcap/Terminfo String (XTGETTCAP), xterm. 
// } END FORMAT: ESC DCS + _ <t> ST

/*-----------------------------------------------------------------------------/
/                        Commands Starting With ESC + CSI                      /
/                                                                              /
/ All the commands in the format ESC CSI ________.                             /
/ The bytes after the CSI may be grouped in different orders and quantities.   /
/ Each command that follows a specific convention will have its arguments,     /
/ separators, subcommands or any other relevant information listed right bellow/
/ itself.                                                                      /
/                                                                              /
/ Some of these commands have arguments. When these arguments have to be one of/
/ a set of options, the options will be listed right bellow the command.       /
/ Otherwise, the argument is arbitrary and specific conventions for each       /
/ command may apply.                                                           /
/                                                                              /
/ A group of subcommand is any command that requires a specific byte after the /
/ the CSI byte or before final byte. This is not an official terminology, and  /
/ this specific byte is referred as the PREFIX for the subcommand.             /
/ List of formats that begins with ESC + CSI:                                  /
/  ESC CSI <n>* <op>                                                           /
/  ESC CSI <n>* SP <op>                                                        /
/  ESC CSI ? <n>+ <op>                                                         /
/  ESC CSI <n>* # <op>                                                         /
/  ESC CSI > <n>* <op>                                                         /
/  ESC CSI = <n> <op>                                                          /
/  ESC CSI ! <op>                                                              /
/  ESC CSI <n>* " <op>                                                         /
/  ESC CSI <n>* $ <op>                                                         /
/  ESC CSI <n>+ ' <op>                                                         /
/  ESC CSI <n>+ * <op>                                                         /
/  ESC CSI <n> ; <m> ; <l> , <op>                                              /
/                                                                              /
/-----------------------------------------------------------------------------*/
//                                  Byte         Usage                                    | Description
#define SAVE_CURSOR                  's'    //   ESC CSI s                                | Save cursor, available only when DECLRMM is disabled (SCOSC, also ANSI.SYS).
#define RESTORE_CURSOR               'u'    //   ESC CSI u                                | Restore cursor (SCORC, also ANSI.SYS).
#define INSERT_BLANK_CHARS           '@'    //   ESC CSI <n> @                            | Insert <n> (Blank) Character(s) (default = 1) (ICH).
#define CURSOR_UP                    'A'    //   ESC CSI <n> A 	                          | Cursor Up <n> Times (default = 1) (CUU).
#define CURSOR_DOWN                  'B'    //   ESC CSI <n> B 	                          | Cursor Down <n> Times (default = 1) (CUD).                     
#define CURSOR_FORWARD               'C'    //   ESC CSI <n> C 	                          | Cursor Forward <n> Times (default = 1) (CUF).                  
#define CURSOR_BACK                  'D'    //   ESC CSI <n> D 	                          | Cursor Backward <n> Times (default = 1) (CUB).                 
#define CURSOR_NEXT_LINE             'E'    //   ESC CSI <n> E 	                          | Cursor Next Line <n> Times (default = 1) (CNL).                
#define CURSOR_PREVIOUS_LINE         'F'    //   ESC CSI <n> F 	                          | Cursor Preceding Line <n> Times (default = 1) (CPL).           
#define CURSOR_HORIZONTAL_ABSOLUTE   'G'    //   ESC CSI <n> G 	                          | Cursor Character Absolute [column] (default = [row,1]) (CHA). 
#define CURSOR_POSITION              'H'    //   ESC CSI <n> ; <m> H                      | Cursor Position [row;column] (default = [1,1]) (CUP).         
// { ERASE_IN_DISPLAY
#define ERASE_IN_DISPLAY             'J'    //   ESC CSI  _  J 	                          | Erase in Display (ED), VT100.                                
#define ERASE_IN_DISPLAY_BELOW        0     //   ESC CSI  0  J                            | Erase Below (default).    
#define ERASE_IN_DISPLAY_ABOVE        1     //   ESC CSI  1  J                            | Erase Above.              
#define ERASE_IN_DISPLAY_ALL          2     //   ESC CSI  2  J                            | Erase All.                
#define ERASE_IN_DISPLAY_SAVED        3     //   ESC CSI  3  J                            | Erase Saved Lines,  xterm.
// } ERASE_IN_DISPLAY
// { ERASE_IN_LINE
#define ERASE_IN_LINE                'K'    //   ESC CSI  _  K 	                          | Erase in Line (EL), VT100.
#define ERASE_IN_LINE_RIGHT           0     //   ESC CSI  0  K                            | Erase to Right (default). 
#define ERASE_IN_LINE_LEFT            1     //   ESC CSI  1  K                            | Erase to Left.            
#define ERASE_IN_LINE_ALL             2     //   ESC CSI  2  K                            | Erase All.                
// } ERASE_IN_LINE
#define INSERT_LINES                 'L'    //   ESC CSI <n> L                            | Insert <n> Line(s) (default = 1) (IL).
#define DELETE_LINES                 'M'    //   ESC CSI <n> M                            | Delete <n> Line(s) (default = 1) (DL).
#define DELETE_CHARS                 'P'    //   ESC CSI <n> P                            | Delete <n> Character(s) (default = 1) (DCH).
#define SCROLL_UP                    'S'    //   ESC CSI <n> S 	                          | Scroll up <n> lines (default = 1) (SU), VT420, ECMA-48.
#define SCROLL_DOWN                  'T'    //   ESC CSI <n> T 	                          | Scroll down <n> lines (default = 1) (SD), VT420.
#define ERASE_CHARS                  'X'    //   ESC CSI <n> X                            | Erase <n> Character(s) (default = 1) (ECH).
#define BACKWARD_TAB                 'Z'    //   ESC CSI <n> Z                            | Cursor Backward Tabulation <n> tab stops (default = 1) (CBT).
#define SCROLL_DOWN_2                '^'    //   ESC CSI <n> ^                            | Scroll down <n> lines (default = 1) (SD), ECMA-48.
#define CHAR_POSITION_ABSOLUTE       '`'    //   ESC CSI <n> `                            | Character Position Absolute [column] (default = [row,1]) (HPA).
#define CHAR_POSITION_RELATIVE       'a'    //   ESC CSI <n> a                            | Character Position Relative [columns] (default = [row,col+1]) (HPR).
#define REPEAT_PRECED_GRAPHIC_CHAR   'b'    //   ESC CSI <n> b                            | Repeat the preceding graphic character <n> times (REP).
// { DEVICE_ATTRIBUTES (DEVATTRREP)
#define DEVICE_ATTRIBUTES_REPORT     'c'    //   ESC CSI  __  c                           | Send Device Attributes (Primary DA).
#define DEVATTRREP_TERMINAL_ATTR      0     //   ESC CSI  00  c                           | Request attributes from terminal.
#define DEVATTRREP_132_COLUMNS        1     //   ESC CSI  01  c                           | 132-columns.                                 
#define DEVATTRREP_PRINTER            2     //   ESC CSI  02  c                           | Printer.                                     
#define DEVATTRREP_REGIS_GRAPHICS     3     //   ESC CSI  03  c                           | ReGIS graphics.                              
#define DEVATTRREP_SIXEL_GRAPHICS     4     //   ESC CSI  04  c                           | Sixel graphics.                              
#define DEVATTRREP_SELECTIVE_ERASE    6     //   ESC CSI  06  c                           | Selective erase.                             
#define DEVATTRREP_USER_DEFINED_KEYS  8     //   ESC CSI  08  c                           | User-defined keys.                           
#define DEVATTRREP_NAT_REPL_CHSETS    9     //   ESC CSI  09  c                           | National Replacement Character sets.         
#define DEVATTRREP_TECH_CHARS         15    //   ESC CSI  15  c                           | Technical characters.                        
#define DEVATTRREP_LOCATOR_PORT       16    //   ESC CSI  16  c                           | Locator port.                                
#define DEVATTRREP_TERMINAL_STATE     17    //   ESC CSI  17  c                           | Terminal state interrogation.                
#define DEVATTRREP_USER_WINDOWS       18    //   ESC CSI  18  c                           | User windows.                                
#define DEVATTRREP_HOR_SCROLL         21    //   ESC CSI  21  c                           | Horizontal scrolling.                        
#define DEVATTRREP_ANSI_COLOR         22    //   ESC CSI  22  c                           | ANSI color, e.g., VT525.                     
#define DEVATTRREP_RECT_EDITING       28    //   ESC CSI  28  c                           | Rectangular editing.                         
#define DEVATTRREP_ANSI_TEXT_LOCATOR  29    //   ESC CSI  29  c                           | ANSI text locator (i.e., DEC Locator mode).  
// } DEVICE_ATTRIBUTES
#define LINE_POSITION_ABSOLUTE       'd'    //   ESC CSI <n> d                            | Line Position Absolute [row] (default = [1,column]) (VPA).
#define LINE_POSITION_RELATIVE       'e'    //   ESC CSI <n> e                            | Line Position Relative [row    // CSI <n> d  Line Position Absolute [rows] (default = [row+1,column]) (VPR).
#define HORIZONTAL_VERTICAL_POSITION 'f'    //   ESC CSI <n> ; <m> f                      | Horizontal and Vertical Position [row;column] (default = [1,1]) (HVP).
// { TAB_CLEAR
#define TAB_CLEAR                    'g'    //   ESC CSI  _  g                            | Tab Clear (TBC).
#define TAB_CLEAR_CURRENT_COL         0     //   ESC CSI  0  g                            | Clear Current Column (default).
#define TAB_CLEAR_ALL                 3     //   ESC CSI  3  g                            | Clear All.
// } TAB_CLEAR
// { SET_MODE
#define SET_MODE                     'h'    //   ESC CSI __ h                             | Set Mode (SM).
#define SET_MODE_KEYBOARD_ACTION      2     //   ESC CSI 02 h                             | Keyboard Action Mode (KAM).
#define SET_MODE_INSERT               4     //   ESC CSI 04 h                             | Insert Mode (IRM).
#define SET_MODE_SEND_RECEIVE         12    //   ESC CSI 12 h                             | Send/receive (SRM).
#define SET_MODE_AUTO_NEWLINE         20    //   ESC CSI 20 h                             | Automatic Newline (LNM).
// } SET_MODE
// { MEDIA_COPY
#define MEDIA_COPY                   'i'    //   ESC CSI __ i                             | Media Copy (MC).
#define MEDIA_COPY_PRINT_SCREEN       0     //   ESC CSI 00 i                             | Print screen (default).
#define MEDIA_COPY_PRINTER_OFF        4     //   ESC CSI 04 i                             | Turn off printer controller mode.
#define MEDIA_COPY_PRINTER_ON         5     //   ESC CSI 05 i                             | Turn on printer controller mode.
#define MEDIA_COPY_HTML_DUMP          10    //   ESC CSI 10 i                             | HTML screen dump,  xterm.
#define MEDIA_COPY_SVG_DUMP           11    //   ESC CSI 11 i                             | SVG screen dump,  xterm.
// } MEDIA_COPY
// { RESET_MODE
#define RESET_MODE                   'l'    //   ESC CSI __ l                             | Reset Mode (RM).
#define RESET_MODE_KEYBOARD_ACTION    2     //   ESC CSI 02 l                             | Keyboard Action Mode (KAM).
#define RESET_MODE_REPLACE            4     //   ESC CSI 04 l                             | Replace Mode (IRM).
#define RESET_MODE_SEND_RECEIVE       12    //   ESC CSI 12 l                             | Send/receive (SRM).
#define RESET_MODE_NORMAL_LINEFEED    20    //   ESC CSI 20 l                             | Normal Linefeed (LNM).
// } RESET_MODE
// { SELECT_GRAPHIC_RENDITION (SGR)
#define SELECT_GRAPHIC_RENDITION     'm'    //   ESC CSI ____ m 	                      | Character Attributes (SGR).
#define SGR_NORMAL                    0     //   ESC CSI 0000 m                           | Normal (default), VT100.
#define SGR_BOLD_WEIGHT               1     //   ESC CSI 0001 m                           | Bold, VT100.
#define SGR_FAINT_WEIGHT              2     //   ESC CSI 0002 m                           | Faint, decreased intensity, ECMA-48 2nd.
#define SGR_ITALIC                    3     //   ESC CSI 0003 m                           | Italicized, ECMA-48 2nd.
#define SGR_UNDERLINE                 4     //   ESC CSI 0004 m                           | Underlined, VT100.
#define SGR_BLINK                     5     //   ESC CSI 0005 m                           | Blink, VT100.
#define SGR_INVERSE                   7     //   ESC CSI 0007 m                           | Inverse, VT100.
#define SGR_INVISIBLE                 8     //   ESC CSI 0008 m                           | Invisible, i.e., hidden, ECMA-48 2nd, VT300.
#define SGR_CROSSED_OUT               9     //   ESC CSI 0009 m                           | Crossed-out characters, ECMA-48 3rd.
#define SGR_DOUBLE_UNDERLINE          21    //   ESC CSI 0021 m                           | Doubly-underlined, ECMA-48 3rd.
#define SGR_NORMAL_WEIGHT             22    //   ESC CSI 0022 m                           | Normal (neither bold nor faint), ECMA-48 3rd.
#define SGR_NOT_ITALIC                23    //   ESC CSI 0023 m                           | Not italicized, ECMA-48 3rd.
#define SGR_NOT_UNDERLINE             24    //   ESC CSI 0024 m                           | Not underlined, ECMA-48 3rd.
#define SGR_NOT_BLINK                 25    //   ESC CSI 0025 m                           | Steady (not blinking), ECMA-48 3rd.
#define SGR_NOT_INVERSE               27    //   ESC CSI 0027 m                           | Positive (not inverse), ECMA-48 3rd.
#define SGR_VISIBLE                   28    //   ESC CSI 0028 m                           | Visible, i.e., not hidden, ECMA-48 3rd, VT300.
#define SGR_NOT_CROSSED_OUT           29    //   ESC CSI 0029 m                           | Not crossed-out, ECMA-48 3rd.
#define SGR_FG_BLACK 	              30    //   ESC CSI 0030 m                           | Set foreground color to Black.   
#define SGR_FG_RED 	                  31    //   ESC CSI 0031 m                           | Set foreground color to Red.     
#define SGR_FG_GREEN 	              32    //   ESC CSI 0032 m                           | Set foreground color to Green.   
#define SGR_FG_YELLOW 	              33    //   ESC CSI 0033 m                           | Set foreground color to Yellow.  
#define SGR_FG_BLUE 	              34    //   ESC CSI 0034 m                           | Set foreground color to Blue.    
#define SGR_FG_MAGENTA                35    //   ESC CSI 0035 m                           | Set foreground color to Magenta. 
#define SGR_FG_CYAN 	              36    //   ESC CSI 0036 m                           | Set foreground color to Cyan.    
#define SGR_FG_WHITE 	              37    //   ESC CSI 0037 m                           | Set foreground color to White.   
#define SGR_FG_DEFAULT 	              39    //   ESC CSI 0039 m                           | Set foreground color to default, ECMA-48 3rd.
#define SGR_BG_BLACK 	              40    //   ESC CSI 0040 m                           | Set background color to Black.               
#define SGR_BG_RED 	                  41    //   ESC CSI 0041 m                           | Set background color to Red.                 
#define SGR_BG_GREEN 	              42    //   ESC CSI 0042 m                           | Set background color to Green.               
#define SGR_BG_YELLOW 	              43    //   ESC CSI 0043 m                           | Set background color to Yellow.              
#define SGR_BG_BLUE 	              44    //   ESC CSI 0044 m                           | Set background color to Blue.                
#define SGR_BG_MAGENTA                45    //   ESC CSI 0045 m                           | Set background color to Magenta.             
#define SGR_BG_CYAN 	              46    //   ESC CSI 0046 m                           | Set background color to Cyan.                
#define SGR_BG_WHITE 	              47    //   ESC CSI 0047 m                           | Set background color to White.               
#define SGR_BG_DEFAULT                49    //   ESC CSI 0049 m                           | Set background color to default, ECMA-48 3rd.
#define SGR_FG_BRIGHT_BLACK           90    //   ESC CSI 0090 m                           | Set foreground color to Bright Black;   
#define SGR_FG_BRIGHT_RED 	          91    //   ESC CSI 0091 m                           | Set foreground color to Bright Red.     
#define SGR_FG_BRIGHT_GREEN           92    //   ESC CSI 0092 m                           | Set foreground color to Bright Green.   
#define SGR_FG_BRIGHT_YELLOW          93    //   ESC CSI 0093 m                           | Set foreground color to Bright Yellow.  
#define SGR_FG_BRIGHT_BLUE 	          94    //   ESC CSI 0094 m                           | Set foreground color to Bright Blue.    
#define SGR_FG_BRIGHT_MAGENTA         95    //   ESC CSI 0095 m                           | Set foreground color to Bright Magenta. 
#define SGR_FG_BRIGHT_CYAN 	          96    //   ESC CSI 0096 m                           | Set foreground color to Bright Cyan.    
#define SGR_FG_BRIGHT_WHITE           97    //   ESC CSI 0097 m                           | Set foreground color to Bright White.   
#define SGR_BG_BRIGHT_BLACK           100   //   ESC CSI 0100 m                           | Set background color to Bright Black.   
#define SGR_BG_BRIGHT_RED 	          101   //   ESC CSI 0101 m                           | Set background color to Bright Red.     
#define SGR_BG_BRIGHT_GREEN           102   //   ESC CSI 0102 m                           | Set background color to Bright Green.   
#define SGR_BG_BRIGHT_YELLOW          103   //   ESC CSI 0103 m                           | Set background color to Bright Yellow.  
#define SGR_BG_BRIGHT_BLUE            104   //   ESC CSI 0104 m                           | Set background color to Bright Blue.    
#define SGR_BG_BRIGHT_MAGENTA         105   //   ESC CSI 0105 m                           | Set background color to Bright Magenta. 
#define SGR_BG_BRIGHT_CYAN            106   //   ESC CSI 0106 m                           | Set background color to Bright Cyan.    
#define SGR_BG_BRIGHT_WHITE           107   //   ESC CSI 0107 m                           | Set background color to Bright White.   
// TODO: double check the argument value for BG_FG_DEFAULT
//      (in the xterm file, this arg has the same value as BG_BRIGHT_BLACK
//       and i think that was a typo. i'd guess it's actually 108)
// #define SGR_BG_FG_DEFAULT          100   //   ESC CSI 0100 m                           | Set foreground and background color to default.
#define SGR_COLLON_SEPARATOR          ':'   //   --                                       | These separators are here because the commnand will change based on how the arguments are separated
#define SGR_SEMICOLLON_SEPARATOR      ';'   //   --                                       | == 
//    {  SGR_FG_EXTENDED
#define SGR_FG_EXTENDED_COLOR_PREFIX   38   //   --                                       | Prefix for setting the foreground color with the 256 extended color pallete.
#define SGR_FG_EXTCLR_RGB               2   //   ESC CSI 38 : 2 : <i> : <r> : <g> : <b> m | Set foreground color using RGB values.            
#define SGR_FG_EXTCLR_INDEX             5   //   ESC CSI 38 : 5 : <i>                   m | Set foreground color to <n>, using indexed color.  
#define SGR_FG_EXTCLR_RGB_2             2   //   ESC CSI 38 ; 2 ; <r> ; <g> ; <b>       m | Set foreground color using RGB values.            
//    }
//    { SGR_BG_EXTENDED
#define SGR_BG_EXTENDED_COLOR_PREFIX   48   //   --                                       | Prefix for setting the background color with the 256 extended color pallete. 
#define SGR_BG_EXTCLR_RGB               2   //   ESC CSI 48 : 2 : <i> : <r> : <g> : <b> m | Set background color using RGB values.                                       
#define SGR_BG_EXTCLR_INDEX             5   //   ESC CSI 48 : 5 : <i>                   m | Set background color to <n>, using indexed color.                             
#define SGR_BG_EXTCLR_RGB_2             2   //   ESC CSI 48 ; 2 ; <r> ; <g> ; <b>       m | Set background color using RGB values.                                       
//    }  SGR_BG
// } SELECT_GRAPHIC_RENDITION
// { DEVICE_STATUS_REPORT (DSR)
#define DEVICE_STATUS_REPORT         'n'    //   ESC CSI  _  n 	                          | Device Status Report (DSR).                
#define DSR_STATUS                    6     //   ESC CSI  6  n                            | Status Report.                             
#define DSR_CURSOR_POSITION           5     //   ESC CSI  5  n                            | Report Cursor Position (CPR) [row;column]. 
// } DEVICE_STATUS_REPORT
#define DEC_SCROLL_TOPBOT_MARGIN 	 'r'    //   ESC CSI <n> ; <m> r                      | Set Scrolling Region [top;bottom] (default = full size of window) (DECSTBM), VT100.
#define DEC_SCROLL_LEFRIG_MARGIN     's'    //   ESC CSI <n> ; <m> s                      | Set left and right margins (DECSLRM), VT420 and up.
// { LOAD_LEDS
#define LOAD_LEDS                    'q'    //   ESC CSI __ q                             | Load LEDs (DECLL), VT100.
#define LOAD_LEDS_CLEAR_ALL           0     //   ESC CSI 00 q                             | Clear all LEDS (default).
#define LOAD_LEDS_LIGHT_NUMLOCK       1     //   ESC CSI 01 q                             | Light Num Lock.
#define LOAD_LEDS_LIGHT_CAPSLOCK      2     //   ESC CSI 02 q                             | Light Caps Lock.
#define LOAD_LEDS_LIGHT_SCROLLLOCK    3     //   ESC CSI 03 q                             | Light Scroll Lock.
#define LOAD_LEDS_OFF_NUMLOCK         21    //   ESC CSI 21 q                             | Extinguish Num Lock.
#define LOAD_LEDS_OFF_CAPSLOCK        22    //   ESC CSI 22 q                             | Extinguish Caps Lock.
#define LOAD_LEDS_OFF_SCROLLLOCK      23    //   ESC CSI 23 q                             | Extinguish Scroll Lock.
// } LOAD_LEDS
// { WINDOW_MANIPULATION (WINMAN)
#define WINDOW_MANIPULATION          't'    //   ESC CSI _______  t                       | Window manipulation (XTWINOPS), dtterm, extended by xterm.
#define WINMAN_DEICONIFY              1     //   ESC CSI 1        t                       | De-iconify window.                                     
#define WINMAN_ICONIFY                2     //   ESC CSI 2        t                       | Iconify window.                                        
#define WINMAN_MOVE                   3     //   ESC CSI 3; x ; y t                       | Move window to [x, y].                                 
#define WINMAN_RESIZE                 4     //   ESC CSI 4; h ; w t                       | Resize the window to given height and width in pixels. 
#define WINMAN_RAISE                  5     //   ESC CSI 5        t                       | Raise the window to the front of the stacking order.          
#define WINMAN_LOWER                  6     //   ESC CSI 6        t                       | Lower the window to the bottom of the stacking order.         
#define WINMAN_REFRESH                7     //   ESC CSI 7        t                       | Refresh the window.                                           
#define WINMAN_RESIZE_TEXT_AREA       8     //   ESC CSI 8        t                       | Resize the text area to given height and width in characters. 
//    { WINMAN_MAXIMIZED
#define WINMAN_MAXIMIZED_PREFIX       9     //   ESC CSI 9; _     t
#define WINMAN_RESTORE_MAXIMIZED      0     //   ESC CSI 9; 0     t                       | Restore maximized window.                                     
#define WINMAN_MAXIMIZE               1     //   ESC CSI 9; 1     t                       | Maximize window (i.e., resize to screen size).                
#define WINMAN_MAXIMIZE_VERTICAL      2     //   ESC CSI 9; 2     t                       | Maximize window vertically.                                   
#define WINMAN_MAXIMIZE_HORIZONTAL    3     //   ESC CSI 9; 3     t                       | Maximize window horizontally.                                 
//    } WINMAN_MAXIMIZED
//    { WINMAN_FULLSCREEN
#define WINMAN_FULLSCREEN_PREFIX      10    //   ESC CSI 10; _    t
#define WINMAN_FULLSCREEN_UNDO        0     //   ESC CSI 10; 0    t                       | Undo full-screen mode.                                        
#define WINMAN_FULLSCREEN_SET         1     //   ESC CSI 10; 1    t                       | Change to full-screen.                                        
#define WINMAN_FULLSCREEN_TOGGLE      2     //   ESC CSI 10; 2    t                       | Toggle full-screen.                                           
//    } WINMAN_FULLSCREEN
#define WINMAN_REPORT_STATE           11    //   ESC CSI 11       t                       | Report window state.                                          
//    { WINMAN_REPORT_POSITION 
#define WINMAN_REPORT_POS             13    //   ESC CSI 13       t                       | Report window position.                                       
#define WINMAN_REPORT_TXTAREA_POS     2     //   ESC CSI 13; 2    t                       | Report text-area position.                                    
//    } WINMAN_REPORT_POSITION
//    { WINMAN_REPORT_SIZE
#define WINMAN_REPORT_TXTAREA_SIZE    14    //   ESC CSI 14       t                       | Report text area size in pixels.                              
#define WINMAN_REPORT_SIZE            2     //   ESC CSI 14; 2    t                       | Report window size in pixels.                                 
//    } WINMAN_REPORT_SIZE
#define WINMAN_REPORT_SCREEN_SIZE     15    //   ESC CSI 15       t                       | Report size of the screen in pixels.                          
#define WINMAN_REPORT_CHARCELL_SIZE   16    //   ESC CSI 16       t                       | Report character cell size in pixels.                         
#define WINMAN_REPORT_TXTAREA_SIZE_CH 18    //   ESC CSI 18       t                       | Report the size of the text area in characters.               
#define WINMAN_REPORT_SCREEN_SIZE_CH  19    //   ESC CSI 19       t                       | Report the size of the screen in characters.                  
#define WINMAN_REPORT_ICON_LABEL      20    //   ESC CSI 20       t                       | Report window's icon label.                                   
#define WINMAN_REPORT_TITLE           21    //   ESC CSI 21       t                       | Report window's title.                                        
//    { WINMAN_SAVE
#define WINMAN_SAVE_PREFIX            22    //   ESC CSI 22; _    t
#define WINMAN_SAVE_WIN_ICON_TITLE    0     //   ESC CSI 22; 0    t                       | Save icon and window title on stack.                          
#define WINMAN_SAVE_ICON_TITLE        1     //   ESC CSI 22; 1    t                       | Save icon title on stack.                                     
#define WINMAN_SAVE_TITLE             2     //   ESC CSI 22; 2    t                       | Save window title on stack.                                   
//    } WINMAN_SAVE
//    { WINMAN_RESTORE (WINMAN_REST_*)
#define WINMAN_REST_PREFIX            23    //   ESC CSI 23; _    t
#define WINMAN_REST_WIN_ICON_TITLE    0     //   ESC CSI 23; 0    t                       | Restore icon and window title from stack.                     
#define WINMAN_REST_ICON_TITLE        1     //   ESC CSI 23; 1    t                       | Restore icon title from stack.                                
#define WINMAN_REST_TITLE             2     //   ESC CSI 23; 2    t                       | Restore window title from stack.                              
//    } WINMAN_RESTORE
#define WINMAN_RESIZE_LINES           24    //   ESC CSI 24       t                       | Resize to <n> lines (DECSLPP), VT340 and VT420.                
#define REQUEST_TERM_PARAM           'x'    //   ESC CSI x                                | Request Terminal Parameters (DECREQTPARM).
// { BEGIN FORMAT: ESC CSI <n>* SP <op>
#define CSI_SPACE_PREFIX   SP
#define SHIFT_LEFT                   '@'    //   ESC CSI <n> SP @                         | Shift left Ps columns(s) (default = 1) (SL), ECMA-48.
#define SHIFT_RIGHT                  'A'    //   ESC CSI <n> SP A                         | Shift right Ps columns(s) (default = 1) (SR), ECMA-48.
//    { SET_CURSOR_STYLE (SCS)
#define SET_CURSOR_STYLE             'q'    //   ESC CSI  _  SP q                         | Set cursor style (DECSCUSR), VT520.
#define SCS_BLINKING_BLOCK            0     //   ESC CSI  0  SP q                         | blinking block.
#define SCS_BLINKING_BLOCK_DEFAULT    1     //   ESC CSI  1  SP q                         | blinking block (default).
#define SCS_STEADY_BLOCK              2     //   ESC CSI  2  SP q                         | steady block.
#define SCS_BLINKING_UNDERLINE        3     //   ESC CSI  3  SP q                         | blinking underline.
#define SCS_STEADY_UNDERLINE          4     //   ESC CSI  4  SP q                         | steady underline.
#define SCS_BLINKING_BAR              5     //   ESC CSI  5  SP q                         | blinking bar,  xterm.
#define SCS_STEADY_BAR                6     //   ESC CSI  6  SP q                         | steady bar,  xterm.
//    } SET_CURSOR_STYLE (SCS)
//    { SET_WARNING_BELL_VOL (WBELL_VOL_*)
#define SET_WARNING_BELL_VOL         't'    //   ESC CSI  _  SP t                         | Set warning-bell volume (DECSWBV), VT520.
#define WBELL_VOL_OFF_ZERO_1          0     //   ESC CSI  0  SP t                         | off.
#define WBELL_VOL_OFF_ZERO_2          1     //   ESC CSI  1  SP t                         | off.
#define WBELL_VOL_LOW_1               2     //   ESC CSI  2  SP t                         | low.
#define WBELL_VOL_LOW_2               3     //   ESC CSI  3  SP t                         | low.
#define WBELL_VOL_LOW_3               4     //   ESC CSI  4  SP t                         | low.
#define WBELL_VOL_HIGH_1              5     //   ESC CSI  5  SP t                         | high.
#define WBELL_VOL_HIGH_2              6     //   ESC CSI  6  SP t                         | high.
#define WBELL_VOL_HIGH_3              7     //   ESC CSI  7  SP t                         | high.
#define WBELL_VOL_HIGH_4              8     //   ESC CSI  8  SP t                         | high.
//    } SET_WARNING_BELL_VOL (SWBELLV)
//    { SET_MARGIN_BELL_VOL (MBELL_VOL)
#define SET_MARGIN_BELL_VOL          'u'    //   ESC CSI  _  SP u                         | Set margin-bell volume (DECSMBV), VT520.
#define MBELL_VOL_HIGH_1              0     //   ESC CSI  0  SP u                         | high.
#define MBELL_VOL_HIGH_2              5     //   ESC CSI  5  SP u                         | high.
#define MBELL_VOL_HIGH_3              6     //   ESC CSI  6  SP u                         | high.
#define MBELL_VOL_HIGH_4              7     //   ESC CSI  7  SP u                         | high.
#define MBELL_VOL_HIGH_5              8     //   ESC CSI  8  SP u                         | high.
#define MBELL_VOL_OFF                 1     //   ESC CSI  1  SP u                         | off.
#define MBELL_VOL_ON_1                2     //   ESC CSI  2  SP u                         | low.
#define MBELL_VOL_ON_2                3     //   ESC CSI  3  SP u                         | low.
#define MBELL_VOL_ON_3                4     //   ESC CSI  4  SP u                         | low.
//    } SET_MARGIN_BELL_VOL (MBELL_VOL)
// } END FORMAT:   ESC CSI <n>* SP <op>
// { BEGIN FORMAT: ESC CSI ? <n>+ <op>
#define CSI_QUESTION_MARK_PREFIX '?'
//    { SELECTIVE_ERASE_IN_DISPLAY
#define SELECTIVE_ERASE_IN_DISPLAY       'J'    //   ESC CSI ? _  J 	                  | Selective Erase in Display (DECSED), VT220.
#define SELECTIVE_ERASE_IN_DISPLAY_BELOW  0     //   ESC CSI ? 0  J                       | Selective Erase Below (default).    
#define SELECTIVE_ERASE_IN_DISPLAY_ABOVE  1     //   ESC CSI ? 1  J                       | Selective Erase Above.              
#define SELECTIVE_ERASE_IN_DISPLAY_ALL    2     //   ESC CSI ? 2  J                       | Selective Erase All.                
#define SELECTIVE_ERASE_IN_DISPLAY_SAVED  3     //   ESC CSI ? 3  J                       | Selective Erase Saved Lines,  xterm.
//    } SELECTIVE_ERASE_IN_DISPLAY
//    { SELECTIVE_ERASE_IN_LINE
#define SELECTIVE_ERASE_IN_LINE          'K'    //   ESC CSI ? _  K                       | Erase in Line (DECSEL), VT220.      
#define SELECTIVE_ERASE_IN_LINE_RIGHT     0     //   ESC CSI ? 0  K                       | Selective Erase to Right (default). 
#define SELECTIVE_ERASE_IN_LINE_LEFT      1     //   ESC CSI ? 1  K                       | Selective Erase to Left.            
#define SELECTIVE_ERASE_IN_LINE_ALL       2     //   ESC CSI ? 2  K                       | Selective Erase All.                
//    } SELECTIVE_ERASE_IN_LINE
//    { SET_REQ_GRAPH_ATTR (SRGA) -- NOTE: I ain't sure if this is how the arguments are supposed to be used. 
#define SET_REQ_GRAPH_ATTR               'S'    //   ESC CSI ? _______   S                | Set or request graphics attribute (XTSMGRAPHICS), xterm.
#define SRGA_ITEM_NUM_COLOR_REGISTERS     1     //   ESC CSI ? 1; _; _   S                | item is number of color registers.           
#define SRGA_ITEM_SIXEL_GRAPHICS_GEOMETRY 2     //   ESC CSI ? 2; _; _   S                | item is Sixel graphics geometry (in pixels). 
#define SRGA_ITEM_REGIS_GRAPHICS_GEOMETRY 3     //   ESC CSI ? 3; _; _   S                | item is ReGIS graphics geometry (in pixels). 
#define SRGA_READ                         1     //   ESC CSI ? _; 1      S                | read attribute.                              
#define SRGA_RESET                        2     //   ESC CSI ? _; 2      S                | reset to default.                            
#define SRGA_SET                          3     //   ESC CSI ? _; 3; <n> S                | set to value in Pv.                          
#define SRGA_READ_MAX_VALUE               4     //   ESC CSI ? _; 4      S                | read the maximum allowed value.              
//    } SET_REQ_GRAPH_ATTR (SRGA)
//    { RESET_TAB_STOPS
#define RESET_TAB_STOPS                  'W'    //   ESC CSI ? 5 W                        | Reset tab stops to start with column 9, every 8 columns (DECST8C), VT510.
#define RESET_TAB_STOPS_ARG               5     //   ESC CSI ? 5 W                        | Reset tab stops to start with column 9, every 8 columns (DECST8C), VT510.
//    } RESET_TAB_STOPS
//    { QUERY_FORMAT_KEYS (QFK) NOTE: the xterm file have all the arguments from 0 to 7 repeated. Looks like a mistake, but not sure.
#define QUERY_FORMAT_KEYS                'g'    //   ESC CSI ? _ g                        | Query key modifier options (XTQFMTKEYS), xterm.
#define QFT_KEYBOARD                      0     //   ESC CSI ? 0 g                        | formatKeyboard.
#define QFT_CURSOR_KEYS                   1     //   ESC CSI ? 1 g                        | formatCursorKeys.
#define QFT_FUNTION_KEYS                  2     //   ESC CSI ? 2 g                        | formatFunctionKeys.
#define QFT_KEYPAD_KEYS                   3     //   ESC CSI ? 3 g                        | formatKeypadKeys.
#define QFT_OTHER_KEYS                    4     //   ESC CSI ? 4 g                        | formatOtherKeys.
#define QFT_MODIFIER_KEYS                 6     //   ESC CSI ? 6 g                        | formatModifierKeys.
#define QFT_SPECIAL_KEYS                  7     //   ESC CSI ? 7 g                        | formatSpecialKeys.
//    } QUERY_FORMAT_KEYS (QFK)
//    { DEC_PRIVATE_MODE_SET (DECSET)
#define DEC_PRIVATE_MODE_SET             'h'    // ESC CSI ? ____* h                      | DEC Private Mode Set (DECSET).
#define DECSET_APPLICATION_CURSOR_KEYS    1     // ESC CSI ? 0001  h                      | Application Cursor Keys (DECCKM), VT100.
#define DECSET_DESIGNATE_USASCII_G0_TO_G3 2     // ESC CSI ? 0002  h                      | Designate USASCII for character sets G0-G3 (DECANM), VT100, and set VT100 mode.
#define DECSET_COLUMN_MODE                3     // ESC CSI ? 0003  h                      | 132 Column Mode (DECCOLM), VT100.
#define DECSET_SMOOTH_SCROLL              4     // ESC CSI ? 0004  h                      | Smooth (Slow) Scroll (DECSCLM), VT100.
#define DECSET_REVERSE_VIDEO              5     // ESC CSI ? 0005  h                      | Reverse Video (DECSCNM), VT100.
#define DECSET_ORIGIN_MODE                6     // ESC CSI ? 0006  h                      | Origin Mode (DECOM), VT100.
#define DECSET_AUTO_WRAP_MODE             7     // ESC CSI ? 0007  h                      | Auto-Wrap Mode (DECAWM), VT100.
#define DECSET_AUTO_REPEAT_KEYS           8     // ESC CSI ? 0008  h                      | Auto-Repeat Keys (DECARM), VT100.
#define DECSET_SEND_MOUSE_X_Y_ON_BUTPRESS 9     // ESC CSI ? 0009  h                      | Send Mouse X  amp Y on button press.
#define DECSET_SHOW_TOOLBAR               10    // ESC CSI ? 0010  h                      | Show toolbar (rxvt).
#define DECSET_START_BLINKING_CURSOR      12    // ESC CSI ? 0012  h                      | Start blinking cursor (AT ampT 610).
#define DECSET_START_BLINKING_CURSOR_2    13    // ESC CSI ? 0013  h                      | Start blinking cursor (set only via resource or menu).
#define DECSET_XOR_BLINKING_CURSOR        14    // ESC CSI ? 0014  h                      | Enable XOR of blinking cursor control sequence and menu.
#define DECSET_PRINT_FORM_FEED            18    // ESC CSI ? 0018  h                      | Print Form Feed (DECPFF), VT220.
#define DECSET_SET_PRINT_EXT_FULLSCREEN   19    // ESC CSI ? 0019  h                      | Set print extent to full screen (DECPEX), VT220.
#define DECSET_SHOW_CURSOR                25    // ESC CSI ? 0025  h                      | Show cursor (DECTCEM), VT220.
#define DECSET_SHOW_SCROLLBAR             30    // ESC CSI ? 0030  h                      | Show scrollbar (rxvt).
#define DECSET_ENB_FONT_SHIFTING          35    // ESC CSI ? 0035  h                      | Enable font-shifting functions (rxvt).
#define DECSET_ENTER_TEKTRONIX_MODE       38    // ESC CSI ? 0038  h                      | Enter Tektronix mode (DECTEK), VT240,  xterm.
#define DECSET_132_MODE                   40    // ESC CSI ? 0040  h                      | 132 mode,  xterm.
#define DECSET_MORE_FIX                   41    // ESC CSI ? 0041  h                      | more(1) fix (see curses resource).
#define DECSET_ENB_NAT_REPLACE_CHSET      42    // ESC CSI ? 0042  h                      | Enable National Replacement Character sets (DECNRCM), VT220.
#define DECSET_ENB_GRAP_EXP_PRINT_MODE    43    // ESC CSI ? 0043  h                      | Enable Graphic Expanded Print Mode (DECGEPM), VT340.
#define DECSET_TURN_ON_MARGIN_BELL        44    // ESC CSI ? 0044  h                      | Turn on margin bell,  xterm.
#define DECSET_ENB_GRAP_PRINT_CLR_MODE    44    // ESC CSI ? 0044  h                      | Enable Graphic Print Color Mode (DECGPCM), VT340.
#define DECSET_REVERSE_WRAP_MODE          45    // ESC CSI ? 0045  h                      | Reverse-wraparound mode (XTREVWRAP),  xterm.
#define DECSET_ENB_GRAP_PRINT_CLR_SYNTAX  45    // ESC CSI ? 0045  h                      | Enable Graphic Print Color Syntax (DECGPCS), VT340.
#define DECSET_START_LOGGING              46    // ESC CSI ? 0046  h                      | Start logging (XTLOGGING),  xterm.
#define DECSET_GRAP_PRINT_BG_MODE         46    // ESC CSI ? 0046  h                      | Graphic Print Background Mode, VT340.
#define DECSET_USE_ALTERNATE_SCREEN_BUF   47    // ESC CSI ? 0047  h                      | Use Alternate Screen Buffer,  xterm.
#define DECSET_ENB_GRAP_ROT_PRINT_MODE    47    // ESC CSI ? 0047  h                      | Enable Graphic Rotated Print Mode (DECGRPM), VT340.
#define DECSET_APPLICATION_KEYPAD_MODE    66    // ESC CSI ? 0066  h                      | Application keypad mode (DECNKM), VT320.
#define DECSET_BACKARROW_SENDS_BACKSPACE  67    // ESC CSI ? 0067  h                      | Backarrow key sends backspace (DECBKM), VT340, VT420.
#define DECSET_LEFT_RIGHT_MARGIN_MODE     69    // ESC CSI ? 0069  h                      | Enable left and right margin mode (DECLRMM), VT420 and up.
#define DECSET_ENB_SIXEL_DISPLAY_MODE     80    // ESC CSI ? 0080  h                      | Enable Sixel Display Mode (DECSDM), VT330, VT340, VT382.
#define DECSET_NOTCLEAR_SCREEN_ON_DECCOLM 95    // ESC CSI ? 0095  h                      | Do not clear screen when DECCOLM is set/reset (DECNCSM), VT510 and up.
#define DECSET_SEND_MOUXY_ON_BUTPRESSRELS 1000  // ESC CSI ? 1000  h                      | Send Mouse X  amp Y on button press and release.
#define DECSET_HILITE_MOUSE_TRACKING      1001  // ESC CSI ? 1001  h                      | Use Hilite Mouse Tracking,  xterm.
#define DECSET_CELL_MOTION_MOUSE_TRACKING 1002  // ESC CSI ? 1002  h                      | Use Cell Motion Mouse Tracking,  xterm.
#define DECSET_ALL_MOTION_MOUSE_TRACKING  1003  // ESC CSI ? 1003  h                      | Use All Motion Mouse Tracking,  xterm.
#define DECSET_SEND_FOCUS_INOUT_EVENTS    1004  // ESC CSI ? 1004  h                      | Send FocusIn/FocusOut events,  xterm.  Enable reporting focus. Reports whenever terminal emulator enters or exits focus as ESC [I and ESC [O, respectively. 
#define DECSET_UTF8_MOUSE_MODE            1005  // ESC CSI ? 1005  h                      | Enable UTF-8 Mouse Mode,  xterm.
#define DECSET_SGR_MOUSE_MODE             1006  // ESC CSI ? 1006  h                      | Enable SGR Mouse Mode,  xterm.
#define DECSET_ALTERNATE_SCROLL_MODE      1007  // ESC CSI ? 1007  h                      | Enable Alternate Scroll Mode,  xterm.
#define DECSET_SCROLL_BOTTOM_TTY_OUTPUT   1010  // ESC CSI ? 1010  h                      | Scroll to bottom on tty output (rxvt).
#define DECSET_SCROLL_BOTTOM_ON_KEYPRESS  1011  // ESC CSI ? 1011  h                      | Scroll to bottom on key press (rxvt).
#define DECSET_ENB_FASTSCROLL             1014  // ESC CSI ? 1014  h                      | Enable \fB\%fastScroll\fP resource,  xterm.
#define DECSET_ENB_URXVT_MOUSE_MODE       1015  // ESC CSI ? 1015  h                      | Enable urxvt Mouse Mode.
#define DECSET_ENB_SGR_MOUSE_PIXELMODE    1016  // ESC CSI ? 1016  h                      | Enable SGR Mouse PixelMode,  xterm.
#define DECSET_INTERPRET_META_KEY         1034  // ESC CSI ? 1034  h                      | Interpret “meta” key,  xterm.
#define DECSET_ENB_SPCMOD_ALT_NUMLOCK     1035  // ESC CSI ? 1035  h                      | Enable special modifiers for Alt and NumLock keys,  xterm.
#define DECSET_SEND_ESC_WHEN_META_MOD     1036  // ESC CSI ? 1036  h                      | Send ESC when Meta modifies a key,  xterm.
#define DECSET_SEND_DEL_FROM_EDITKEYPAD   1037  // ESC CSI ? 1037  h                      | Send DEL from the editing-keypad Delete key,  xterm.
#define DECSET_SEND_ESC_WHEN_ALT_MOD      1039  // ESC CSI ? 1039  h                      | Send ESC when Alt modifies a key,  xterm.
#define DECSET_KEEP_SELEC_NOT_HIGLIG      1040  // ESC CSI ? 1040  h                      | Keep selection even if not highlighted,  xterm.
#define DECSET_USE_CLIPBOARD_SELECTION    1041  // ESC CSI ? 1041  h                      | Use the CLIPBOARD selection,  xterm.
#define DECSET_ENB_URGWIN_ON_CTRL_G       1042  // ESC CSI ? 1042  h                      | Enable Urgency window manager hint when Control-G is received,  xterm.
#define DECSET_ENB_RAISEWIN_ON_CTRL_G     1043  // ESC CSI ? 1043  h                      | Enable raising of the window when Control-G is received,  xterm.
#define DECSET_REUSE_MOST_RECENT_CLIPBRD  1044  // ESC CSI ? 1044  h                      | Reuse the most recent data copied to CLIPBOARD,  xterm.
#define DECSET_EXTENDED_REVERSE_WRAP_MODE 1045  // ESC CSI ? 1045  h                      | Extended Reverse-wraparound mode (XTREVWRAP2),  xterm.
#define DECSET_ENB_SWAP_ALT_SCREEN_BUF    1046  // ESC CSI ? 1046  h                      | Enable switching to/from Alternate Screen Buffer,  xterm.
#define DECSET_USE_ALT_SCREEN_BUFFER      1047  // ESC CSI ? 1047  h                      | Use Alternate Screen Buffer,  xterm.
#define DECSET_SAVE_CURSOR                1048  // ESC CSI ? 1048  h                      | Save cursor as in DECSC,  xterm.
#define DECSET_SAVE_CURSOR_2              1049  // ESC CSI ? 1049  h                      | Save cursor as in DECSC,  xterm.
#define DECSET_TERMINFOCAP_FNKEY_MODE     1050  // ESC CSI ? 1050  h                      | Set terminfo/termcap function-key mode,  xterm.
#define DECSET_SET_SUN_FUNCKEY_MODE       1051  // ESC CSI ? 1051  h                      | Set Sun function-key mode,  xterm.
#define DECSET_SET_HP_FUNCKEY_MODE        1052  // ESC CSI ? 1052  h                      | Set HP function-key mode,  xterm.
#define DECSET_SET_SCO_FUNCKEY_MODE       1053  // ESC CSI ? 1053  h                      | Set SCO function-key mode,  xterm.
#define DECSET_SET_LEGACY_KEYBOARD_EMUL   1060  // ESC CSI ? 1060  h                      | Set legacy keyboard emulation, i.e, X11R6,  xterm.
#define DECSET_SET_VT220_KEYBOARD_EMUL    1061  // ESC CSI ? 1061  h                      | Set VT220 keyboard emulation,  xterm.
#define DECSET_SET_READLINE_MOUSEBUT_1    2001  // ESC CSI ? 2001  h                      | Enable readline mouse button-1,  xterm.
#define DECSET_SET_READLINE_MOUSEBUT_2    2002  // ESC CSI ? 2002  h                      | Enable readline mouse button-2,  xterm.
#define DECSET_SET_READLINE_MOUSEBUT_3    2003  // ESC CSI ? 2003  h                      | Enable readline mouse button-3,  xterm.
#define DECSET_SET_BRACKETED_PASTE_MODE   2004  // ESC CSI ? 2004  h                      | Set bracketed paste mode,  xterm.
#define DECSET_ENB_READLINE_CHARQUOTE     2005  // ESC CSI ? 2005  h                      | Enable readline character-quoting,  xterm.
#define DECSET_ENB_READLINE_NEWLINE_PASTE 2006  // ESC CSI ? 2006  h                      | Enable readline newline pasting,  xterm.
//    } DEC_PRIVATE_MODE_SET (DECSET)
//    { DEC_MEDIA_COPY (DEC_MC)
#define DEC_MEDIA_COPY                    'i'   // ESC CSI ? __ i                         | Media Copy (MC), DEC-specific.
#define DEC_MC_PRINT_CURSOR_LINE           1    // ESC CSI ? 01 i                         | Print line containing cursor.
#define DEC_MC_AUTOPRINT_OFF               4    // ESC CSI ? 04 i                         | Turn off autoprint mode.
#define DEC_MC_AUTOPRINT_ON                5    // ESC CSI ? 05 i                         | Turn on autoprint mode.
#define DEC_MC_PRINT_COMPOSED_DISPLAY      10   // ESC CSI ? 10 i                         | Print composed display, ignores DECPEX.
#define DEC_MC_PRINT_ALL_PAGES             11   // ESC CSI ? 11 i                         | Print all pages.
//    } DEC_MEDIA_COPY (DEC_MC)
//    { DEC_PRIVATE_MODE_RESET (DECRST)
#define DEC_PRIVATE_MODE_RESET            'l'   // ESC CSI ? ____ l                       | DEC Private Mode Reset (DECRST).
#define DECRST_NORMAL_CURSOR_KEYS          1    // ESC CSI ? 0001 l                       | Normal Cursor Keys (DECCKM), VT100.
#define DECRST_VT52_MODE                   2    // ESC CSI ? 0002 l                       | Designate VT52 mode (DECANM), VT100.
#define DECRST_80_COLUMN_MODE              3    // ESC CSI ? 0003 l                       | 80 Column Mode (DECCOLM), VT100.
#define DECRST_JUMP_FAST_SCROLL            4    // ESC CSI ? 0004 l                       | Jump (Fast) Scroll (DECSCLM), VT100.
#define DECRST_NORMAL_VIDEO                5    // ESC CSI ? 0005 l                       | Normal Video (DECSCNM), VT100.
#define DECRST_NORMAL_CURSOR_MODE          6    // ESC CSI ? 0006 l                       | Normal Cursor Mode (DECOM), VT100.
#define DECRST_NO_AUTOWRAP_MODE            7    // ESC CSI ? 0007 l                       | No Auto-Wrap Mode (DECAWM), VT100.
#define DECRST_NO_AUTOREPEAT_MODE          8    // ESC CSI ? 0008 l                       | No Auto-Repeat Keys (DECARM), VT100.
#define DECRST_NO_SEND_MOUSE_XY_ON_BUTPRS  9    // ESC CSI ? 0009 l                       | Don't send Mouse X  amp Y on button press,  xterm.
#define DECRST_HIDE_TOOLBAR                10   // ESC CSI ? 0010 l                       | Hide toolbar (rxvt).
#define DECRST_STOP_BLINKING_CURSOR        12   // ESC CSI ? 0012 l                       | Stop blinking cursor (AT ampT 610).
#define DECRST_STOP_BLINKING_CURSOR_2      13   // ESC CSI ? 0013 l                       | Disable blinking cursor (reset only via resource or menu).
#define DECRST_DSB_XOR_BLINKING_CURSOR     14   // ESC CSI ? 0014 l                       | Disable XOR of blinking cursor control sequence and menu.
#define DECRST_NOT_PRINT_FORM_FEED         18   // ESC CSI ? 0018 l                       | Don't Print Form Feed (DECPFF), VT220.
#define DECRST_SET_PRINT_SCROLL_REGION     19   // ESC CSI ? 0019 l                       | Limit print to scrolling region (DECPEX), VT220.
#define DECRST_HIDE_CURSOR                 25   // ESC CSI ? 0025 l                       | Hide cursor (DECTCEM), VT220.
#define DECRST_HIDE_SCROLLBAR              30   // ESC CSI ? 0030 l                       | Don't show scrollbar (rxvt).
#define DECRST_DSB_FONT_SHIFTING           35   // ESC CSI ? 0035 l                       | Disable font-shifting functions (rxvt).
#define DECRST_132_MODE                    40   // ESC CSI ? 0040 l                       | 132 mode,  xterm.
#define DECRST_NO_MORE_FIX                 41   // ESC CSI ? 0041 l                       | No more(1) fix (see curses resource).
#define DECRST_DSB_NAT_REPLACE_CHSET       42   // ESC CSI ? 0042 l                       | Disable National Replacement Character sets (DECNRCM), VT220.
#define DECRST_DSB_GRAP_EXP_PRINT_MODE     43   // ESC CSI ? 0043 l                       | Disable Graphic Expanded Print Mode (DECGEPM), VT340.
#define DECRST_TURN_OFF_MARGIN_BELL        44   // ESC CSI ? 0044 l                       | Turn off margin bell,  xterm.
#define DECRST_DSB_GRAP_PRINT_CLR_MODE     44   // ESC CSI ? 0044 l                       | Disable Graphic Print Color Mode (DECGPCM), VT340.
#define DECRST_NO_REVERSE_WRAP_MODE        45   // ESC CSI ? 0045 l                       | No Reverse-wraparound mode (XTREVWRAP),  xterm.
#define DECRST_DSB_GRAP_PRINT_CLR_SYNTAX   45   // ESC CSI ? 0045 l                       | Disable Graphic Print Color Syntax (DECGPCS), VT340.
#define DECRST_STOP_LOGGING                46   // ESC CSI ? 0046 l                       | Stop logging (XTLOGGING),  xterm.
#define DECRST_USE_NORMAL_SCREEN_BUF       47   // ESC CSI ? 0047 l                       | Use Normal Screen Buffer,  xterm.
#define DECRST_DSB_GRAP_ROT_PRINT_MODE     47   // ESC CSI ? 0047 l                       | Disable Graphic Rotated Print Mode (DECGRPM), VT340.
#define DECRST_NUMERIC_KEYPAD_MODE         66   // ESC CSI ? 0066 l                       | Numeric keypad mode (DECNKM), VT320.
#define DECRST_BACKARROW_SENDS_DELETE      67   // ESC CSI ? 0067 l                       | Backarrow key sends delete (DECBKM), VT340, VT420.
#define DECRST_DSB_LEFT_RIGHT_MARGIN_MODE  69   // ESC CSI ? 0069 l                       | Disable left and right margin mode (DECLRMM), VT420 and up.
#define DECRST_DSB_SIXEL_DISPLAY_MODE      80   // ESC CSI ? 0080 l                       | Disable Sixel Display Mode (DECSDM), VT330, VT340, VT382.
#define DECRST_CLEAR_SCREEN_ON_DECCOLM     95   // ESC CSI ? 0095 l                       | Clear screen when DECCOLM is set/reset (DECNCSM), VT510 and up.
#define DECRST_NOT_SND_MXY_ON_BUTPRESSRELS 1000 // ESC CSI ? 1000 l                       | Don't send Mouse X  amp Y on button press and release.
#define DECRST_NOT_HILITE_MOUSE_TRACKING   1001 // ESC CSI ? 1001 l                       | Don't use Hilite Mouse Tracking,  xterm.
#define DECRST_NOT_CELL_MOTION_MOUSE_TRCK  1002 // ESC CSI ? 1002 l                       | Don't use Cell Motion Mouse Tracking,  xterm.
#define DECRST_NOT_ALL_MOTION_MOUSE_TRCK   1003 // ESC CSI ? 1003 l                       | Don't use All Motion Mouse Tracking,  xterm.
#define DECRST_NOT_SEND_FOCUS_INOUT_EVTS   1004 // ESC CSI ? 1004 l                       | Don't send FocusIn/FocusOut events,  xterm.
#define DECRST_DSB_UTF8_MOUSE_MODE         1005 // ESC CSI ? 1005 l                       | Disable UTF-8 Mouse Mode,  xterm.
#define DECRST_DSB_SGR_MOUSE_MODE          1006 // ESC CSI ? 1006 l                       | Disable SGR Mouse Mode,  xterm.
#define DECRST_DSB_ALTERNATE_SCROLL_MODE   1007 // ESC CSI ? 1007 l                       | Disable Alternate Scroll Mode,  xterm.
#define DECRST_NOT_SCRL_BOTTOM_TTY_OUTPUT  1010 // ESC CSI ? 1010 l                       | Don't scroll to bottom on tty output (rxvt).
#define DECRST_NOT_SCRL_BOTTOM_ON_KEYPRESS 1011 // ESC CSI ? 1011 l                       | Don't scroll to bottom on key press (rxvt).
#define DECRST_DSB_FASTSCROLL              1014 // ESC CSI ? 1014 l                       | Disable \fB\%fastScroll\fP resource,  xterm.
#define DECRST_DSB_URXVT_MOUSE_MODE        1015 // ESC CSI ? 1015 l                       | Disable urxvt Mouse Mode.
#define DECRST_DSB_SGR_MOUSE_PIXELMODE     1016 // ESC CSI ? 1016 l                       | Disable SGR Mouse Pixel-Mode,  xterm.
#define DECRST_NOT_INTERPRET_META_KEY      1034 // ESC CSI ? 1034 l                       | Don't interpret “meta” key,  xterm.
#define DECRST_DSB_SPCMOD_ALT_NUMLOCK      1035 // ESC CSI ? 1035 l                       | Disable special modifiers for Alt and NumLock keys,  xterm.
#define DECRST_NOT_SEND_ESC_WHEN_META_MOD  1036 // ESC CSI ? 1036 l                       | Don't send ESC when Meta modifies a key,  xterm.
#define DECRST_SEND_VT220REMV_EDIT_KEYPAD  1037 // ESC CSI ? 1037 l                       | Send VT220 Remove from the editing-keypad Delete key,  xterm.
#define DECRST_NOT_SEND_ESC_WHEN_ALT_MOD   1039 // ESC CSI ? 1039 l                       | Don't send ESC when Alt modifies a key,  xterm.
#define DECRST_NOT_KEEP_SELEC_NOT_HIGLIG   1040 // ESC CSI ? 1040 l                       | Do not keep selection when not highlighted,  xterm.
#define DECRST_USE_PRIMARY_SELECTION       1041 // ESC CSI ? 1041 l                       | Use the PRIMARY selection,  xterm.
#define DECRST_DSB_URGWIN_ON_CTRL_G        1042 // ESC CSI ? 1042 l                       | Disable Urgency window manager hint when Control-G is received,  xterm.
#define DECRST_DSB_RAISEWIN_ON_CTRL_G      1043 // ESC CSI ? 1043 l                       | Disable raising of the window when Control-G is received,  xterm.
#define DECRST_NOT_EXTNDD_REVS_WRAP_MODE   1045 // ESC CSI ? 1045 l                       | No Extended Reverse-wraparound mode (XTREVWRAP2),  xterm.
#define DECRST_DSB_SWAP_ALT_SCREEN_BUF     1046 // ESC CSI ? 1046 l                       | Disable switching to/from Alternate Screen Buffer,  xterm.
#define DECRST_USE_NORMAL_SCREEN_BUFFER    1047 // ESC CSI ? 1047 l                       | Use Normal Screen Buffer,  xterm.
#define DECRST_RESTORE_CURSOR              1048 // ESC CSI ? 1048 l                       | Restore cursor as in DECRC,  xterm.
#define DECRST_NORMAL_SCBUF_REST_CURSOR    1049 // ESC CSI ? 1049 l                       | Use Normal Screen Buffer and restore cursor as in DECRC,  xterm.
#define DECRST_TERMINFOCAP_FNKEY_MODE      1050 // ESC CSI ? 1050 l                       | Reset terminfo/termcap function-key mode,  xterm.
#define DECRST_SET_SUN_FUNCKEY_MODE        1051 // ESC CSI ? 1051 l                       | Reset Sun function-key mode,  xterm.
#define DECRST_SET_HP_FUNCKEY_MODE         1052 // ESC CSI ? 1052 l                       | Reset HP function-key mode,  xterm.
#define DECRST_SET_SCO_FUNCKEY_MODE        1053 // ESC CSI ? 1053 l                       | Reset SCO function-key mode,  xterm.
#define DECRST_SET_LEGACY_KEYBOARD_EMUL    1060 // ESC CSI ? 1060 l                       | Reset legacy keyboard emulation, i.e, X11R6,  xterm.
#define DECRST_RST_TO_SUNPC_KEYBOARD_EMUL  1061 // ESC CSI ? 1061 l                       | Reset keyboard emulation to Sun/PC style,  xterm.
#define DECRST_DSB_READLINE_MOUSEBUT_1     2001 // ESC CSI ? 2001 l                       | Disable readline mouse button-1,  xterm.
#define DECRST_DSB_READLINE_MOUSEBUT_2     2002 // ESC CSI ? 2002 l                       | Disable readline mouse button-2,  xterm.
#define DECRST_DSB_READLINE_MOUSEBUT_3     2003 // ESC CSI ? 2003 l                       | Disable readline mouse button-3,  xterm.
#define DECRST_RST_BRACKETED_PASTE_MODE    2004 // ESC CSI ? 2004 l                       | Reset bracketed paste mode,  xterm.
#define DECRST_DSB_READLINE_CHARQUOTE      2005 // ESC CSI ? 2005 l                       | Disable readline character-quoting,  xterm.
#define DECRST_DSB_READLINE_NEWLINE_PASTE  2006 // ESC CSI ? 2006 l                       | Disable readline newline pasting,  xterm.
//    } DEC_PRIVATE_MODE_RESET (DECRST)
//    { QUERY_MODIFIER_KEYS (QMK) 
#define QUERY_MODIFIER_KEYS                'm'  // ESC CSI ? _ m                          | Query key modifier options (XTQMODKEYS), xterm.
#define QMK_MOD_KEYBOARD                    0   // ESC CSI ? 0 m                          | modifyKeyboard.
#define QMK_CURSOR_KEYS                     1   // ESC CSI ? 1 m                          | modifyCursorKeys.
#define QMK_FUNCTION_KEYS                   2   // ESC CSI ? 2 m                          | modifyFunctionKeys.
#define QMK_KEYPAD_KEYS                     3   // ESC CSI ? 3 m                          | modifyKeypadKeys.
#define QMK_OTHER_KEYS                      4   // ESC CSI ? 4 m                          | modifyOtherKeys.
#define QMK_MODIFIER_KEYS                   6   // ESC CSI ? 6 m                          | modifyModifierKeys.
#define QMK_SPECIAL_KEYS                    7   // ESC CSI ? 7 m                          | modifySpecialKeys.
//    } QUERY_MODIFIER_KEYS (QMK) 
//    { DEC_DEVICE_STATUS_REPORT (DEC_DSR)
#define DEC_DEVICE_STATUS_REPORT           'n'  // ESC CSI ? __ n                         | Device Status Report (DSR, DEC-specific).
#define DEC_DSR_CURSOR_POSITION             6   // ESC CSI ? __ n                         | Report Cursor Position (DECXCPR).
#define DEC_DSR_PRINTER_STATUS              15  // ESC CSI ? __ n                         | Report Printer status.
#define DEC_DSR_UDK_STATUS                  25  // ESC CSI ? __ n                         | Report UDK status.
#define DEC_DSR_KEYBOARD_STATUS             26  // ESC CSI ? __ n                         | Report Keyboard status.
#define DEC_DSR_LOCATOR_STATUS              55  // ESC CSI ? __ n                         | Report Locator status.
#define DEC_DSR_LOCATOR_TYPE                56  // ESC CSI ? __ n                         | Report Locator type.
#define DEC_DSR_MACRO_SPACE                 62  // ESC CSI ? __ n                         | Report macro space (DECMSR).
#define DEC_DSR_MEMORY_CHECKSUM             63  // ESC CSI ? __ n                         | Report memory checksum (DECCKSR), VT420 and up.
#define DEC_DSR_DATA_INTEGRITY              75  // ESC CSI ? __ n                         | Report data integrity.
#define DEC_DSR_MUTI_SESSION_CONFIG         85  // ESC CSI ? __ n                         | Report multi-session configuration.
//    } DEC_DEVICE_STATUS_REPORT (DEC_DSR)
#define DEC_REQUEST_PRIVATE_MODE_PREFIX    '$'  // ESC CSI ? <n> $ _                      | Request DEC private mode (DECRQM).
#define DEC_REQUEST_PRIVATE_MODE           'p'  // ESC CSI ? <n> $ p                      | Request DEC private mode (DECRQM).
#define DEC_RESTORE_PRIVATE_MODE_VALUES    'r'  // ESC CSI ? <n> r                        | Restore DEC Private Mode Values (XTRESTORE), xterm.
#define DEC_SAVE_PRIVATE_MODE_VALUES       's'  // ESC CSI ? <n> s                        | Save DEC Private Mode Values (XTSAVE), xterm.
// } END FORMAT: ESC CSI ? <n>+ <op>
// { BEGIN FORMAT: ESC CSI <n>* # <op>
#define CSI_HASH_PREFIX                    '#'  // ESC CSI # _* _
#define PUSH_CURDYN_ANSI_COLORS            'P'  // ESC CSI # __ P                         | Push current dynamic- and ANSI-palette colors onto stack (XTPUSHCOLORS), xterm.
#define POP_DYN_ANSI_COLORS                'Q'  // ESC CSI # __ Q                         | Pop stack to set dynamic- and ANSI-palette colors (XTPOPCOLORS), xterm.
#define REPORT_CUR_ANSI_COLORS             'R'  // ESC CSI # __ R                         | Report the current entry on the palette stack, and the number of palettes stored on the stack, using the same form as XTPOPCOLOR (default = 0) (XTREPORTCOLORS), xterm.
#define REPORT_POS_TITLE_STACK             'S'  // ESC CSI # __ S                         | Report position on title-stack (XTTITLEPOS), xterm.
#define PUSH_VIDEO_ATTRS                   'p'  // ESC CSI # __ p                         | Push video attributes onto stack (XTPUSHSGR), xterm.
#define POP_VIDEO_ATTRS                    'q'  // ESC CSI # __ q                         | Pop video attributes from stack (XTPOPSGR), xterm.
//    { PUSH_VIDEO_ATTRS_2 (PUSH_VA2)
#define PUSH_VIDEO_ATTRS_2                 '{'  // ESC CSI # __ {                         | Push video attributes onto stack (XTPUSHSGR), xterm. } // close bracket so that it doesn't mess with the brackets for grouping commands and formats  }
#define PUSH_VA2_BOLD                       1   // ESC CSI # 01 {                         | Bold.                                                }
#define PUSH_VA2_FAINT                      2   // ESC CSI # 02 {                         | Faint.                                               }
#define PUSH_VA2_ITALIC                     3   // ESC CSI # 03 {                         | Italicized.                                          }
#define PUSH_VA2_UNDERLINE                  4   // ESC CSI # 04 {                         | Underlined.                                          }
#define PUSH_VA2_BLINK                      5   // ESC CSI # 05 {                         | Blink.                                               }
#define PUSH_VA2_INVERSE                    7   // ESC CSI # 07 {                         | Inverse.                                             }
#define PUSH_VA2_INVISIBLE                  8   // ESC CSI # 08 {                         | Invisible.                                           }
#define PUSH_VA2_CROSSED_OUT                9   // ESC CSI # 09 {                         | Crossed-out characters.                              }
#define PUSH_VA2_DOUBLE_UNDERLINE           21  // ESC CSI # 21 {                         | Doubly-underlined.                                   }
#define PUSH_VA2_FG_COLOR                   30  // ESC CSI # 30 {                         | Foreground color.                                    }
#define PUSH_VA2_BF_COLOR                   31  // ESC CSI # 31 {                         | Background color.                                    }
//    } PUSH_VIDEO_ATTRS_2 (PUSH_VA2)                                                                                                            {{
#define POP_VIDEO_ATTRS_2                  '}'  // ESC CSI #    }                         | Pop video attributes from stack (XTPOPSGR), xterm.   
#define SELECT_CHECKSUM_EXTENSION          'y'  // ESC CSI <n> # y                        | Select checksum extension (XTCHECKSUM), xterm.
#define REPORT_SELECT_GRAPHIC_RENDITION    '|'  // ESC CSI ___ # |                        | Report selected graphic rendition (XTREPORTSGR), xterm.
// } END FORMAT: ESC CSI <n>* # <op>
// { BEGIN FORMAT: ESC CSI > <n>* <op>
#define CSI_GT_PREFIX '>'
//    { RESET_TITLE_MODE_FEATURES (RST_TIT)
#define RESET_TITLE_MODE_FEATURES          'T'  // ESC CSI > _ T                           | Reset title mode features to default value (XTRMTITLE), xterm.
#define RST_TIT_NOTSET_WINICON_LABELS_HEX   0   // ESC CSI > 0 T                           | Do not set window/icon labels using hexadecimal.
#define RST_TIT_NOTQRY_WINICON_LABELS_HEX   1   // ESC CSI > 1 T                           | Do not query window/icon labels using hexadecimal.
#define RST_TIT_NOTSET_WINICON_LABELS_UTF8  2   // ESC CSI > 2 T                           | Do not set window/icon labels using UTF-8.
#define RST_TIT_NOTQRY_WINICON_LABELS_UTF8  3   // ESC CSI > 3 T                           | Do not query window/icon labels using UTF-8.
//    } RESET_TITLE_MODE_FEATURES (RST_TIT)
//    { SEND_DEVICE_ATTRIBUTES_SECONDARY (SDATR2)
#define SEND_DEVICE_ATTRIBUTES_SECONDARY   'c'  // ESC CSI > _  c                          | Send Device Attributes (Secondary DA).
#define SDATR2_TERM_ID_CODE                 0   // ESC CSI > __ c                          | or omitted  request the terminal's identification code.
#define SDATR2_VT100                        0   // ESC CSI > __ c                          | “VT100”.
#define SDATR2_VT220                        1   // ESC CSI > __ c                          | “VT220”.
#define SDATR2_VT240                        2   // ESC CSI > __ c                          | “VT240” or “VT241”.
#define SDATR2_VT330                        18  // ESC CSI > __ c                          | “VT330”.
#define SDATR2_VT340                        19  // ESC CSI > __ c                          | “VT340”.
#define SDATR2_VT320                        24  // ESC CSI > __ c                          | “VT320”.
#define SDATR2_VT382                        32  // ESC CSI > __ c                          | “VT382”.
#define SDATR2_VT420                        41  // ESC CSI > __ c                          | “VT420”.
#define SDATR2_VT510                        61  // ESC CSI > __ c                          | “VT510”.
#define SDATR2_VT520                        64  // ESC CSI > __ c                          | “VT520”.
#define SDATR2_VT525                        65  // ESC CSI > __ c                          | “VT525”.
//    } SEND_DEVICE_ATTRIBUTES (SDATR)
//    { SETRST_FORMAT_KEY_OPTIONS (STRST_FMTKEYS)
#define SETRST_FORMAT_KEY_OPTIONS          'f'  // ESC CSI > _ f                           | Set/reset key format options (XTFMTKEYS), xterm.
#define STRST_FMTKEYS_KEYBOARD              0   // ESC CSI > 0 f                           | formatKeyboard.
#define STRST_FMTKEYS_CURSOR_KEYS           1   // ESC CSI > 1 f                           | formatCursorKeys.
#define STRST_FMTKEYS_FUNCTION_KEYS         2   // ESC CSI > 2 f                           | formatFunctionKeys.
#define STRST_FMTKEYS_KEYPAD_KEYS           3   // ESC CSI > 3 f                           | formatKeypadKeys.
#define STRST_FMTKEYS_OTHER_KEYS            4   // ESC CSI > 4 f                           | formatOtherKeys.
#define STRST_FMTKEYS_MODIFIER_KEYS         6   // ESC CSI > 6 f                           | formatModifierKeys.
#define STRST_FMTKEYS_SPECIAL_KEYS          7   // ESC CSI > 7 f                           | formatSpecialKeys.
//    } SETRST_FORMAT_KEY_OPTIONS (STRST_FMTKEYS)
//    { SETRST_MODIFIER_KEY_OPTIONS (STRST_MODKEYS)
#define SETRST_MODIFIER_KEY_OPTIONS        'm'  // ESC CSI > _ m                           | Set/reset key modifier options (XTMODKEYS), xterm.
#define STRST_MODKEYS_KEY_OPTIONS           0   // ESC CSI > 0 m                           | modifyKeyboard.
#define STRST_MODKEYS_KEYBOARD              1   // ESC CSI > 1 m                           | modifyCursorKeys.
#define STRST_MODKEYS_CURSOR_KEYS           2   // ESC CSI > 2 m                           | modifyFunctionKeys.
#define STRST_MODKEYS_FUNCTION_KEYS         3   // ESC CSI > 3 m                           | modifyKeypadKeys.
#define STRST_MODKEYS_KEYPAD_KEYS           4   // ESC CSI > 4 m                           | modifyOtherKeys.
#define STRST_MODKEYS_OTHER_KEYS            6   // ESC CSI > 6 m                           | modifyModifierKeys.
#define STRST_MODKEYS_MODIFIER_KEYS         7   // ESC CSI > 7 m                           | modifySpecialKeys.
//    } SETRST_MODIFIER_KEY_OPTIONS (STRST_MODKEYS)
//    { DISABLE_KEY_MODIFIER_OPTIONS  (DSB_MODKEYS)
#define DISABLE_KEY_MODIFIER_OPTIONS       'n'  // ESC CSI > _ n                           | Disable key modifier options, xterm.
#define DSB_MODKEYS_KEY_OPTIONS             0   // ESC CSI > 0 n                           | modifyKeyboard.
#define DSB_MODKEYS_KEYBOARD                1   // ESC CSI > 1 n                           | modifyCursorKeys.
#define DSB_MODKEYS_CURSOR_KEYS             2   // ESC CSI > 2 n                           | modifyFunctionKeys.
#define DSB_MODKEYS_FUNCTION_KEYS           3   // ESC CSI > 3 n                           | modifyKeypadKeys.
#define DSB_MODKEYS_KEYPAD_KEYS             4   // ESC CSI > 4 n                           | modifyOtherKeys.
#define DSB_MODKEYS_OTHER_KEYS              6   // ESC CSI > 6 n                           | modifyModifierKeys.
#define DSB_MODKEYS_MODIFIER_KEYS           7   // ESC CSI > 7 n                           | modifySpecialKeys.
//    } DISABLE_KEY_MODIFIER_OPTIONS  (DSB_MODKEYS)
//    { SET_POINTER_MODE (SPM)
#define SET_POINTER_MODE                   'p'  // ESC CSI > _ p                           | Set resource value \fB\%pointerMode\fP (XTSMPOINTER), xterm.
#define SPM_NEVER_HIDE                      0   // ESC CSI > 0 p                           | never hide the pointer.
#define SPM_HIDE_IF_NOT_MOUSE_TRACKING      1   // ESC CSI > 1 p                           | hide if the mouse tracking mode is not enabled.
#define SPM_ALWAYS_HIDE_EXC_LEAVING_WINDOW  2   // ESC CSI > 2 p                           | always hide the pointer, except when leaving the window.
#define SPM_ALWAYS_HIDE                     3   // ESC CSI > 3 p                           | always hide the pointer, even if leaving/entering the window.
//    } SET_POINTER_MODE (SPM)
//    { REPORT_XTERM_NAME_VERSION
#define REPORT_XTERM_NAME_VERSION          'q'  // ESC CSI > _ q                           |
#define REPORT_XTERM_NAME_VERSION_ARG       0   // ESC CSI > 0 q                           | Report  xterm name and version (XTVERSION).
//    } REPORT_XTERM_NAME_VERSION
//    { STRST_SHIFT_ESCAPE_OPTIONS (SHIFTESC)
#define STRST_SHIFT_ESCAPE_OPTIONS         's'  // ESC CSI > _ s                           | Set/reset shift-escape options (XTSHIFTESCAPE), xterm.
#define SHIFTESC_ALLOW_OVERRIDE_MOUSE_PROT  0   // ESC CSI > 0 s                           | allow shift-key to override mouse protocol.
#define SHIFTESC_COND_ALLOW_MOD_MOUSE_PROT  1   // ESC CSI > 1 s                           | conditionally allow shift-key as modifier in mouse protocol.
#define SHIFTESC_ALWYS_ALLOW_MOD_MOUSE_PROT 2   // ESC CSI > 2 s                           | always allow shift-key as modifier in mouse protocol.
#define SHIFTESC_NEVER_ALLOW_MOD_MOUSE_PROT 3   // ESC CSI > 3 s                           | never allow shift-key as modifier in mouse protocol.
//    } STRST_SHIFT_ESCAPE_OPTIONS (SHIFTESC)
//    { STQRY_TITLE_MODE_FEATURES (STQRY_TIT)
#define STQRY_TITLE_MODE_FEATURES          't'  // ESC CSI > _ t                           | Reset title mode features to default value (XTRMTITLE), xterm.
#define STQRY_TIT_SET_WINICON_LABELS_HEX    0   // ESC CSI > 0 t                           | Set window/icon labels using hexadecimal.   
#define STQRY_TIT_QRY_WINICON_LABELS_HEX    1   // ESC CSI > 1 t                           | Query window/icon labels using hexadecimal. 
#define STQRY_TIT_SET_WINICON_LABELS_UTF8   2   // ESC CSI > 2 t                           | Set window/icon labels using UTF-8.         
#define STQRY_TIT_QRY_WINICON_LABELS_UTF8   3   // ESC CSI > 3 t                           | Query window/icon labels using UTF-8.       
//    } RESET_TITLE_MODE_FEATURES (RST_TIT)
// } END FORMAT: ESC CSI > <n>* <op>
// { BEGIN FORMAT: ESC CSI = <n> <op>
#define CSI_EQUALS_PREFIX '='
//    { SEND_DEVICE_ATTRIBUTES_TERTIARY (SDATR3)
#define SEND_DEVICE_ATTRIBUTES_TERTIARY    'c'  // ESC CSI = _  c                          | Send Device Attributes (Tertiary DA).
#define SDATR3_TERM_UNIT_ID                 0   // ESC CSI = 0  c                          | report Terminal Unit ID (default), VT400.
//    } SEND_DEVICE_ATTRIBUTES_TERTIARY (SDATR3)
// } END FORMAT: ESC CSI = <n> <op>
// { BEGIN FORMAT: ESC CSI ! <op>
#define CSI_BANG_PREFIX '!'
#define SOFT_TERMINAL_RESET                'p'  // ESC CSI ! p                             | Soft terminal reset (DECSTR), VT220 and up.
// } END FORMAT: ESC CSI ! <op>
// { BEGIN FORMAT: ESC CSI <n>* " <op>
#define CSI_QUOTE_PREFIX '"'
//    { DEC_SET_CONFORMACE_LEVEL (DECSCL)
#define DEC_SET_CONFORMACE_LEVEL           'p'  // ESC CSI __ ; _ " p                      | Set conformance level (DECSCL), VT220 and up.
#define DECSCL_LEVEL_1                      61  // ESC CSI 61 ; _ " p                      | level 1, e.g., VT100.
#define DECSCL_LEVEL_2                      62  // ESC CSI 62 ; _ " p                      | level 2, e.g., VT200.
#define DECSCL_LEVEL_3                      63  // ESC CSI 63 ; _ " p                      | level 3, e.g., VT300.
#define DECSCL_LEVEL_4                      64  // ESC CSI 64 ; _ " p                      | level 4, e.g., VT400.
#define DECSCL_LEVEL_5                      65  // ESC CSI 65 ; _ " p                      | level 5, e.g., VT500.
#define DECSCL_8BIT_CONTROLS                 0  // ESC CSI __ ; 0 " p                      | 8-bit controls.
#define DECSCL_7BIT_CONTROLS                 1  // ESC CSI __ ; 1 " p                      | 7-bit controls (DEC factory default).
#define DECSCL_8BIT_CONTROLS_2               2  // ESC CSI __ ; 2 " p                      | 8-bit controls.
//    } DEC_SET_CONFORMACE_LEVEL (DECSCL)
//    { SELECT_CHAR_PROTECTION_ATTRIBUTE (DECSCA)
#define SELECT_CHAR_PROTECTION_ATTRIBUTE   'q'  // ESC CSI _ " q                           | Select character protection attribute (DECSCA), VT220.
#define DECSCA_DECSED_DECSEL_CAN_ERASE      0   // ESC CSI 0 " q                           | DECSED and DECSEL can erase (default).
#define DECSCA_DECSED_DECSEL_CANT_ERASE     1   // ESC CSI 1 " q                           | DECSED and DECSEL cannot erase.
#define DECSCA_DECSED_DECSEL_CAN_ERASE_2    2   // ESC CSI 2 " q                           | DECSED and DECSEL can erase.
//    } SELECT_CHAR_PROTECTION_ATTRIBUTE (DECSCA)
#define REQUEST_DISPLAYED_EXTENT           'v'  // ESC CSI " v                             | Request Displayed Extent (DECRQDE), VT340, VT420.
// } END FORMAT: ESC CSI <n>* " <op>
// { BEGIN FORMAT: ESC CSI <n>* $ <op>
#define CSI_DOLAR_PREFIX '$'
#define REQUEST_ANSI_MODE                  'p'  // ESC CSI _ $ p                           | Request ANSI mode (DECRQM).
#define CHANGE_ATTR_RECT_AREA              'r'  // ESC CSI _ ; _ ; _ ; _ ; _ $ r           | Change Attributes in Rectangular Area (DECCARA), VT400 and up.  
#define REVERSE_ATTR_RECT_AREA             't'  // ESC CSI _ ; _ ; _ ; _ ; _ $ t           | Reverse Attributes in Rectangular Area (DECRARA), VT400 and up. 
//    { REQUEST_PRESENTATION_STATE_REPORT (REQPRES)
#define REQUEST_PRESENTATION_STATE_REPORT  'w'  // ESC CSI _ $ w                           | Request presentation state report (DECRQPSR), VT320 and up.                         
#define REQPRES_ERROR                       0   // ESC CSI 0 $ w                           | error.                              
#define REQPRES_CURSOR_INFORMATION          1   // ESC CSI 1 $ w                           | cursor information report (DECCIR). 
#define REQPRES_TAB_STOP                    2   // ESC CSI 2 $ w                           | tab stop report (DECTABSR).         
//    } REQUEST_PRESENTATION_STATE_REPORT (REQPRES)
#define COPY_RECT_AREA                     'v'  // ESC CSI _;_;_;_;_;_;_;_ $ v             | Copy Rectangular Area (DECCRA), VT400 and up.
#define FILL_RECT_AREA                     'x'  // ESC CSI Pc ; Pt ; Pl ; Pb ; Pr $ x      | Fill Rectangular Area (DECFRA), VT420 and up.
#define ERASE_RECT_ARE                     'z'  // ESC CSI Pt ; Pl ; Pb ; Pr $ z           | Erase Rectangular Area (DECERA), VT400 and up.
#define SELECTIVE_ERASE_RECT_AREA          '{'  // ESC CSI Pt ; Pl ; Pb ; Pr $ {           | Selective Erase Rectangular Area (DECSERA), VT400 and up.  }}
//    {  SELECT_COLUMNS_PER_PAGE
#define SELECT_COLUMNS_PER_PAGE            '|'  // ESC CSI ___ $ |                         | Select columns per page (DECSCPP), VT340.
#define SELECT_COLUMNS_PER_PAGE_DEFAULT     0   // ESC CSI 0   $ |                         | 80 columns, default if omitted.
#define SELECT_COLUMNS_PER_PAGE_80          80  // ESC CSI 80  $ |                         | 80 columns.
#define SELECT_COLUMNS_PER_PAGE_132         132 // ESC CSI 132 $ |                         | 132 columns.
//    }  SELECT_COLUMNS_PER_PAGE
//    { SELECT_ACTIVE_STATUS_DISPLAY (DECSASD)                                                                                                           {{{{
#define SELECT_ACTIVE_STATUS_DISPLAY       '}'  //  ESC CSI _ $ }                          | Select active status display (DECSASD), VT320 and up.
#define DECSASD_MAIN                        0   //  ESC CSI 0 $ }                          | main (default)
#define DECSASD_STATUS_LINE                 1   //  ESC CSI 1 $ }                          | status line
//    } SELECT_ACTIVE_STATUS_DISPLAY (DECSASD)
//    { SELECT_STATUS_LINE_TYPE (DECSSLNTYPE)                                                                                                            
#define SELECT_STATUS_LINE_TYPE            '~'  //  ESC CSI _ $ ~                          | Select status line type (DECSSDT), VT320 and up.
#define DECSSLNTYPE_NONE                    0   //  ESC CSI 0 $ ~                          | none
#define DECSSLNTYPE_INDICATOR               1   //  ESC CSI 1 $ ~                          | indicator (default)
#define DECSSLNTYPE_HOST_WRITEABLE          2   //  ESC CSI 2 $ ~                          | host-writable.
//    } SELECT_STATUS_LINE_TYPE (DECSSLNTYPE)                                                                                                             
// } END FORMAT: ESC CSI <n>* $ <op>
// { BEGIN FORMAT: ESC CSI <n>+ ' <op>
#define CSI_TICK_PREFIX                    '\'' // ESC CSI <n> ' _
#define ENABLE_FILTER_RECT                 'w'  // ESC CSI <top>; <lef>; <bot>; <rig> ' w  | Enable Filter Rectangle (DECEFR), VT420 and up.
//    { ENABLE_LOCATOR_REPORTING (DECELR)
#define ENABLE_LOCATOR_REPORTING           'z'  // ESC CSI _; _ ' z                        | Enable Locator Reporting (DECELR).
#define DECELR_DISABLED                     0   // ESC CSI 0; _ ' z                        | Locator disabled (default).                    
#define DECELR_ENABLED                      1   // ESC CSI 1; _ ' z                        | Locator enabled.                               
#define DECELR_ENABLED_ONCE                 2   // ESC CSI 2; _ ' z                        | Locator enabled for one report, then disabled. 
#define DECELR_CHARACTER_CELLS              0   // ESC CSI _; 0 ' z                        | or omitted. default to character cells.
//    } ENABLE_LOCATOR_REPORTING (DECELR)
//    { SELECT_LOCATOR_EVENTS (DECSLE)
#define SELECT_LOCATOR_EVENTS              '{'  // ESC CSI  _ ' {                          | Select Locator Events (DECSLE).                          }}
#define DECSLE_EXPLICIT_HOST_REQUESTS       0   // ESC CSI  0 ' {                          | only respond to explicit host requests (DECRQLP).        }
#define DECSLE_REPORT_BUTTON_DOWN           1   // ESC CSI  1 ' {                          | report button down transitions.                          }
#define DECSLE_NOT_REPORT_BUTTON_DOWN       2   // ESC CSI  2 ' {                          | do not report button down transitions.                   }
#define DECSLE_REPORT_BUTTON_UP             3   // ESC CSI  3 ' {                          | report button up transitions.                            }
#define DECSLE_NOT_REPORT_BUTTON_UP         4   // ESC CSI  4 ' {                          | do not report button up transitions.                     }
//    } SELECT_LOCATOR_EVENTS (DECSLE)
//    { REQUEST_LOCATOR_POSITION (DECRQLP)
#define REQUEST_LOCATOR_POSITION           '|'  // ESC CSI  _ ' |                          | Request Locator Position (DECRQLP).
#define DECRQLP_SINGLE_LOCATOR_REPORT_1     0   // ESC CSI  0 ' |                          | or omitted  transmit a single DECLRP locator report.
#define DECRQLP_SINGLE_LOCATOR_REPORT_2     1   // ESC CSI  1 ' |                          | or omitted  transmit a single DECLRP locator report.
//    } REQUEST_LOCATOR_POSITION (DECRQLP)                                                                                                              {{
#define DEC_INSERT_COLUMNS                 '}'  // ESC CSI <n> ' }                         | Insert <n> Column(s) (default = 1) (DECIC), VT420 and up. 
#define DEC_DELETE_COLUMNS                 '~'  // ESC CSI <n> ' ~                         | Delete <n> Column(s) (default = 1) (DECDC), VT420 and up. 
// } END FORMAT: ESC CSI <n>+ ' <op>
// { BEGIN FORMAT: ESC CSI <n>+ * <op>
#define CSI_START_PREFIX '*'
//    { SELECT_ATTRIBUTE_CHANGE_EXTENT (DECSACE)
#define SELECT_ATTRIBUTE_CHANGE_EXTENT    'x'   // ESC CSI _ * x                           | Select Attribute Change Extent (DECSACE), VT420 and up. 
#define DECSAGE_START_TO_END_WRAPPED       0    // ESC CSI 0 * x                           | from start to end position, wrapped.                    
#define DECSAGE_START_TO_END_WRAPPED_2     1    // ESC CSI 1 * x                           | from start to end position, wrapped.                    
#define DECSAGE_RECT_EXACT                 2    // ESC CSI 2 * x                           | rectangle (exact).                                      
//    } SELECT_ATTRIBUTE_CHANGE_EXTENT (DECSACE)
#define REQUEST_CHECKSUM_REACT_AREA       'y'   // ESC CSI <i>;<g>;<tp>;<lf>;<bt>;<rg> * y | Request Checksum of Rectangular Area (DECRQCRA), VT420 and up.
#define SELECT_NUM_LINES_PER_SCREEN       '|'   // ESC CSI <n> * |                         | Select number of lines per screen (DECSNLS), VT420 and up.
// } END FORMAT: ESC CSI <n>+ * <op>
// { BEGIN FORMAT: ESC CSI <n> ; <m> ; <l> , <op>
#define CSI_COMMA_PREFIX  ','
//    { ASSIGN_COLOR (DECAC)
#define ASSIGN_COLOR                      '|'  // ESC CSI _; _; _ , |                      | Assign Color (DECAC), VT525 only.
#define DECAC_NORMAL_TEXT                  1   // ESC CSI 1; _; _ , |                      | normal text  
#define DECAC_WINDOW_FRAME                 2   // ESC CSI 2; _; _ , |                      | window frame 
//    } ASSIGN_COLOR (DECAC)
//    { ALTERNATE_TEXT_COLOR (DECATC)                                                                                                       {{{{{{{{{{{{{{{{{{ 
#define ALTERNATE_TEXT_COLOR              '}'  // ESC CSI __; _; _ , }                      | Alternate Text Color (DECATC), VT525 only.
#define DECATC_NORMAL_TEXT                 0   // ESC CSI 00; _; _ , }                      | normal text
#define DECATC_BOLD                        1   // ESC CSI 01; _; _ , }                      | bold
#define DECATC_REVERSE                     2   // ESC CSI 02; _; _ , }                      | reverse
#define DECATC_UNDERLINE                   3   // ESC CSI 03; _; _ , }                      | underline
#define DECATC_BLINK                       4   // ESC CSI 04; _; _ , }                      | blink
#define DECATC_BOLD_REVERSE                5   // ESC CSI 05; _; _ , }                      | bold reverse
#define DECATC_BOLD_UNDERLINE              6   // ESC CSI 06; _; _ , }                      | bold underline
#define DECATC_BOLD_BLINK                  7   // ESC CSI 07; _; _ , }                      | bold blink
#define DECATC_REVERSE_UNDERLINE           8   // ESC CSI 08; _; _ , }                      | reverse underline
#define DECATC_REVERSE_BLINK               9   // ESC CSI 09; _; _ , }                      | reverse blink
#define DECATC_UNDERLINE_BLINK             10  // ESC CSI 10; _; _ , }                      | underline blink
#define DECATC_BOLD_REVERSE_UNDERLINE      11  // ESC CSI 11; _; _ , }                      | bold reverse underline
#define DECATC_BOLD_REVERSE_BLINK          12  // ESC CSI 12; _; _ , }                      | bold reverse blink
#define DECATC_BOLD_UNDERLINE_BLINK        13  // ESC CSI 13; _; _ , }                      | bold underline blink
#define DECATC_REVERSE_UNDERLINE_BLINK     14  // ESC CSI 14; _; _ , }                      | reverse underline blink
#define DECATC_BOLD_REVRSE_UNDERLINE_BLINK 15  // ESC CSI 15; _; _ , }                      | bold reverse underline blink
//    } ALTERNATE_TEXT_COLOR (DECATC)
// } END FORMAT: ESC CSI <n> ; <m> ; <l> , <op>

// TODO: I dont really know what to do with the following sequences.
//    what? is this just a typo and it was suposed to be CSI & ; u? or is it some form of special notation?
//    v
// CSI&; u  User-Preferred Supplemental Set (DECRQUPSS), VT320, VT510.
// is this one the same as Scroll Down? is the difference only the number of arguments?
// CSI Ps ; Ps ; Ps ; Ps ; Ps T  Initiate highlight mouse tracking (XTHIMOUSE), xterm.
   
#define ST STRING_TERMINATOR
/*-----------------------------------------------------------------------------/
/               Operating System Commands starts with ESC + OSC                /
/                                                                              /
/ All the commands in the format ESC OSC ________.                             /
/                                                                              /
/ Some of these commands have arguments. When these arguments have to be one of/
/ a set of options, the options will be listed right bellow the command.       /
/ Otherwise, the argument is arbitrary and specific conventions for each       /
/ command may apply.                                                           /
/                                                                              /
/  ESC OSC <n>* <op>                                                           /
/                                                                              /
/-----------------------------------------------------------------------------*/
//                                  Byte         Usage                                    | Description
#define SET_TEXT_PARAMETERS_1       BELL      // ESC OSC <n> ; <t> BEL                    | (STP) Set Text Parameters, xterm. 
#define SET_TEXT_PARAMETERS_2       ST        // ESC OSC <n> ; <t> ST                     | (STP) Set Text Parameters, xterm.
#define STP_ICON_NAME_WINDOW_TITLE  0         // ESC OSC  0  ; <t> ST                     | Change Icon Name and Window Title to Pt.
#define STP_ICON_NAME               1         // ESC OSC  1                               | Change Icon Name to Pt.
#define STP_WINDOW_TITLE            2         // ESC OSC  2                               | Change Window Title to Pt.
#define STP_X_PROPERTY              3         // ESC OSC  3                               | Set X property on top-level window.
#define STP_COLOR_NUMBER            4         // ESC OSC  4  ;  c  ; spec                 | Change Color Number c to the color specified by spec.
#define STP_SPECIAL_COLOR_NUMBER    5         // ESC OSC  5  ;  c  ; spec                 | Change Special Color Number c to the color specified by spec.
#define STP_TOGGLE_SPECIAL_CLRNUM   6         // ESC OSC  6  ;  c  ; f                    | Enable/disable Special Color Number c.
#define STP_VT100_FG_COLOR          10        // ESC OSC  10                              | Change VT100 text foreground color to Pt.
#define STP_VT100_BG_COLOR          11        // ESC OSC  11                              | Change VT100 text background color to Pt.
#define STP_TEXT_CURSOR_COLOR       12        // ESC OSC  12                              | Change text cursor color to Pt.
#define STP_POINTER_FG_COLOR        13        // ESC OSC  13                              | Change pointer foreground color to Pt.
#define STP_POINTER_BG_COLOR        14        // ESC OSC  14                              | Change pointer background color to Pt.
#define STP_TEKTRONIX_FG_COLOR      15        // ESC OSC  15                              | Change Tektronix foreground color to Pt.
#define STP_TEKTRONIX_BG_COLOR      16        // ESC OSC  16                              | Change Tektronix background color to Pt.
#define STP_HIGHLIGHT_BG_COLOR      17        // ESC OSC  17                              | Change highlight background color to Pt.
#define STP_TEKTRONIX_CURSOR_COLOR  18        // ESC OSC  18                              | Change Tektronix cursor color to Pt.
#define STP_HIGHLIGHT_FG_COLOR      19        // ESC OSC  19                              | Change highlight foreground color to Pt.
#define STP_POINTER_CURSOR_SHAPE    22        // ESC OSC  22                              | Change pointer cursor shape to Pt.
#define STP_CHANGE_LOG_FILE         46        // ESC OSC  46                              | Change Log File to Pt.
#define STP_SET_FONT                50        // ESC OSC  50                              | Set Font to Pt.
#define STP_FOR_EMACS               51        // ESC OSC  51                              | reserved for Emacs shell.
#define STP_MANIP_SELECTION_DATA    52        // ESC OSC  52                              | Manipulate Selection Data.
#define STP_XTERM_QUERY_ALLOWED     60        // ESC OSC  60                              | Query allowed features (XTQALLOWED).
#define STP_XTERM_QUERY_DISALLOWED  61        // ESC OSC  61                              | Query disallowed features (XTQDISALLOWED).
#define STP_XTERM_QUERY_ALLOWABLE   62        // ESC OSC  62                              | Query allowable features (XTQALLOWABLE).
#define STP_RESET_COLOR             104       // ESC OSC  104 ; c                         | Reset Color Number c.
#define STP_RESET_SPECIAL_COLOR     105       // ESC OSC  105 ; c                         | Reset Special Color Number c.
#define STP_TOGGLE_SPECIAL_COLOR    106       // ESC OSC  106 ; c ; f                     | Enable/disable Special Color Number c.
#define STP_RESET_VT100_TXTFGCLR    110       // ESC OSC  110                             | Reset VT100 text foreground color.
#define STP_RESET_VT100_TXTBGCLR    111       // ESC OSC  111                             | Reset VT100 text background color.
#define STP_RESET_TEXT_CURSOR_COLOR 112       // ESC OSC  112                             | Reset text cursor color.
#define STP_RESET_POINTER_FG_COLOR  113       // ESC OSC  113                             | Reset pointer foreground color.
#define STP_RESET_POINTER_BG_COLOR  114       // ESC OSC  114                             | Reset pointer background color.
#define STP_RESET_TKTX_FG_COLOR     115       // ESC OSC  115                             | Reset Tektronix foreground color.
#define STP_RESET_TKTX_BG_COLOR     116       // ESC OSC  116                             | Reset Tektronix background color.
#define STP_RESET_HIGHLIGHT_COLOR   117       // ESC OSC  117                             | Reset highlight color.
#define STP_RESET_TKTX_CURSOR_COLOR 118       // ESC OSC  118                             | Reset Tektronix cursor color.
#define STP_RESET_HIGHLIGHT_FGCOLOR 119       // ESC OSC  119                             | Reset highlight foreground color.
#define STP_SET_ICON_TO_FILE        'I'       // ESC OSC  I ; c                           | Set icon to file.
#define STP_SET_WINDOW_TITLE        'l'       // ESC OSC  l ; c                           | Set window title.
#define STP_SET_ICON_LABEL          'L'       // ESC OSC  L ; c                           | Set icon label.


\\DATA 3000
\\EXTRA 0
marcos equ "lascarinho mangatinho mininho"
ornitorico equ "perry"
null equ -1
salsa equ 0420

MOV EDX, DS
SYS 15
ADD EDX, 6
MOV [EDX], 3
PUSH [EDX]
MOV CX,6
SYS 3
POP EAX
SYS 15
SYS 4
MOV EBX, [6]
STOP
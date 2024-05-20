\\DATA 3000
\\EXTRA 0
marcos equ "lascar"

MOV EDX, DS
SYS 15
ADD EDX, 6
PUSH EDX //en memoria es la ubicacion M[4.063]
MOV [EDX], 3 //en memoria es la ubicacion M[45]
PUSH [EDX]
POP EAX
SYS 15
MOV EBX, [6]
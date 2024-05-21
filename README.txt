para compilar el archivo desde el cmd:
...> gcc main.c TMV.c

y se compila y aparece el TM.exe en /bin/debug, el cual se ejectua para el codigo binario .vmx asi:
...> TMV.exe [algo.vmx] [otroAlgo.vmi] [m=M] [-d]

y en caso de querer el disassembler
...> algo.exe otroAlgo.vmx -d
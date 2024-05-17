#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "TMV.h"

int main(int argc, char *argv[]){ //vmx.exe [filename.vmx] [filename.vmi] [m=M] [-d] //argc entre 2 y 5, argv[0;4]
    TMV mv;
    unsigned int SIZE_MEM= 16384;
    printf("\nbueno, se q la mem es de tamanio %u chars, same as kb, a ver cuanto da el calculo\n", SIZE_MEM);
    if( *(argv[argc-1] + 1) == '='){ // argv[argc-1] es argc-1 porque si esta la flag -d, no nos interesa en cuanto se setea la memoria
        char aux; char *strMem;
        aux= *(argv[argc-1] + 2);           //lee el primer digito del numero
        strMem= strchr(argv[argc-1], aux);  //, para luego usarlo para retomar de ahi el numero completo
        SIZE_MEM = atoi(strMem) * 1024 ;    //y transformarlo a bytes
    }
    if (strcmp(strchr(argv[1],'.') , ".vmx") == 0 ){ //si tiene vmx
        if ( argc>2 && strchr(argv[2],'.') && strcmp(strchr(argv[2],'.') , ".vmi") == 0 ) //si tmb tiene vmi
            mv= createMV(argv[1], argv[2], SIZE_MEM);
        else
            mv= createMV(argv[1], "no", SIZE_MEM);
    }else
        mv= levantaVMI(argv[1]);



    if  (strcmp(argv[argc-1], "-d") == 0)
        disAssembler(&mv, argv[1]);
    else {
        execute(&mv);
        //printf("\nbueno, se q la mem es de tamanio %u chars, same as kb, a ver cuanto da el calculo\n", SIZE_MEM);
        //guardaImagen(mv);
    }
    scanf("%u",&SIZE_MEM);
    printf("\nbueno, se q la mem es de tamanio %u chars, same as kb, a ver cuanto da el calculo\n", SIZE_MEM);
    return 0;
}

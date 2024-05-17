#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "TMV.h"

TMV createMV(char* fname, char* vmiName, unsigned int SIZE_MEM){
    TMV maq;
    int valores_R[16] = {-1, -1, -1, -1, -1, 0x0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};//CS=DS=ES=SS=KS= -1
    int n;
    FILE* arch;
    unsigned short int version, tamCodigo, tamDS, tamES, tamSS, tamKS, offsetEP;
    char aux, identificador[6]; // son 6 por el \0??

    maq.TDS = NULL;

    if ( (arch= fopen(fname,"rb")) != NULL){
            fseek(arch,5,SEEK_SET);
            fread(&aux,sizeof(char),1,arch);
            version= (unsigned short int) aux;
            printf("linea ~22 la version es:%u\n",version);

            memcpy(maq.R, valores_R, sizeof(valores_R));
            maq.error[0] = maq.error[1] = maq.error[2] = maq.error[3] = maq.error[4] = maq.error[5] = maq.error[6] = 0;

            fseek(arch,0,SEEK_SET);
            if (version==2 || version==1){ //versiones permitidas
                sprintf(maq.nombreImg, vmiName); //si no tiene vmi, se guarda "no"

                maq.M= (char*)malloc(SIZE_MEM * sizeof(char));
                n= SIZE_MEM / 1024;
                maq.ramKiB= (short int) n;
                //printf("\n ram:%d \n",maq.ramKiB);

                n=8; // de los cuales usaremos solo 5 segmentos en la TDS
                maq.TDS= (int*)malloc(n * sizeof(int));


                if(version==1){
                    leeHeaderV1(arch, &tamCodigo, identificador);
                    tamDS= SIZE_MEM - tamCodigo;
                    tamES=tamKS=tamSS=offsetEP=0;
                }else
                    leeHeaderV2(arch, &tamCodigo, &tamDS, &tamES, &tamSS, &tamKS, &offsetEP, identificador);

                if ((tamCodigo+tamDS+tamES+tamSS+tamKS) <= SIZE_MEM){
                    n=0; //contador de segmentos existentes
                    printf("cuidado aca abajo, y en general estos calculos de offset, linea ~55\n");
                    if(tamKS>0){
                        maq.R[4]=0x00000000; //KS
                        maq.TDS[n]= 0 | tamKS;
                        n++;
                    }
                    if(tamCodigo>0){
                        maq.R[0]= (n<<16); //CS
                        if (n>0)
                            maq.TDS[n]= 0 | baseMasOffset(maq.TDS[0]); // la base de CS sera la pos final de KS
                        else
                            maq.TDS[n]= 0;
                        maq.TDS[n]= (maq.TDS[n]<<16) | tamCodigo;
                        n++;
                    }
                    if(tamDS>0){ //en version 1 tamDS= SIZE_MEM - tamCodigo;
                        maq.R[1]= (n<<16) ; //DS
                        maq.TDS[n]= 0 | baseMasOffset(maq.TDS[n-1]); // la base de DS sera la pos final de CS
                        maq.TDS[n]= (maq.TDS[n]<<16) | tamDS;
                        n++;
                    }
                    if(tamES>0){
                        maq.R[2]= (n<<16); //ES
                        maq.TDS[n]= 0 | baseMasOffset(maq.TDS[n-1]);
                        maq.TDS[n]= (maq.TDS[n]<<16) | tamES;
                        n++;
                    }
                    if(tamSS>0){
                        maq.R[3]= (n<<16); //SS
                        maq.TDS[n]= 0 | baseMasOffset(maq.TDS[n-1]);
                        maq.TDS[n]= (maq.TDS[n]<<16) | tamSS;
                        maq.R[6]= baseMasOffset( maq.TDS[ baseMasOffset( maq.R[3] ) ] ); //SP
                    }

                    codigoAMemoria(&maq,arch,tamCodigo);
                    if (tamKS>0)
                        KSaMemoria(&maq,arch,tamKS);
                    maq.R[5]= ( maq.TDS[ baseMasOffset(maq.R[0]) ] & 0xFFFF0000 ) + offsetEP; //base del CS y offset del entry point

                }else
                    maq.error[6]=1;

            }
            else
                printf("La version no coincide\n");
            fclose(arch);
    }
    else
        printf("El archivo fallo al abrir\n");

    //maq.M= la memoria no se inicializa pq es gigante y si tiene basura la ignoramos
    return maq;
}

int baseMasOffset(int dirLog){
    return (((dirLog>>16)&0xFFFF) + (dirLog&0xFFFF));
}

int changeEndian(int dato){
    int aux;

    aux= (dato>>24) & 0xFF;
    aux= aux | ((dato>>8) & 0xFF00);
    aux= aux | ((dato<<8) & 0xFF0000);
    aux= aux | ((dato<<24) & 0xFF000000);

    return aux;
}

TMV levantaVMI(char* fname){
    FILE* arch;
    unsigned short int version, tamRAM;
    char identificador[6];
    int i, aux;
    TMV maq;

    maq.TDS = NULL;

    if ( (arch= fopen(fname,"rb")) != NULL){

        leeHeaderImagen(arch, &version, &tamRAM, identificador);
        if (version == 1 || version == 2){ //versiones permitidas
            sprintf(maq.nombreImg,fname);
            maq.ramKiB= tamRAM;
            tamRAM= tamRAM*1024; //porque se guardaba en KiB
            maq.M= (char*)malloc(tamRAM * sizeof(char));

            i=8; // de los cuales usaremos solo 5 segmentos en la TDS
            maq.TDS= (int*)malloc(i * sizeof(int));

            for (i=0; i<16; i++){
                fread(&aux, 4/*sizeof(int)*/, 1, arch); //chequear que no me arruine el little y big endian
                maq.R[i]= changeEndian(aux);
            }
            for (i=0; i<8; i++){
                fread(&aux, 4/*sizeof(int)*/, 1, arch);
                maq.TDS[i]= changeEndian(aux);
            }
            for (i=0; i<tamRAM; i++)
                fread(&maq.M[i], 1/*sizeof(char)*/, 1, arch);
        }
        else
            printf("La version no coincide (%u) \n",version);

        fclose(arch);
    }
    return maq;
}

void leeHeaderV1(FILE* arch, unsigned short int *tamCodigo, char identificador[6]){
    char aux;
    unsigned short int cont=-1;

    //printf("Header: ");
    do{ //LEE EL HEADER
        cont++;
        fread(&aux,sizeof(char),1,arch);
        //printf("%X ", aux); //nomas chequeando

        switch(cont){
            case 0 ... 4:
                identificador[cont]=aux;
                break;
            case 5:
                identificador[cont]='\0';
                //*version= (short int)aux;
                //printf("(<- version) ");
                break;
            case 6:
                *tamCodigo= ((aux & 0xFF)<<8);
                break;
            case 7:
                *tamCodigo= *tamCodigo | (aux & 0xFF);
                break;
            default:
                printf("se comio el primer byte del codigo\n");
                break;
        }
    }while (!feof(arch) && cont<7); //EL FEOF DEJARIA PASAR EN CASO DE QUE HAYA SIMPLEMENTE 1 BIT DE MAS, PERO DSPS LEEMOS DE A 8 BITS (char)
    //printf("LEYO EL HEADER CON version:%u, tamCodigo:%u, identif:%s \n",*version, *tamCodigo, identificador);

}

void leeHeaderV2(FILE* arch, unsigned short int *tamCodigo, unsigned short int *tamDS, unsigned short int *tamES, unsigned short int *tamSS, unsigned short int *tamKS, unsigned short int *offsetEP, char identificador[6]){
    char aux;
    unsigned short int cont=-1;

    //printf("Header: ");
    do{ //LEE EL HEADER
        cont++;
        fread(&aux,sizeof(char),1,arch);
        //printf("%X ", aux); //nomas chequeando

        switch(cont){
            case 0 ... 4:
                identificador[cont]=aux;
                break;
            case 5:
                identificador[cont]='\0';
                //*version= (short int)aux;
                //printf("(<- version) ");
                break;
            case 6:
                *tamCodigo= ((aux & 0xFF)<<8);
                break;
            case 7:
                *tamCodigo= *tamCodigo | (aux & 0xFF);
                break;
            case 8:
                *tamDS= ((aux & 0xFF)<<8);
                break;
            case 9:
                *tamDS= *tamDS | (aux & 0xFF);
                break;
            case 10:
                *tamES= ((aux & 0xFF)<<8);
                break;
            case 11:
                *tamES= *tamES | (aux & 0xFF);
                break;
            case 12:
                *tamSS= ((aux & 0xFF)<<8);
                break;
            case 13:
                *tamSS= *tamSS | (aux & 0xFF);
                break;
            case 14:
                *tamKS= ((aux & 0xFF)<<8);
                break;
            case 15:
                *tamKS= *tamKS | (aux & 0xFF);
                break;
            case 16:
                *offsetEP= ((aux & 0xFF)<<8);
                break;
            case 17:
                *offsetEP= *offsetEP | (aux & 0xFF);
                break;
            default:
                printf("se comio el primer byte del codigo\n");
                break;
        }
    }while (!feof(arch) && cont<17); //EL FEOF DEJARIA PASAR EN CASO DE QUE HAYA SIMPLEMENTE 1 BIT DE MAS, PERO DSPS LEEMOS DE A 8 BITS (char)
    //printf("LEYO EL HEADER CON version:%u, tamCodigo:%u, identif:%s \n",*version, *tamCodigo, identificador);

}

void codigoAMemoria(TMV *mv, FILE* arch, unsigned short int tamCodigo){
    int i, finCS;
    char aux;

    printf("\n");
    i= ( mv->TDS[baseMasOffset(mv->R[0])] >>16) &0xFFFF ; // la base del TDS [CS]
    finCS= i + tamCodigo;
    for(i ; i<finCS ; i++){
        fread(&aux,sizeof(char),1,arch);
    //    printf("%X ", aux);
        mv->M[i] = aux;
    }
    //printf("\n\n");

}

void KSaMemoria(TMV* mv, FILE* arch, unsigned short int tamKS){
   int i, finKS;
    char aux;

    printf("\n");
    i= ( mv->TDS[baseMasOffset(mv->R[4])] >>16) &0xFFFF ; // la base del TDS [KS]
    finKS= i + tamKS;

    while ( (i<finKS) && !(feof(arch)) ){
        fread(&aux,sizeof(char),1,arch);
    //    printf("%X ", aux);
        mv->M[i] = aux;
    }

    if( !feof(arch) )/////////////////////////////////////////////////////////////////////////////////////////////////OJISIMO/////////////////////////////////////
        mv->error[6]=1;
    //printf("\n\n");
}

void inicializaVectorFunc(TFunc func) {

    func[0] = MOV;
    func[1] = ADD;
    func[2] = SUB;
    func[3] = SWAP;
    func[4] = MUL;
    func[5] = DIV;
    func[6] = CMP;
    func[7] = SHL;
    func[8] = SHR;
    func[9] = AND;
    func[10] = OR;
    func[11] = XOR;
    func[12] = RND;
    func[16] = SYS;
    func[17] = JMP;
    func[18] = JZ;
    func[19] = JP;
    func[20] = JN;
    func[21] = JNZ;
    func[22] = JNP;
    func[23] = JNN;
    func[24] = LDL;
    func[25] = LDH;
    func[26] = NOT;
    //func[27] = PUSH; //NUEVO
    //func[28] = POP;  //NUEVO
    func[31] = STOP;

}

void inicializaVectorFuncDisass(char *func[32]) {

    func[0] = "MOV";
    func[1] = "ADD";
    func[2] = "SUB";
    func[3] = "SWAP";
    func[4] = "MUL";
    func[5] = "DIV";
    func[6] = "CMP";
    func[7] = "SHL";
    func[8] = "SHR";
    func[9] = "AND";
    func[10] = "OR";
    func[11] = "XOR";
    func[12] = "RND";
    func[16] = "SYS";
    func[17] = "JMP";
    func[18] = "JZ";
    func[19] = "JP";
    func[20] = "JN";
    func[21] = "JNZ";
    func[22] = "JNP";
    func[23] = "JNN";
    func[24] = "LDL";
    func[25] = "LDH";
    func[26] = "NOT";
    func[27] = "PUSH"; //NUEVO
    func[28] = "POP";  //NUEVO
    func[31] = "STOP";

}

int hayError(TMV mv){
    if (mv.error[0]==1 || mv.error[1]==1 || mv.error[2]==1 || mv.error[3]==1 || mv.error[4]==1 || mv.error[5]==1 || mv.error[6]==1)
        return 1;

    return 0;
}

void avanzaUnaInstruccion(TMV *mv, TFunc func){
    short int codOp;
    char tamOpA, tamOpB;
    TOp op[2];
    int ubiAux, i;

    leeInstruccion(mv, &codOp, &tamOpA, &tamOpB);

    i=0;
    ubiAux= baseMasOffset(mv->R[5]) ; ///(asumimos que la base del CS=0 )+ offset ,MAL deberias ser como en el sidsasembler
    op[1]= guardaOperandos(mv, tamOpB, ubiAux);
    ubiAux+= (int) ( (~tamOpB) & 0b11); //le suma los que leyo del anterior
    i+= (int) ( (~tamOpB) & 0b11);
    op[0]= guardaOperandos(mv, tamOpA, ubiAux);
    ubiAux+= (int) ( (~tamOpA) & 0b11); //le suma los que leyo del anterior
    i+= (int) ( (~tamOpA) & 0b11);

    //mv->R[5]= mv->R[5] & 0xffff0000;
    mv->R[5]= mv->R[5] + (i+1); //setea el IP en el siguiente, ignorando si el codOp es un salto

    if ( (codOp>=0 && codOp<=12) || (codOp>=16 && codOp<=26) || codOp==31 )
        func[codOp](mv, op);
    else
        mv->error[2]=1;


    printf("codOp=%u, errores?:%u\n", codOp, hayError(*mv));
    //for (i=0; i<50; i++)
    //    printf("M[%d]=%X ", i, mv->M[i]);
    //printf("\n");

}

void execute(TMV *mv){
    TFunc func;
    inicializaVectorFunc(func);

    while ( hayError(*mv)==0 && (((mv->R[5]>>16) & 0xffff) == (( mv->TDS[baseMasOffset(mv->R[0])] >>16)&0xFFFF )) && (mv->R[5] & 0xffff)<(mv->TDS[baseMasOffset(mv->R[0])] & 0xffff)){
            //while IP2primerosBytes==0 es decir apunta al TDS[R[0]], pq ahi esta el CS && offset<size
        avanzaUnaInstruccion(mv,func);

    }
    if ( hayError(*mv)==1 ){
        if (mv->error[0]==1)
            printf("ERROR DE SEGMENTACION, IP SE PASO DEL SEGMENTO DE CODIGO. Segmentation Fault \n");
        if (mv->error[1]==1)
            printf("ERROR DE DIVISION POR 0 \n");
        if (mv->error[2]==1)
            printf("ERROR POR CODIGO DE INSTRUCCION INVALIDO \n");
        if (mv->error[3]==1)
            printf("-----------------STOP----------------- \n");
        if (mv->error[4]==1)
            printf("ERROR POR STACK OVERFLOW \n");
        if (mv->error[5]==1)
            printf("ERROR POR STACK UNDERFLOW \n");
        if (mv->error[6]==1)
            printf("ERROR POR MEMORIA INSUFICIENTE \n");
    }

}

void disAssembler(TMV *mv, char* fname){
    short int codOp;
    char tamOpA, tamOpB;
    FILE* arch;
    unsigned short int tamCodigo;
    char identificador[6]; // son 6 por el \n??
    TOp op[2];
    int ubiAux, i;
    char *funcDisass[32], str[6];

    if ( (arch= fopen(fname,"rb")) != NULL){
        leeHeaderV1(arch, &tamCodigo, identificador);
        fclose(arch);

        inicializaVectorFuncDisass(funcDisass);
        for(i=0; i<tamCodigo; i++){
            leeInstruccion(mv, &codOp, &tamOpA, &tamOpB);
            ubiAux= baseMasOffset(mv->R[5]);
            printf("[%X] %X ", (short int)ubiAux &0xFFFF , (mv->M[ubiAux] &0xFF));
            op[1]= guardaOperandosYDisass(mv, tamOpB, ubiAux);
            ubiAux+= (int) ( (~tamOpB) & 0b11); //le suma los que leyo del anterior
            i+= (int) ( (~tamOpB) & 0b11);
            op[0]= guardaOperandosYDisass(mv, tamOpA, ubiAux);
            ubiAux+= (int) ( (~tamOpA) & 0b11); //le suma los que leyo del anterior
            i+= (int) ( (~tamOpA) & 0b11);

            mv->R[5]= mv->R[5] & 0xffff0000;
            mv->R[5]= mv->R[5] | (i+1); //setea el IP en el siguiente

            if ( (codOp>=0 && codOp<=12) || (codOp>=16 && codOp<=28) || codOp==31 ){
                inicializaVectorFuncDisass(funcDisass); // lo inicializa cada vez q va a imprimir pq sino a veces se buguea y reemplaza los contenidos
                printf("\t| %s\t", funcDisass[codOp]);
                imprimeOp(*mv, op[0], str);
                printf("%s, ", str);
                imprimeOp(*mv, op[1], str);
                printf("%s", str);
            }else{
                printf("mm no esta el mnemonico de este codigo de operacion!! (codOp= %d)", codOp);
                mv->error[2]=1;
            }
            printf("\n");
        }
    }
}

void imprimeOp(TMV mv, TOp op, char str[6]){
    int offset;
    char aux[6];

    switch(op.tipo){
        case(0b00): //memoria
            setStrDeReg(mv, (op.priByte & 0xF), aux);

            offset= op.segByte;
            offset= (offset<<8) | (op.terByte); //suma el offset que lleva el operando en si
            offset= offset<<16; offset= offset>>16; //propaga el signo en caso de ser negativo
            if (offset<0)
                sprintf(str, "[%s%d]", aux, offset);
            else
                sprintf(str, "[%s+%d]", aux, offset);
            break;
        case(0b01): //inmediato
            sprintf(str, "%d", getOp(mv, op));
            break;
        case(0b10): //registro
            setStrDeReg(mv, op.priByte, str);
            break;
        default: //nulo
            sprintf(str, "%s", "none");
            break;
    }
}

void setStrDeReg(TMV mv, char byte, char str[6]){
    switch(byte & 0xF){
        case(0):
            sprintf(str, "%s", "CS");
            break;
        case(1):
            sprintf(str, "%s", "DS");
            break;
        case(5):
            sprintf(str, "%s", "IP");
            break;
        case(8):
            sprintf(str, "%s", "CC");
            break;
        case(9):
            sprintf(str, "%s", "AC");
            break;
        case(10):
            sprintf(str, "%s", "EAX");
            break;
        case(11):
            sprintf(str, "%s", "EBX");
            break;
        case(12):
            sprintf(str, "%s", "ECX");
            break;
        case(13):
            sprintf(str, "%s", "EDX");
            break;
        case(14):
            sprintf(str, "%s", "EEX");
            break;
        case(15):
            sprintf(str, "%s", "EFX");
            break;
        default:
            printf("ERROR REGISTRO INEXISTENTE\n");
            mv.error[2]=1;
            break;
    }
    switch((byte>>4) & 0b11){
        case(0):
            break;
        case(1):
            sprintf(str, "%cL", str[1]);
            break;
        case(2):
            sprintf(str, "%cH", str[1]);
            break;
        case(3):
            sprintf(str, "%cX", str[1]);
            break;
        default:
            printf("no deberia ni poder llegar aca, debi hacer la mascara mal\n");
            mv.error[2]=1;
            break;
    }
}

TOp guardaOperandosYDisass(TMV *mv, char tamOp, int ubicacion){
    short int tam;
    TOp op;

    op.tipo= tamOp;
    tam= (short int) ( (~tamOp) & 0b11);

    if (tam>0){
        op.priByte= mv->M[ubicacion+1];
        printf("%X ", op.priByte&0xFF);
        if(tam>1){
            op.segByte= mv->M[ubicacion+2];
            printf("%X ", op.segByte&0xFF);
            if(tam>2){
                op.terByte= mv->M[ubicacion+3];
                printf("%X ", op.terByte&0xFF);
            }else
                printf("   ");
        }else
            printf("   ");
    }else
        printf("   ");
    return op;
}

TOp guardaOperandos(TMV *mv, char tamOp, int ubicacion){
    short int tam;
    TOp op;

    //ubicacion= (mv->R[5] & 0xFFFF0000) | ubicacion;
    op.tipo= tamOp;
    tam= (short int) ( (~tamOp) & 0b11);

    if (tam>0){
        op.priByte= mv->M[ubicacion+1];
        if(tam>1){
            op.segByte= mv->M[ubicacion+2];
            if(tam>2)
                op.terByte= mv->M[ubicacion+3];
        }
    }
    return op;
}

void leeInstruccion(TMV *mv, short int *codOp, char *tamOpA, char *tamOpB){
    char instruc;
    int ubicacion;

    ubicacion= baseMasOffset(mv->R[5]) ;
    instruc=  mv->M[ubicacion];

    *codOp= instruc & 0b11111;
    *tamOpA= (instruc >> 4) & 0b11;
    *tamOpB= (instruc >> 6) & 0b11;

    printf("\nLEYO INSTRUCCION: %X\n",instruc);
}

void setOp(TMV *mv, TOp *op, int nro){
    int baseEnReg, offset, offsetReg, ubicacion;
    unsigned int regNro;

    switch(op->tipo){
        case(0b00): //memoria
            baseEnReg=  0 | mv->R[ (unsigned int) (op->priByte & 0b1111) ]; //usually DS, osea 0x1
            //ACLARACION: SI O SI EL REGISTRO AL Q VA TIENE Q APUNTAR A UN LUGAR EN MEMORIA; RELATIVA AL COMIENZO DE UN SEGMENTO (TDS). pag 3 pdf lenguahe asm
            offset= (op->segByte&0xFF);
            offset= (offset<<8) | (op->terByte&0xFF); //suma el offset que lleva el operando en si
            offset= offset<<16; offset= offset>>16; //propaga el signo en caso de ser negativo
            offsetReg= (int)(baseEnReg & 0xFFFF); //suma el offset intrinseco del registro
            offsetReg= offsetReg<<16; offsetReg= offsetReg>>16; //propaga el signo en caso de ser negativo
            offset= (int) offset;
            offset+= offsetReg;

            baseEnReg= (baseEnReg>>16)& 0xFFFF ;
            if (baseEnReg!=baseMasOffset(mv->R[1])){ //IF NOT SAME POSICION Q el indice de DS en LA TDS
                printf("ERROR. El setOp quiere entrar a otro Segmento q no es DS. (%X)\n",baseEnReg);
                mv->error[0]=1;
                break;
            }

            ubicacion= ((mv->TDS[ baseEnReg ]>>16) & 0xFFFF) + offset;
            if ( offset<0 || (ubicacion >= ((mv->TDS[ baseEnReg ]>>16) + (mv->TDS[ baseEnReg ] & 0xFFFF))) ){ //IF NOT se cae del DS
                mv->error[0]=1;
                break;
            }
            //printf("el int q esta por ser copiado es %d\n", nro);
            mv->M[ubicacion] = (nro>>24 & 0xFF);
            mv->M[ubicacion+1] = (nro>>16 & 0xFF);
            mv->M[ubicacion+2] = (nro>>8 & 0xFF);
            mv->M[ubicacion+3] = (nro & 0xFF);
            break;

        case(0b10): //registro
            regNro=  (unsigned int) (op->priByte & 0b1111);
            if (regNro<0 || regNro>15){
                printf("ERROR. Se quiere meter a un registro <0 o >15 \n");
                mv->error[2]=1;
                break;
            }
            switch(op->priByte>>4 & 0b11){ //evalua los bits en las posiciones 5 y 6, contando de menor a mayor. Sector del registro a leer
                case(0b00): //EAX 4bytes mas significativos
                    mv->R[ regNro ] = nro;
                    break;

                case(0b01): //AL, lower byte
                    mv->R[regNro] = (mv->R[regNro] & 0xFFFFFF00);
                    mv->R[regNro] = mv->R[regNro] + (nro & 0xFF); //solo se guardan los menos significativos
                    break;

                case(0b10): //AH, higher (low) byte
                    mv->R[regNro] = (mv->R[regNro] & 0xFFFF00FF);
                    mv->R[regNro] = mv->R[regNro] + ((nro<<8) & 0xFF00); //solo se guardan los menos significativos
                    break;

                case(0b11): //AX, los 2 bytes menos significativos
                    mv->R[regNro] = (mv->R[regNro] & 0xFFFF0000);
                    mv->R[regNro] = mv->R[regNro] + (nro & 0xFFFF); //solo se guardan los menos significativos
                    break;

                default:
                    printf("ERRRRRRRRRouououuououoououououououuouuR no deberia poder siquiera llegar aca");
                    mv->error[2]=1;
                    break;
            }
            break;

        default: //nulo
            printf("ERRRRRRRRRouououuououoououououououuouuR no deberia llegar aca, no se puede setear un tipo de dato inmediato/nulo");
            mv->error[2]=1;
            break;
    }
}

int getOp(TMV mv, TOp op){
    int aux=0, baseEnReg, offset, offsetReg, ubiMem;

    switch(op.tipo){
        case(0b00): //memoria
            baseEnReg=  mv.R[ (unsigned int) (op.priByte & 0b1111) ]; //usually DS, osea 0x1
            //ACLARACION: SI O SI EL REGISTRO AL Q VA TIENE Q APUNTAR A UN LUGAR EN MEMORIA; RELATIVA AL COMIENZO DE UN SEGMENTO (TDS). pag 3 pdf lenguahe asm

            offset= (op.segByte&0xFF);
            offset= (offset<<8) | (op.terByte&0xFF); //suma el offset que lleva el operando en si
            offset= offset<<16; offset= offset>>16; //propaga el signo en caso de ser negativo
            offsetReg= (int)(baseEnReg & 0xFFFF); //suma el offset intrinseco del registro, SUPONGO Q NO PUEDE SER NEGATIVO PQ IRIA AL SEGMENTO ANTERIOR, NO SENSE
            offsetReg= offsetReg<<16; offsetReg= offsetReg>>16; //propaga el signo en caso de ser negativo
            offset= (int) offset;
            offset+= offsetReg;

            baseEnReg= (baseEnReg>>16)& 0xFFFF ;
            if (baseEnReg!=1){ //IF NOT SEGUNDA POSICION DE LA TDS (DS)
                mv.error[0]=1;
                break;
            }

            ubiMem= ((mv.TDS[ baseEnReg ] >>16) & 0xFFFF) + offset;
            if ( offset<0 || (ubiMem >= ((mv.TDS[ baseEnReg ]>>16) + (mv.TDS[ baseEnReg ] & 0xFFFF))) ){ //IF NOT se cae del DS
                mv.error[0]=1;
                break;
            }

            aux= (mv.M[ubiMem] & 0xFF);
            aux= (aux<<8) | (mv.M[ubiMem+1] & 0xFF);
            aux= (aux<<8) | (mv.M[ubiMem+2] & 0xFF);
            aux= (aux<<8) | (mv.M[ubiMem+3] & 0xFF);
            break;

        case(0b01): //inmediato
            aux= (op.priByte&0xFF);
            aux= (aux<<8) | (op.segByte&0xFF);
            aux= (int) aux;
            aux= aux<<16;   aux= aux>>16; //propaga signo a 4 bytes
            break;

        case(0b10): //registro
            aux=  mv.R[ (unsigned int) (op.priByte & 0b1111) ];
            if ( (op.priByte & 0xF)<0 || (op.priByte & 0xF)>15 ){ ///CHEQUEAR SI FUNCIONA
                printf("ERROR. Se quiere meter a un registro <0 o >15 \n");
                mv.error[2]=1;
                break;
            }
            switch(op.priByte>>4 & 0b11){ //evalua los bits en las posiciones 5 y 6, contando de menor a mayor. Sector del registro a leer
                case(0b00): //EAX 4bytes mas significativos
                    break;

                case(0b01): //AL, lower byte
                    aux= (aux & 0xFF);
                    aux= aux<<24;   aux= aux>>24; //propaga signo a 4 bytes
                    break;

                case(0b10): //AH, higher (low) byte
                    aux= (aux & 0xFF00);
                    aux= aux<<16;   aux= aux>>24; //propaga signo a 4 bytes
                    break;

                case(0b11): //AX, los 2 bytes menos significativos
                    aux= (aux & 0xFFFF);
                    aux= aux<<16;   aux= aux>>16; //propaga signo a 4 bytes
                    break;

                default:
                    printf("ERRRRRRRRROOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOR no deberia llegar aca, hice mal la mascara");
                    mv.error[2]=1;
                    break;
            }
            break;

        default: //nulo
            printf("ERRRRRRRRROOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOR pidio getOp de un tipo nulo");
            mv.error[2]=1;
            break;
    }
    return aux;
}

int modificaCC(int aux){
    int CC;

    if (aux<0){
        CC=2;
    }else {
        if(aux==0){
            CC=1;
        }else{
            CC=0;
        }
    }
    CC= CC<<30;

    return CC;
}

void MOV(TMV *mv, TOp op[2]){
    setOp(mv, &op[0], getOp(*mv, op[1]));
    //printf("se hizo MOV un operando de tipo %X en op[0] con valores en hexa %X %X %X\n", op[0].tipo, op[0].priByte, op[0].segByte, op[0].terByte);
    //printf("y guardo un: %d \n", getOp(*mv,op[0]));
}

void ADD(TMV *mv, TOp op[2]){
    int aux;
    //printf("la suma es entre: %d y %d\n", getOp(*mv,op[0]), getOp(*mv,op[1]));
    aux= getOp(*mv, op[0]) + getOp(*mv, op[1]);
    setOp(mv, &op[0], aux);
    //printf("La suma devolvio %d. Y quedo un operando de tipo %X en op[0] con valores en hexa %X %X %X\n", aux, op[0].tipo, op[0].priByte, op[0].segByte, op[0].terByte);
    //printf("y guardo un: %d \n", getOp(*mv,op[0]));
    mv->R[8]= modificaCC(getOp(*mv,op[0]));
    //printf("y en CC guardo un: %X \n", mv->R[8]);
}

void SUB(TMV *mv, TOp op[2]){
    int aux;
    //printf("la resta es entre: %d y %d\n", getOp(*mv,op[0]), getOp(*mv,op[1]));
    aux= getOp(*mv, op[0]) - getOp(*mv, op[1]); //hay q chequear si no se le complica el complemento A2, si se le complica probar poniendo casteando en otras variables (int)
    setOp(mv, &op[0], aux);
    //printf("La resta devolvio %d. Y quedo un operando de tipo %X en op[0] con valores en hexa %X %X %X\n", aux, op[0].tipo, op[0].priByte, op[0].segByte, op[0].terByte);
    //printf("y guardo un: %d \n", getOp(*mv,op[0]));
    mv->R[8]= modificaCC(getOp(*mv,op[0]));
    //printf("y en CC guardo un: %X \n", mv->R[8]);
}

void SWAP(TMV *mv, TOp op[2]){
    int aux;
    aux= getOp(*mv, op[0]);
    setOp(mv, &op[0], getOp(*mv, op[1]));
    setOp(mv, &op[1], aux);
    //printf("se hizo SWAP y quedo en op[0] de tipo %X con valores en hexa %X %X %X\n", op[0].tipo, op[0].priByte, op[0].segByte, op[0].terByte);
    //printf("se hizo SWAP y quedo en op[1] de tipo %X con valores en hexa %X %X %X\n", op[1].tipo, op[1].priByte, op[1].segByte, op[1].terByte);
    //printf("y guardo un op[0]: %d \n", getOp(*mv,op[0]));
    //printf("y guardo un op[1]: %d \n", getOp(*mv,op[1]));
}

void MUL(TMV *mv, TOp op[2]){
    int aux;
    //printf("la mult es entre: %d y %d\n", getOp(*mv,op[0]), getOp(*mv,op[1]));
    aux= getOp(*mv, op[0]) * getOp(*mv, op[1]);
    setOp(mv, &op[0], aux);
    //printf("La mult devolvio %d. Y quedo un operando de tipo %X en op[0] con valores en hexa %X %X %X\n", aux, op[0].tipo, op[0].priByte, op[0].segByte, op[0].terByte);
    //printf("y guardo un: %d \n", getOp(*mv,op[0]));
    mv->R[8]= modificaCC(getOp(*mv,op[0]));
    //printf("y en CC guardo un: %X \n", mv->R[8]);
}

void DIV(TMV *mv, TOp op[2]){  //exit o que
    int aux;
    if(getOp(*mv, op[1]) != 0){
        mv->R[9]= getOp(*mv, op[0]) % getOp(*mv, op[1]); //guarda el resto en �ac?
        aux= getOp(*mv, op[0]) / getOp(*mv, op[1]); //supongo q 5/8 da 0??????? y resto 5
        setOp(mv, &op[0], aux);
        mv->R[8]= modificaCC(getOp(*mv,op[0]));
        //printf("y en CC guardo un: %X \n", mv->R[8]);
    }else
        mv->error[1]=1;
}

void CMP(TMV *mv, TOp op[2]){
    int aux;
    //printf("el CMP es entre: %d y %d\n", getOp(*mv,op[0]), getOp(*mv,op[1]));
    aux= getOp(*mv, op[0]) - getOp(*mv, op[1]); //hay q chequear si no se le complica el complemento A2, si se le complica probar poniendo casteando en otras variables (int)
    //printf("La resta devolvio %d.\n", aux);
    mv->R[8]= modificaCC(aux);
    //printf("y en CC guardo un: %X \n", mv->R[8]);
}

void SHL(TMV *mv, TOp op[2]){
    int aux;
    aux= getOp(*mv, op[0]) << getOp(*mv, op[1]);
    setOp(mv, &op[0], aux);
    mv->R[8]= modificaCC( getOp(*mv, op[0]) );
    //printf("y en CC guardo un: %X \n", mv->R[8]);
}

void SHR(TMV *mv, TOp op[2]){
    int aux;
    aux= getOp(*mv, op[0]) >> getOp(*mv, op[1]); //propagacion de signo?
    setOp(mv, &op[0], aux);
    mv->R[8]= modificaCC( getOp(*mv, op[0]) );
    //printf("y en CC guardo un: %X \n", mv->R[8]);
}

void AND(TMV *mv, TOp op[2]){
    int aux;
    aux= getOp(*mv, op[0]) & getOp(*mv, op[1]);
    setOp(mv, &op[0], aux);
    mv->R[8]= modificaCC( getOp(*mv, op[0]) );
    //printf("y en CC guardo un: %X \n", mv->R[8]);
}

void OR(TMV *mv, TOp op[2]){
    int aux;
    aux= getOp(*mv, op[0]) | getOp(*mv, op[1]);
    setOp(mv, &op[0], aux);
    mv->R[8]= modificaCC( getOp(*mv, op[0]) );
    //printf("y en CC guardo un: %X \n", mv->R[8]);
}

void XOR(TMV *mv, TOp op[2]){
    int aux;
    aux= getOp(*mv, op[0]) ^ getOp(*mv, op[1]);
    setOp(mv, &op[0], aux);
    mv->R[8]= modificaCC( getOp(*mv, op[0]) );
    //printf("y en CC guardo un: %X \n", mv->R[8]);
}

void RND(TMV *mv, TOp op[2]){ //preguntar
    int aux;
    aux= rand() % ( getOp(*mv, op[1]) +1); // Se suma 1 para incluir el valor de OP[1]
    setOp(mv, &op[0], aux);
}

void guardaEnUbi(TMV *mv, int ubicacion, unsigned int tam, int dato){
    switch(tam){ //esto podria ser un while decreciente y quedaria muy clean
        case 1:
            mv->M[ubicacion]= (dato & 0xFF) ;
            break;
        case 2:
            mv->M[ubicacion]= ((dato>>8 )& 0xFF) ;
            mv->M[ubicacion+1]= (dato & 0xFF) ;
            break;
        case 3:
            mv->M[ubicacion]= ((dato>>16 )& 0xFF) ;
            mv->M[ubicacion+1]= ((dato>>8 )& 0xFF) ;
            mv->M[ubicacion+2]= (dato & 0xFF) ;
            break;
        case 4: //0x12378623
            mv->M[ubicacion]= ((dato>>24 )& 0xFF) ; //12
            //printf("mv->M[%d]= %X\n",ubicacion, mv->M[ubicacion]);
            mv->M[ubicacion+1]= ((dato>>16 )& 0xFF) ; //37
            //printf("mv->M[%d]= %X\n",ubicacion+1, mv->M[ubicacion+1]);
            mv->M[ubicacion+2]= ((dato>>8 )& 0xFF) ; //86
            //printf("mv->M[%d]= %X\n",ubicacion+2, mv->M[ubicacion+2]);
            mv->M[ubicacion+3]= (dato & 0xFF) ; //23
            //printf("mv->M[%d]= %X\n",ubicacion+3, mv->M[ubicacion+3]);
            break;
        default:
            printf("ERROR: Mas de 4 bytes por celda asignado");
            mv->error[2]=1;
            break;
    }
}

void leeDeUbi(TMV *mv, int ubicacion, unsigned int tam, int *dato){
    int datoaux=0;
    if (tam>0){ //esto podria ser un while creciente y quedaria muy clean
        datoaux= mv->M[ubicacion] & 0xFF ;
        if(tam>1){
            datoaux= (datoaux<<8) | (mv->M[ubicacion+1] & 0xFF) ;
            if (tam>2){
                datoaux= (datoaux<<8) | (mv->M[ubicacion+2] & 0xFF) ;
                if (tam>3){
                    datoaux= (datoaux<<8) | (mv->M[ubicacion+3] & 0xFF) ;
                    if (tam>4){
                        printf("ERROR: Mas de 4 bytes por celda asignado");
                        mv->error[2]=1;
                    }
                }
            }
        }
    }
    *dato= datoaux;
}

void leeHeaderImagen(FILE* arch, unsigned short int *version, unsigned short int *tamRAM, char identificador[6]){
    short int cont=-1;
    char aux;

    do{ //LEE EL HEADER DE LA IMAGEN
        cont++;
        fread(&aux,sizeof(char),1,arch);
        //printf("%X ", aux); //nomas chequeando

        switch(cont){
            case 0 ... 4:
                identificador[cont]=aux;
                break;
            case 5:
                identificador[cont]='\0';
                *version= (short int)aux;
                //printf("(<- version) ");
                break;
            case 6:
                *tamRAM= ((aux & 0xFF)<<8);
                break;
            case 7:
                *tamRAM= *tamRAM | (aux & 0xFF);
                break;
            default:
                printf("se comio el primer byte del codigo\n");
                break;
        }
        printf("paso nro: %u // aux: #%u '%c LEYO EL HEADER CON version:%u, tamRAM:%u, identif:%s \n",cont , aux, aux, *version, *tamRAM, identificador);
    }while (!feof(arch) && cont<7);
    printf("LEYO EL HEADER CON version:%u, tamRAM:%u, identif:%s \n",*version, *tamRAM, identificador);
}

void guardaImagen(TMV mv){
    FILE* arch;
    unsigned short int version=0, tamRAM;
    char identificador[6]; // son 6 por el \n??
    int i, aux;

    version=1; //si llego a un breakpoint es porque es version 2
    sprintf(identificador, "VMI24");
    tamRAM= mv.ramKiB;

    printf("LO QUE VA A GRABAR ES version:%u, tamRAM:%u, identif:%s \n",version, tamRAM, identificador);
    if ( (arch= fopen(mv.nombreImg,"wb")) != NULL){
        printf("arranca a grabar \n");
        //fwrite(identificador, 1/*sizeof(char)*/, 5, arch); //guarda en los bytes 0 a 4 VMIej ?? hope so. antes hacia bucle de a 1 con while (4>=i++) y identificador[i-1]
        i=0;
        while (4>=i++)
            fwrite(&identificador[i-1], 1/*sizeof(char)*/, 1, arch);
        printf("grabo el identif \n");
        fwrite(&version, 1, 1, arch); //el bytes menos significativos de version
        identificador[0]= ( (tamRAM>>8) & 0xFF);
        identificador[1]= (tamRAM & 0xFF); //uso el identificador [0] y [1] como char auxiliar, para no crear otra variable
        fwrite(&identificador[0], 1, 1, arch);
        fwrite(&identificador[1], 1, 1, arch); //los dos bytes menos significativos de tamRAM
        printf("grabo la version y la ram \n");

        for (i=0; i<16; i++){
            aux= changeEndian( mv.R[i] );
            fwrite(&aux, 4/*sizeof(int)*/, 1, arch); //chequear que no me arruine el little y big endian
        }
        for (i=0; i<8; i++){
            aux= changeEndian( mv.TDS[i] );
            fwrite(&aux, 4/*sizeof(int)*/, 1, arch);
        }
        tamRAM= mv.ramKiB*1024;
        for (i=0; i<tamRAM; i++)
            fwrite(&mv.M[i], 1/*sizeof(char)*/, 1, arch);
        printf("imagen guardada\n");
        fclose(arch);
    }else
        printf("El archivo .vmi fallo al abrir para su grabado\n");


}

void SYS(TMV *mv, TOp op[2]){
    int i, corri, j, ubicacion, dato;
    unsigned int CL, CH;
    char aux;

    CL=(mv->R[12] & 0xFF);
    CH=((mv->R[12] >>8) & 0xFF);
    ubicacion=  ( (mv->TDS[ (mv->R[13]>>16) & 0xF ]>>16 ) & 0xFFFF ) + ( mv->R[13] & 0xFFFF )  ; //base+offset. Usa el EDX como recipiente de lugar en memoria. NO CHEQUEA QUE SE CAIGA!
    switch (getOp(*mv, op[1])){
        case 1: //LEE
            //printf("bien, entra a leer\n");
            switch (mv->R[10] & 0xFFFF){ //chequea AL
                case 1: //DECIMAL
                    for(i=0; i< CL; i++){ //cantidad de iteraciones = CL
                        dato=0;
                        printf("Dato de tama�o %d bytes, de la celda %d en decimal: ",CH,i);
                        scanf("%d", &dato); //decimal, tama�o de lectura = CH
                        printf("\n");
                        guardaEnUbi(mv, ubicacion, CH, dato);
                        ubicacion+=CH;
                    }
                    break;
                case 2: //CHAR
                    for(i=0; i< CL; i++){ //cantidad de iteraciones = CL
                        dato=0;
                        printf("Dato de tama�o %d bytes, de la celda %d en char: ",CH,i);
                        scanf("%c", &dato); //char, tama�o de lectura = CH
                        printf("\n");
                        guardaEnUbi(mv, ubicacion, CH, dato);
                        ubicacion+=CH;
                    }
                    break;
                case 4: //OCTAL
                    for(i=0; i< CL; i++){ //cantidad de iteraciones = CL
                        dato=0;
                        printf("Dato de tama�o %d bytes, de la celda %d en octal: ",CH,i);
                        scanf("%o", &dato); //octal, tama�o de lectura = CH
                        printf("\n");
                        guardaEnUbi(mv, ubicacion, CH, dato);
                        ubicacion+=CH;
                    }
                    break;
                case 8://hexa
                    for(i=0; i< CL; i++){ //cantidad de iteraciones = CL
                        dato=0;
                        printf("Dato de tama�o %d bytes, de la celda %d en hexa: ",CH,i);
                        scanf("%X", &dato); //hexa, tama�o de lectura = CH
                        printf("\n");
                        guardaEnUbi(mv, ubicacion, CH, dato);
                        ubicacion+=CH;
                    }
                    break;
                default:
                    printf("ERROR, 'AL' MAL SETEADO\n");
                    mv->error[2]=1;
            }
            break;
        case 2: //ESCRIBE
            if(((mv->R[10] & 0xFFFF) > 0) && ((mv->R[10] & 0xFFFF) < 16)){
                for(i=0; i< CL; i++){ //cantidad de iteraciones = CL
                    leeDeUbi(mv, ubicacion, CH, &dato);
                    printf("Dato de tama�o %d bytes, celda nro %d, en M[%X]: ",CH,i,ubicacion);
                    if ((mv->R[10] & 0x8)==8)
                        printf("%%%X ", dato);
                    if ((mv->R[10] & 0x4)==4)
                        printf("@%o ", dato);
                    if (((mv->R[10] & 0x2)==2)){
                        corri= 8*(CH-1);
                        for(j=0; j<CH; j++){
                            aux= (dato>>corri);
                            if ((aux>31) && (aux<127))
                                printf("'%c ", aux);
                            else
                                printf("'. ");
                            corri-= 8;
                        }
                    }
                    if ((mv->R[10] & 0x1)==1)
                        printf("#%d ", dato);
                    printf("\n");
                    ubicacion+=CH;
                }
            }else{
                printf("ERROR, 'AL' MAL SETEADO\n");
                mv->error[2]=1;
            }

            break;
        case 3: ///STRING READ

            break;
        case 4: ///STRING WRITE

            break;
        case 7: ///CLEAR SCREEN

            break;
        case 15: //BREAKPOINT
                if (strcmp(mv->nombreImg,"no") != 0){
                    char finciclo= 0;
                    char auxc;
                    guardaImagen(*mv);
                    //printf("guarda imagen\n");
                    fflush(stdin);
                    scanf("%c",&auxc);

                    while (auxc!='g' && auxc!='q' && auxc!='G' && auxc!='Q' && finciclo!=1){
                        TFunc func;
                        inicializaVectorFunc(func);

                        if ( hayError(*mv)==0 && (mv->R[5] & 0xffff0000)==0 && (mv->R[5] & 0x0000ffff)<(mv->TDS[0] & 0x0000ffff)){
                            //if IP2primerosBytes==0 es decir apunta al TDS[0], asumimos q ahi esta el CS && offset<size
                            avanzaUnaInstruccion(mv,func);
                            guardaImagen(*mv);
                            //printf("guarda imagen\n");

                            scanf("%c",&auxc);
                        }
                        else
                            finciclo=1;
                    }
                    if(auxc=='q' || auxc=='Q')
                        STOP(mv, op);
                }
            break;
        default:
            printf("OPERADOR INVALIDO AL LLAMAR AL SYS \n");
            mv->error[2]=1;
            break;
    }
}

void JMP(TMV *mv, TOp op[2]){
    if (getOp(*mv, op[1]) < ( mv->TDS[0] & 0xFFFF ) ){ // comparo q el corrimiento desde el inicio del CS (el cual asumo en TDS[0] Y tmb asumo q empieza en mv->M[0] ), no se caiga
        mv->R[5]= getOp(*mv, op[1]);
    }else
        mv->error[0]=1;
}

void JZ(TMV *mv, TOp op[2]){
    if ( getOp(*mv, op[1]) < ( mv->TDS[0] & 0xFFFF ) ){ // comparo q el corrimiento desde el inicio del CS (el cual asumo en TDS[0] Y tmb asumo q empieza en mv->M[0] ), no se caiga
        if (mv->R[8] == 0x40000000) // chequea q N=0 y Z=1
            mv->R[5]= getOp(*mv, op[1]);
        //else
            //printf("no hizo el salto\n");
    }else
        mv->error[0]=1;
}

void JP(TMV *mv, TOp op[2]){
    if ( getOp(*mv, op[1]) < ( mv->TDS[0] & 0xFFFF ) ){ // comparo q el corrimiento desde el inicio del CS (el cual asumo en TDS[0] Y tmb asumo q empieza en mv->M[0] ), no se caiga
        if (mv->R[8] == 0) // chequea q N=0 y Z=0
            mv->R[5]= getOp(*mv, op[1]);
        //else
            //printf("no hizo el salto\n");
    }else
        mv->error[0]=1;
}

void JN(TMV *mv, TOp op[2]){
    if ( getOp(*mv, op[1]) < ( mv->TDS[0] & 0xFFFF ) ){ // comparo q el corrimiento desde el inicio del CS (el cual asumo en TDS[0] Y tmb asumo q empieza en mv->M[0] ), no se caiga
        if (mv->R[8] == 0x80000000) // chequea q N=1 y Z=0
            mv->R[5]= getOp(*mv, op[1]);
        //else
            //printf("no hizo el salto\n");
    }else
        mv->error[0]=1;
}

void JNZ(TMV *mv, TOp op[2]){
    if ( getOp(*mv, op[1]) < ( mv->TDS[0] & 0xFFFF ) ){ // comparo q el corrimiento desde el inicio del CS (el cual asumo en TDS[0] Y tmb asumo q empieza en mv->M[0] ), no se caiga
        if ( (mv->R[8] == 0x80000000) | (mv->R[8] == 0) ) // chequea q (N=1 y Z=0) o (N=0 y Z=0)
            mv->R[5]= getOp(*mv, op[1]);
        //else
            //printf("no hizo el salto\n");
    }else
        mv->error[0]=1;
}

void JNP(TMV *mv, TOp op[2]){
    if ( getOp(*mv, op[1]) < ( mv->TDS[0] & 0xFFFF ) ){ // comparo q el corrimiento desde el inicio del CS (el cual asumo en TDS[0] Y tmb asumo q empieza en mv->M[0] ), no se caiga
        if ( (mv->R[8] == 0x80000000) | (mv->R[8] == 0x40000000) ) // chequea q (N=1 y Z=0) o (N=0 y Z=1)
            mv->R[5]= getOp(*mv, op[1]);
        //else
            //printf("no hizo el salto\n");
    }else
        mv->error[0]=1;
}

void JNN(TMV *mv, TOp op[2]){
    if ( getOp(*mv, op[1]) < ( mv->TDS[0] & 0xFFFF ) ){ // comparo q el corrimiento desde el inicio del CS (el cual asumo en TDS[0] Y tmb asumo q empieza en mv->M[0] ), no se caiga
        if ( (mv->R[8] == 0x40000000) | (mv->R[8] == 0) ) // chequea q (N=0 y Z=1) o (N=0 y Z=0)
            mv->R[5]= getOp(*mv, op[1]);
        //else
            //printf("no hizo el salto\n");
    }else
        mv->error[0]=1;
}

void LDL(TMV *mv, TOp op[2]){
    mv->R[9]= mv->R[9] & 0xFFFF0000;
    mv->R[9]= mv->R[9] | (getOp(*mv, op[1]) & 0xFFFF);
    //printf("luego de modificar los 2 bytes pekes en AC guardo= %%%X\n",mv->R[9]);
}

void LDH(TMV *mv, TOp op[2]){
    mv->R[9]= mv->R[9] & 0x0000FFFF;
    mv->R[9]= mv->R[9] | ( (getOp(*mv, op[1]) <<16) & 0xFFFF0000);
    //printf("luego de modificar los 2 bytes grandes en AC guardo= %%%X\n",mv->R[9]);
}

void NOT(TMV *mv, TOp op[2]){
    setOp(mv, &op[1], ~getOp(*mv, op[1]));
    mv->R[8]= modificaCC( getOp(*mv, op[1]) );
    //printf("y en CC guardo un: %X \n", mv->R[8]);
}

void STOP(TMV *mv, TOp op[2]){
    mv->error[3]=1;
}

/*
void PUSH (TMV *mv, TOp op[2]){    //NUEVO   el getop devuelve la info en 4 bytes
    mv->R[6] =- 4
    if (mv->R[6] < mv->R[3])
        mv->error[4]= 1;
    else{
        TOp aux;
        aux.tipo= 0b10;
        aux.priByte= getOp(mv, mv->R[6]);
        aux.segByte=
        aux.terByte=
        op[1] = cambiaEndian();
        offset= offset<<16; offset= offset>>16; //propaga el signo
    }

}*/

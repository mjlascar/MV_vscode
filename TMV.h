typedef struct _MV{
    char *M;
    unsigned int *TDS;
    int R[16];
    short int ramKiB;
    char nombreImg[25];
    char error[7]; // [0] = fallo de segmento, [1] = division por cero, [2] = instruccion invalida
                   // [3] = STOP, [4] = STACK OVERFLOW, [5] = STACK UNDERFLOW, [6] = memoria insuficiente
}TMV;

typedef struct _OP{
    char tipo, priByte, segByte, terByte;
}TOp;

typedef void (*TFunc[32])(TMV *mv, TOp op[2]);

TMV createMV(char* arch, char* vmiName, unsigned int SIZE_MEM);
TMV levantaVMI(char* arch);
int baseMasOffset(int dirLog);
void leeHeaderV1(FILE* arch, unsigned short int *tamCodigo, char identificador[6]);
void leeHeaderV2(FILE* arch, unsigned short int *tamCodigo, unsigned short int *tamDS, unsigned short int *tamES, unsigned short int *tamSS, unsigned short int *tamKS, unsigned short int *offsetEP, char identificador[6]);
void codigoAMemoria(TMV*, FILE*, unsigned short int);
void KSaMemoria(TMV*, FILE*, unsigned short int);
void inicializaVectorFunc(TFunc);
void inicializaVectorFuncDisass(char *funcDisass[32]);
int hayError(TMV);
void disAssembler(TMV*, char*);
void imprimeOp(TMV mv, TOp op, char str[6]);
void setStrDeReg(TMV mv, char byte, char str[6]);
void avanzaUnaInstruccion(TMV *mv, TFunc func);
void execute(TMV*);
TOp guardaOperandos(TMV*, char, int);
TOp guardaOperandosYDisass(TMV*, char, int);
void leeInstruccion(TMV*, short int*, char*, char*);
void setOp(TMV *, TOp *, int);
int getOp(TMV, TOp);
int modificaCC(int aux);

void MOV(TMV *mv, TOp op[2]);
void ADD(TMV *mv, TOp op[2]);
void SUB(TMV *mv, TOp op[2]);
void SWAP(TMV *mv, TOp op[2]);
void MUL(TMV *mv, TOp op[2]);
void DIV(TMV *mv, TOp op[2]);
void CMP(TMV *mv, TOp op[2]);
void SHL(TMV *mv, TOp op[2]);
void SHR(TMV *mv, TOp op[2]);
void AND(TMV *mv, TOp op[2]);
void OR(TMV *mv, TOp op[2]);
void XOR(TMV *mv, TOp op[2]);
void RND(TMV *mv, TOp op[2]);
void guardaEnUbi(TMV*, int, unsigned int, int);
void leeDeUbi(TMV*, int, unsigned int, int*);
void leeHeaderImagen(FILE* arch, unsigned short int *version, unsigned short int *tamRAM, char identificador[6]);
void guardaImagen(TMV);
void SYS(TMV *mv, TOp op[2]);
void JMP(TMV *mv, TOp op[2]);
void JZ(TMV *mv, TOp op[2]);
void JP(TMV *mv, TOp op[2]);
void JN(TMV *mv, TOp op[2]);
void JNZ(TMV *mv, TOp op[2]);
void JNP(TMV *mv, TOp op[2]);
void JNN(TMV *mv, TOp op[2]);
void LDL(TMV *mv, TOp op[2]);
void LDH(TMV *mv, TOp op[2]);
void NOT(TMV *mv, TOp op[2]);
void STOP(TMV *mv, TOp op[2]);
void PUSH(TMV *mv, TOp op[2]);
void POP(TMV *mv, TOp op[2]);
void CALL(TMV *mv, TOp op[2]);
void RET(TMV *mv, TOp op[2]);
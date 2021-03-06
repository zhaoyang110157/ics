#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "y64asm.h"

line_t *line_head = NULL;
line_t *line_tail = NULL;
int lineno = 0;

#define err_print(_s, _a ...) do { \
  if (lineno < 0) \
    fprintf(stderr, "[--]: "_s"\n", ## _a); \
  else \
    fprintf(stderr, "[L%d]: "_s"\n", lineno, ## _a); \
} while (0);


int64_t vmaddr = 0;    /* vm addr */

/* register table */
const reg_t reg_table[REG_NONE] = {
    {"%rax", REG_RAX, 4},
    {"%rcx", REG_RCX, 4},
    {"%rdx", REG_RDX, 4},
    {"%rbx", REG_RBX, 4},
    {"%rsp", REG_RSP, 4},
    {"%rbp", REG_RBP, 4},
    {"%rsi", REG_RSI, 4},
    {"%rdi", REG_RDI, 4},
    {"%r8",  REG_R8,  3},
    {"%r9",  REG_R9,  3},
    {"%r10", REG_R10, 4},
    {"%r11", REG_R11, 4},
    {"%r12", REG_R12, 4},
    {"%r13", REG_R13, 4},
    {"%r14", REG_R14, 4}
};
const reg_t* find_register(char *name)
{
    int i;
    for (i = 0; i < REG_NONE; i++)
        if (!strncmp(name, reg_table[i].name, reg_table[i].namelen))
            return &reg_table[i];
    return NULL;
}


/* instruction set */
instr_t instr_set[] = {
    {"nop", 3,   HPACK(I_NOP, F_NONE), 1 },
    {"halt", 4,  HPACK(I_HALT, F_NONE), 1 },
    {"rrmovq", 6,HPACK(I_RRMOVQ, F_NONE), 2 },
    {"cmovle", 6,HPACK(I_RRMOVQ, C_LE), 2 },
    {"cmovl", 5, HPACK(I_RRMOVQ, C_L), 2 },
    {"cmove", 5, HPACK(I_RRMOVQ, C_E), 2 },
    {"cmovne", 6,HPACK(I_RRMOVQ, C_NE), 2 },
    {"cmovge", 6,HPACK(I_RRMOVQ, C_GE), 2 },
    {"cmovg", 5, HPACK(I_RRMOVQ, C_G), 2 },
    {"irmovq", 6,HPACK(I_IRMOVQ, F_NONE), 10 },
    {"rmmovq", 6,HPACK(I_RMMOVQ, F_NONE), 10 },
    {"mrmovq", 6,HPACK(I_MRMOVQ, F_NONE), 10 },
    {"addq", 4,  HPACK(I_ALU, A_ADD), 2 },
    {"subq", 4,  HPACK(I_ALU, A_SUB), 2 },
    {"andq", 4,  HPACK(I_ALU, A_AND), 2 },
    {"xorq", 4,  HPACK(I_ALU, A_XOR), 2 },
    {"jmp", 3,   HPACK(I_JMP, C_YES), 9 },
    {"jle", 3,   HPACK(I_JMP, C_LE), 9 },
    {"jl", 2,    HPACK(I_JMP, C_L), 9 },
    {"je", 2,    HPACK(I_JMP, C_E), 9 },
    {"jne", 3,   HPACK(I_JMP, C_NE), 9 },
    {"jge", 3,   HPACK(I_JMP, C_GE), 9 },
    {"jg", 2,    HPACK(I_JMP, C_G), 9 },
    {"call", 4,  HPACK(I_CALL, F_NONE), 9 },
    {"ret", 3,   HPACK(I_RET, F_NONE), 1 },
    {"pushq", 5, HPACK(I_PUSHQ, F_NONE), 2 },
    {"popq", 4,  HPACK(I_POPQ, F_NONE),  2 },
    {".byte", 5, HPACK(I_DIRECTIVE, D_DATA), 1 },
    {".word", 5, HPACK(I_DIRECTIVE, D_DATA), 2 },
    {".long", 5, HPACK(I_DIRECTIVE, D_DATA), 4 },
    {".quad", 5, HPACK(I_DIRECTIVE, D_DATA), 8 },
    {".pos", 4,  HPACK(I_DIRECTIVE, D_POS), 0 },
    {".align", 6,HPACK(I_DIRECTIVE, D_ALIGN), 0 },
    {NULL, 1,    0   , 0 } //end
};

instr_t *find_instr(char *name)
{
    int i;
    for (i = 0; instr_set[i].name; i++)
	if (strncmp(instr_set[i].name, name, instr_set[i].len) == 0)
	    return &instr_set[i];
    return NULL;
}

/* symbol table (don't forget to init and finit it) */
symbol_t *symtab = NULL;

/*
 * find_symbol: scan table to find the symbol
 * args
 *     name: the name of symbol
 *
 * return
 *     symbol_t: the 'name' symbol
 *     NULL: not exist
 */
symbol_t *find_symbol(char *name)
{
    symbol_t *tmp = symtab;
    while( tmp != tmp ){
        if( strcmp(tmp->name,name) == 0){
	    return tmp;
	}
	tmp = tmp->next;
    }
    return NULL;
}

/*
 * add_symbol: add a new symbol to the symbol table
 * args
 *     name: the name of symbol
 *
 * return
 *     0: success
 *     -1: error, the symbol has exist
 */
int add_symbol(char *name)
{
    symbol_t *symptr = symtab;
    /* check duplicate */
    while( symptr->next != NULL) {//to find the tail;
	symptr = symptr->next;
	
	if(strcmp(symptr->name,name))
		return -1;
    } 

    /* create new symbol_t (don't forget to free it)*/
    symbol_t *newsym;
    newsym = (symbol_t *)malloc(sizeof(symbol_t));
    newsym->name = name;
    newsym->addr = vmaddr;
    newsym->next = NULL;
    /* add the new symbol_t to symbol table */
    symptr->next = newsym;
    return 0;
}

/* relocation table (don't forget to init and finit it) */
reloc_t *reltab = NULL;

/*
 * add_reloc: add a new relocation to the relocation table
 * args
 *     name: the name of symbol
 *
 * return
 *     0: success
 *     -1: error, the symbol has exist
 */
void add_reloc(char *name, bin_t *bin)
{
    /* create new reloc_t (don't forget to free it)*/
    reloc_t *newrel;
    newrel = (reloc_t*)malloc(sizeof(reloc_t));
    newrel->next = NULL;
    newrel->y64bin = bin;
    newrel->name = name;

    /* add the new reloc_t to relocation table */
    reloc_t *relptr = reltab;
    while(relptr->next!=NULL){
   	relptr = relptr->next;
    }
    relptr->next = newrel;
}


/* macro for parsing y64 assembly code */
#define IS_DIGIT(s) ((*(s)>='0' && *(s)<='9') || *(s)=='-' || *(s)=='+')
#define IS_LETTER(s) ((*(s)>='a' && *(s)<='z') || (*(s)>='A' && *(s)<='Z'))
#define IS_COMMENT(s) (*(s)=='#')
#define IS_REG(s) (*(s)=='%')
#define IS_IMM(s) (*(s)=='$')
#define IS_HEX_LETTER(s) ((*(s)>='a' && *(s)<='f') || (*(s)>='A' && *(s)<='F') || (*(s)=='x'))

#define IS_BLANK(s) (*(s)==' ' || *(s)=='\t')
#define IS_END(s) (*(s)=='\0')
#define IS_LBRACKET(s) (*(s)=='(')
#define IS_RBRACKET(s) (*(s)==')')
#define IS_COLON(s) (*(s)==':')
#define IS_DELIM(s) (*(s)==',')

#define SKIP_BLANK(s) do {  \
  while(!IS_END(s) && IS_BLANK(s))  \
    (s)++;    \
} while(0);

/* return value from different parse_xxx function */
typedef enum { PARSE_ERR=-1, PARSE_REG, PARSE_DIGIT, PARSE_SYMBOL, 
    PARSE_MEM, PARSE_DELIM, PARSE_INSTR, PARSE_LABEL} parse_t;

/*
 * parse_instr: parse an expected data token (e.g., 'rrmovq')
 * args
 *     ptr: point to the start of string
 *     inst: point to the inst_t within instr_set
 *
 * return
 *     PARSE_INSTR: success, move 'ptr' to the first char after token,
 *                            and store the pointer of the instruction to 'inst'
 *     PARSE_ERR: error, the value of 'ptr' and 'inst' are undefined
 */
parse_t parse_instr(char **ptr, instr_t **inst)
{
    /* skip the blank */
    SKIP_BLANK(*ptr);
    /* find_instr and check end */
    *inst = find_instr(*ptr);
    /* set 'ptr' and 'inst' */
    if((*inst)!= NULL){
	*ptr += (*inst)->len;
	return PARSE_INSTR; 
    }
    return PARSE_ERR;
}

/*
 * parse_delim: parse an expected delimiter token (e.g., ',')
 * args
 *     ptr: point to the start of string
 *
 * return
 *     PARSE_DELIM: success, move 'ptr' to the first char after token
 *     PARSE_ERR: error, the value of 'ptr' and 'delim' are undefined
 */
parse_t parse_delim(char **ptr, char delim)
{
    /* skip the blank and check */
    SKIP_BLANK(*ptr);

    /* set 'ptr' */
    if(IS_DELIM(*ptr)){
	*ptr += 1;
	return PARSE_DELIM;
    }
    return PARSE_ERR;
}

/*
 * parse_reg: parse an expected register token (e.g., '%rax')
 * args
 *     ptr: point to the start of string
 *     regid: point to the regid of register
 *
 * return
 *     PARSE_REG: success, move 'ptr' to the first char after token, 
 *                         and store the regid to 'regid'
 *     PARSE_ERR: error, the value of 'ptr' and 'regid' are undefined
 */
parse_t parse_reg(char **ptr, regid_t *regid)
{
    /* skip the blank and check */
    SKIP_BLANK(*ptr);
    /* find register */
    bool_t isBracket = FALSE;
    if(IS_LBRACKET(*ptr)){
	isBracket = TRUE;
	*ptr += 1;
    }
    /* set 'ptr' and 'regid' */
    if(IS_REG(*ptr)){
	const reg_t *reg = find_register(*ptr);

	if(reg == NULL){
	    return PARSE_ERR;
	}
	
	*ptr += reg->namelen;
	*regid = reg->id;
	if(isBracket == TRUE){
	    if(IS_RBRACKET(*ptr))
		*ptr += 1;
	    else 
		return PARSE_ERR;
	}
	
	return PARSE_REG;
    }
    return PARSE_ERR;
}

/*
 * parse_symbol: parse an expected symbol token (e.g., 'Main')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *
 * return
 *     PARSE_SYMBOL: success, move 'ptr' to the first char after token,
 *                               and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr' and 'name' are undefined
 */
parse_t parse_symbol(char **ptr, char **name)
{
    /* skip the blank and check */
    SKIP_BLANK(*ptr);
    int slen = 0;
    char *ptmp = *ptr;

    /* allocate name and copy to it */
    if(IS_LETTER(ptmp)){
	while( !(IS_END(ptmp) || IS_BLANK(ptmp) || IS_DELIM(ptmp))){
	    slen ++;
	    ptmp ++;
	}
	*name = (char *)malloc(slen +1);
	*(*name + slen) = '\0';
	strncpy(*name, *ptr, slen);
	*ptr += slen;
	return PARSE_SYMBOL;
    }  
    /* set 'ptr' and 'name' */

    return PARSE_ERR;
}

/*
 * parse_digit: parse an expected digit token (e.g., '0x100')
 * args
 *     ptr: point to the start of string
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, move 'ptr' to the first char after token
 *                            and store the value of digit to 'value'
 *     PARSE_ERR: error, the value of 'ptr' and 'value' are undefined
 */
parse_t parse_digit(char **ptr, long *value)
{
    /* skip the blank and check */

    /* calculate the digit, (NOTE: see strtoll()) */

    /* set 'ptr' and 'value' */
    SKIP_BLANK(*ptr);
    
    if (IS_DIGIT(*ptr)){
	long digit = strtoul(*ptr, ptr, 0);

	while ((IS_DIGIT(*ptr)) || (IS_HEX_LETTER(*ptr))){
        	(*ptr)++;
	}

	*value = digit;

	return PARSE_DIGIT;
    }
    
    return PARSE_ERR;

}

/*
 * parse_imm: parse an expected immediate token (e.g., '$0x100' or 'STACK')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, the immediate token is a digit,
 *                            move 'ptr' to the first char after token,
 *                            and store the value of digit to 'value'
 *     PARSE_SYMBOL: success, the immediate token is a symbol,
 *                            move 'ptr' to the first char after token,
 *                            and allocate and store name to 'name' 
 *     PARSE_ERR: error, the value of 'ptr', 'name' and 'value' are undefined
 */
parse_t parse_imm(char **ptr, char **name, long *value)
{
    /* skip the blank and check */

    /* if IS_IMM, then parse the digit */

    /* if IS_LETTER, then parse the symbol */
    
    /* set 'ptr' and 'name' or 'value' */
	
    SKIP_BLANK(*ptr);

    if(IS_IMM(*ptr)){
	*ptr += 1;
	if(parse_digit(ptr,value) == PARSE_DIGIT)
	    return PARSE_DIGIT;
    }
    else 
	if(IS_LETTER(*ptr)){
	    if(parse_symbol(ptr,name) == PARSE_SYMBOL)
		return PARSE_SYMBOL;
	}	

    return PARSE_ERR;
}

/*
 * parse_mem: parse an expected memory token (e.g., '8(%rbp)')
 * args
 *     ptr: point to the start of string
 *     value: point to the value of digit
 *     regid: point to the regid of register
 *
 * return
 *     PARSE_MEM: success, move 'ptr' to the first char after token,
 *                          and store the value of digit to 'value',
 *                          and store the regid to 'regid'
 *     PARSE_ERR: error, the value of 'ptr', 'value' and 'regid' are undefined
 */
parse_t parse_mem(char **ptr, long *value, regid_t *regid)
{
    /* skip the blank and check */

    /* calculate the digit and register, (ex: (%rbp) or 8(%rbp)) */

    /* set 'ptr', 'value' and 'regid' */
    SKIP_BLANK(*ptr);

    if(IS_DIGIT(*ptr)){
			if(parse_digit(ptr,value) != PARSE_DIGIT)
				return PARSE_ERR;
			if(parse_reg(ptr, regid) == PARSE_REG)
				return PARSE_MEM;
    }
    else	if(IS_REG(*ptr) ||IS_LBRACKET(*ptr)){
				*value = 0;
				if(parse_reg(*ptr, regid) == PARSE_REG)
					return PARSE_MEM;
	 }
    return PARSE_ERR;
}

/*
 * parse_data: parse an expected data token (e.g., '0x100' or 'array')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, data token is a digit,
 *                            and move 'ptr' to the first char after token,
 *                            and store the value of digit to 'value'
 *     PARSE_SYMBOL: success, data token is a symbol,
 *                            and move 'ptr' to the first char after token,
 *                            and allocate and store name to 'name' 
 *     PARSE_ERR: error, the value of 'ptr', 'name' and 'value' are undefined
 */
parse_t parse_data(char **ptr, char **name, long *value)
{
    /* skip the blank and check */

    /* if IS_DIGIT, then parse the digit */

    /* if IS_LETTER, then parse the symbol */

    /* set 'ptr', 'name' and 'value' */
    SKIP_BLANK(*ptr);

    if (IS_DIGIT(*ptr))	{
	if (parse_digit(ptr, value) == PARSE_DIGIT){
	    return PARSE_DIGIT;
	}
    }
    else 
	if (IS_LETTER(*ptr)){
            if (parse_symbol(ptr, name) == PARSE_SYMBOL){
		return PARSE_SYMBOL;
	    }
	}
    return PARSE_ERR;
}

/*
 * parse_label: parse an expected label token (e.g., 'Loop:')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *
 * return
 *     PARSE_LABEL: success, move 'ptr' to the first char after token
 *                            and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr' is undefined
 */
parse_t parse_label(char **ptr, char **name)
{
    /* skip the blank and check */

    /* allocate name and copy to it */

    /* set 'ptr' and 'name' */
    SKIP_BLANK(*ptr);
    int llen = 0;
    char *ptmp = *ptr;

    while (!(IS_END(ptmp) || IS_BLANK(ptmp))){
	if (IS_COMMENT(ptmp))
            break;
        if (IS_COLON(ptmp)){
	    *name = (char *)malloc(llen + 1);
	    *(*name + llen) = '\0';
            strncpy(*name, *ptr, llen);
            *ptr += llen + 1;
	    return PARSE_LABEL;
        }

        llen++;
        ptmp++;
    }
    return PARSE_ERR;
}

/*
 * parse_line: parse a line of y64 code (e.g., 'Loop: mrmovq (%rcx), %rsi')
 * (you could combine above parse_xxx functions to do it)
 * args
 *     line: point to a line_t data with a line of y64 assembly code
 *
 * return
 *     PARSE_XXX: success, fill line_t with assembled y64 code
 *     PARSE_ERR: error, try to print err information (e.g., instr type and line number)
 */
type_t parse_line(line_t *line)
{

/* when finish parse an instruction or lable, we still need to continue check 
* e.g., 
*  Loop: mrmovl (%rbp), %rcx
*           call SUM  #invoke SUM function */

    /* skip blank and check IS_END */
    
    /* is a comment ? */

    /* is a label ? */

    /* is an instruction ? */

    /* set type and y64bin */

    /* update vmaddr */    

    /* parse the rest of instruction according to the itype */
    char *ptr = line->y64asm;
    SKIP_BLANK(ptr);
    if (IS_COMMENT(ptr)){
	line->type = TYPE_COMM;
	return line->type;
    }

    char *name;

    if (parse_label(&ptr, &name) == PARSE_LABEL){
		line->type = TYPE_INS;
		(line->y64bin).addr = vmaddr;
		(line->y64bin).bytes = 0;

		if (add_symbol(name) < 0){
			fprintf(stderr, "[L%d]: Dup symbol:%s\n", lineno, name);
			return TYPE_ERR;
		}
    }

    SKIP_BLANK(ptr);

    instr_t *inst;

    if (parse_instr(&ptr, &inst) == PARSE_INSTR){

		line->type = TYPE_INS;
		(line->y64bin).addr = vmaddr;
		(line->y64bin).bytes = inst->bytes;

		regid_t regA;
		regid_t regB;

		long value;
		char *name;
		char delim = ',';
		parse_t vtype;

		if (!strcmp(inst->name,"nop")){
	    		(line->y64bin).codes[0] = inst->code;
		}
		else	if (!strcmp(inst->name,"halt")){
					(line->y64bin).codes[0] = inst->code;
				}
		else if (!strcmp(inst->name,"rrmovq") || !strcmp(inst->name,"cmovle") 
						|| !strcmp(inst->name,"cmovl") || !strcmp(inst->name,"cmove") 
						|| !strcmp(inst->name,"cmovne") || !strcmp(inst->name,"cmovge") 
						|| !strcmp(inst->name,"cmovg"))
						{
							(line->y64bin).codes[0] = inst->code;
							if (parse_reg(&ptr, &regA) != PARSE_REG){	
								fprintf(stderr, "[L%d]: Invalid REG\n", lineno);
								return TYPE_ERR;
							}

							if (parse_delim(&ptr, delim) != PARSE_DELIM){
								fprintf(stderr, "[L%d]: Invalid ','\n", lineno);
								return TYPE_ERR;
							}

							if (parse_reg(&ptr, &regB) != PARSE_REG){
								fprintf(stderr, "[L%d]: Invalid REG\n", lineno);
								return TYPE_ERR;
							}						
							(line->y64bin).codes[1] = HPACK(regA, regB);
						}
	 else    if (!strcmp(inst->name,"irmovq")){
				(line->y64bin).codes[0] = inst->code;
				vtype = parse_imm(&ptr, &name, &value);
	
				if (vtype == PARSE_ERR){
					    fprintf(stderr, "[L%d]: Invalid Immediate\n", lineno);
					    return TYPE_ERR;
				}

				if (parse_delim(&ptr, delim) != PARSE_DELIM){
						fprintf(stderr, "[L%d]: Invalid ','\n", lineno);
					    return TYPE_ERR;
				}

				if (parse_reg(&ptr, &regB) != PARSE_REG){
				        fprintf(stderr, "[L%d]: Invalid REG\n", lineno);
					    return TYPE_ERR;
				}

				(line->y64bin).codes[1] = HPACK(REG_NONE, regB);

				if (vtype == PARSE_DIGIT){
					  for (int i = 0; i < 8; ++i)
		    				(line->y64bin).codes[2+i] = HPACK((value>>(i*8+4))&0xF, (value>>(i*8))&0xF); 
				}
				else     if (vtype == PARSE_SYMBOL)
						add_reloc(name, &(line->y64bin));
	 }

    else 	if (!strcmp(inst->name,"rmmovq")){
			(line->y64bin).codes[0] = inst->code;

			if (parse_reg(&ptr, &regA) != PARSE_REG){
				fprintf(stderr, "[L%d]: Invalid REG\n", lineno);
						return TYPE_ERR;
			}

			if (parse_delim(&ptr, delim) != PARSE_DELIM){
				fprintf(stderr, "[L%d]: Invalid ','\n", lineno);
						return TYPE_ERR;
			}

			if (parse_mem(&ptr, &value, &regB) != PARSE_MEM){
				fprintf(stderr, "[L%d]: Invalid MEM\n", lineno);
						return TYPE_ERR;
			}

			(line->y64bin).codes[1] = HPACK(regA, regB);

			for (int i = 0; i < 8; ++i)
            		(line->y64bin).codes[2+i] = HPACK((value>>(i*8+4))&0xF, (value>>(i*8))&0xF);

      }

     else    if (!strcmp(inst->name,"mrmovq")){
			(line->y64bin).codes[0] = inst->code;

            if (parse_mem(&ptr, &value, &regB) != PARSE_MEM){
					fprintf(stderr, "[L%d]: Invalid MEM\n", lineno);
							return TYPE_ERR;
	        }

	        if (parse_delim(&ptr, delim) != PARSE_DELIM){
					fprintf(stderr, "[L%d]: Invalid ','\n", lineno);
						return TYPE_ERR;
	        }

            if (parse_reg(&ptr, &regA) != PARSE_REG){
					fprintf(stderr, "[L%d]: Invalid REG\n", lineno);
							return TYPE_ERR;
	        }

            (line->y64bin).codes[1] = HPACK(regA, regB);

            for (int i = 0; i < 8; ++i)
                    (line->y64bin).codes[2+i] = HPACK((value>>(i*8+4))&0xF, (value>>(i*8))&0xF);

      }

     else  if (!strcmp(inst->name,"addq") || !strcmp(inst->name,"subq")
				|| !strcmp(inst->name,"andq") || !strcmp(inst->name,"xorq"))
	        {
						(line->y64bin).codes[0] = inst->code;
						if (parse_reg(&ptr, &regA) != PARSE_REG){
								fprintf(stderr, "[L%d]: Invalid REG\n", lineno);
								return TYPE_ERR;
						}
						if (parse_delim(&ptr, delim) != PARSE_DELIM){
							fprintf(stderr, "[L%d]: Invalid ','\n", lineno);
							return TYPE_ERR;
						}

						if (parse_reg(&ptr, &regB) != PARSE_REG){
							fprintf(stderr, "[L%d]: Invalid REG\n", lineno);
							return TYPE_ERR;
						}
						(line->y64bin).codes[1] = HPACK(regA, regB);
	        }
					   
	else     if (!strcmp(inst->name,"jmp") || !strcmp(inst->name,"jle")
				|| !strcmp(inst->name,"jl") || !strcmp(inst->name,"je")
				|| !strcmp(inst->name,"jne") || !strcmp(inst->name,"jge")
				|| !strcmp(inst->name,"jg") || !strcmp(inst->name,"call"))
            {
						(line->y64bin).codes[0] = inst->code;
						vtype = parse_imm(&ptr, &name, &value);
						if (vtype == PARSE_ERR){
							fprintf(stderr, "[L%d]: Invalid DEST\n", lineno);
							return TYPE_ERR;
						}
						if (vtype == PARSE_DIGIT){
							fprintf(stderr, "[L%d]: Invalid DEST\n", lineno);
							return TYPE_ERR;
						}
						else if (vtype == PARSE_SYMBOL)
							add_reloc(name, &(line->y64bin));
				}
	else	if (!strcmp(inst->name,"ret")){
				(line->y64bin).codes[0] = inst->code;
			}
	else   if (!strcmp(inst->name,"pushq"))       {
				(line->y64bin).codes[0] = inst->code;
				if (parse_reg(&ptr, &regA) != PARSE_REG){
						fprintf(stderr, "[L%d]: Invalid REG\n", lineno);
						return TYPE_ERR;
				}
	                (line->y64bin).codes[1] = HPACK(regA, REG_NONE);
	 }
	else 	if (!strcmp(inst->name,"popq")){
					(line->y64bin).codes[0] = inst->code;
		            if (parse_reg(&ptr, &regA) != PARSE_REG){
							fprintf(stderr, "[L%d]: Invalid REG\n", lineno);
							return TYPE_ERR;
					}
					(line->y64bin).codes[1] = HPACK(regA, REG_NONE);
			}

		  else if (!strcmp(inst->name,".byte") || !strcmp(inst->name,".word")
		  			|| !strcmp(inst->name,".long") || !strcmp(inst->name,".quad"))
			{
					vtype = parse_data(&ptr, &name, &value);
					if (vtype == PARSE_ERR)
					   return TYPE_ERR;       
					if (vtype == PARSE_DIGIT)
					{
						for (int i = 0; i < inst->bytes; i++)   
                			(line->y64bin).codes[i] = HPACK((value>>(i*8+4))&0xF, (value>>(i*8))&0xF);
					}

					else if (vtype == PARSE_SYMBOL)
					{
						add_reloc(name, &(line->y64bin));
					}
        }

        else if (!strcmp(inst->name,".pos"))
        {
			if (parse_digit(&ptr, &value) != PARSE_DIGIT)
				return TYPE_ERR;

			vmaddr = (int64_t)value;
			(line->y64bin).addr = vmaddr;
        }

        else if (!strcmp(inst->name,".align"))
        {
			if (parse_digit(&ptr, &value) != PARSE_DIGIT)
                return TYPE_ERR;

			int64_t alignno = (int64_t)value;
			vmaddr = ((vmaddr + alignno - 1) / alignno) * alignno;
            line->y64bin.addr = vmaddr;
        }

		vmaddr += (line->y64bin).bytes;
		SKIP_BLANK(ptr);
		if (! (IS_END(ptr)||IS_COMMENT(ptr)) )
		{
			line->type = TYPE_ERR;
		}
	}
    return line->type;
}

/*
 * assemble: assemble an y64 file (e.g., 'asum.ys')
 * args
 *     in: point to input file (an y64 assembly file)
 *
 * return
 *     0: success, assmble the y64 file to a list of line_t
 *     -1: error, try to print err information (e.g., instr type and line number)
 */
int assemble(FILE *in)
{
    static char asm_buf[MAX_INSLEN]; /* the current line of asm code */
    line_t *line;
    int slen;
    char *y64asm;

    /* read y64 code line-by-line, and parse them to generate raw y64 binary code list */
    while (fgets(asm_buf, MAX_INSLEN, in) != NULL) {
        slen  = strlen(asm_buf);
        while ((asm_buf[slen-1] == '\n') || (asm_buf[slen-1] == '\r')) { 
            asm_buf[--slen] = '\0'; /* replace terminator */
        }

        /* store y64 assembly code */
        y64asm = (char *)malloc(sizeof(char) * (slen + 1)); // free in finit
        strcpy(y64asm, asm_buf);

        line = (line_t *)malloc(sizeof(line_t)); // free in finit
        memset(line, '\0', sizeof(line_t));

        line->type = TYPE_COMM;
        line->y64asm = y64asm;
        line->next = NULL;

        line_tail->next = line;
        line_tail = line;
        lineno ++;

        if (parse_line(line) == TYPE_ERR) {
            return -1;
        }
    }

	lineno = -1;
    return 0;
}

/*
 * relocate: relocate the raw y64 binary code with symbol address
 *
 * return
 *     0: success
 *     -1: error, try to print err information (e.g., addr and symbol)
 */
int relocate(void)
{
    reloc_t *rtmp = NULL;
    
    rtmp = reltab->next;
    while (rtmp) {
        /* find symbol */
        /* relocate y64bin according itype */
    	symbol_t *symtmp;
		symtmp = find_symbol(rtmp->name);

		if (symtmp == NULL) {
			fprintf(stderr, "[--]: Unknown symbol:'%s'\n", rtmp->name);
			return -1;
		}

		int relbytes = (rtmp->y64bin)->bytes;
	
		switch (relbytes)
		{
			case 1:
				rel_val(rtmp, 0, 1, symtmp->addr);
				break;
			case 2:
				rel_val(rtmp, 0, 2, symtmp->addr);
				break;
			case 4:
				rel_val(rtmp, 0, 4, symtmp->addr);
				break;
			case 8:
				rel_val(rtmp, 0, 8, symtmp->addr);
				break;
			case 9:
				rel_val(rtmp, 1, 8, symtmp->addr);
				break;
			case 10:
				rel_val(rtmp, 2, 8, symtmp->addr);
				break;
			default:
				return -1;
		}
       /* next */
        rtmp = rtmp->next;
   }
   return 0;
}

/*
 * binfile: generate the y64 binary file
 * args
 *     out: point to output file (an y64 binary file)
 *
 * return
 *     0: success
 *     -1: error
 */
int binfile(FILE *out)
{
    /* prepare image with y64 binary code */
    /* binary write y64 code to output file (NOTE: see fwrite()) */
	line_t *ltmp = line_head->next;
	while (ltmp != NULL)
	{
		if (ltmp->type == TYPE_INS)
		{
			if (fseek(out, (ltmp->y64bin).addr, SEEK_SET) != 0)
				return -1;
			fwrite((ltmp->y64bin).codes, 1, (ltmp->y64bin).bytes, out);
		}
		ltmp = ltmp->next;
	}
    return 0;
}


/* whether print the readable output to screen or not ? */
bool_t screen = FALSE; 

static void hexstuff(char *dest, int value, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        char c;
        int h = (value >> 4*i) & 0xF;
        c = h < 10 ? h + '0' : h - 10 + 'a';
        dest[len-i-1] = c;
    }
}

void print_line(line_t *line)
{
    char buf[32];

    /* line format: 0xHHH: cccccccccccc | <line> */
    if (line->type == TYPE_INS) {
        bin_t *y64bin = &line->y64bin;
        int i;
        
        strcpy(buf, "  0x000:                      | ");
        
        hexstuff(buf+4, y64bin->addr, 3);
        if (y64bin->bytes > 0)
            for (i = 0; i < y64bin->bytes; i++)
                hexstuff(buf+9+2*i, y64bin->codes[i]&0xFF, 2);
    } else {
        strcpy(buf, "                              | ");
    }

    printf("%s%s\n", buf, line->y64asm);
}

/* 
 * print_screen: dump readable binary and assembly code to screen
 * (e.g., Figure 4.8 in ICS book)
 */
void print_screen(void)
{
    line_t *tmp = line_head->next;
    while (tmp != NULL) {
        print_line(tmp);
        tmp = tmp->next;
    }
}

/* init and finit */
void init(void)
{
    reltab = (reloc_t *)malloc(sizeof(reloc_t)); // free in finit
    memset(reltab, 0, sizeof(reloc_t));

    symtab = (symbol_t *)malloc(sizeof(symbol_t)); // free in finit
    memset(symtab, 0, sizeof(symbol_t));

    line_head = (line_t *)malloc(sizeof(line_t)); // free in finit
    memset(line_head, 0, sizeof(line_t));
    line_tail = line_head;
    lineno = 0;
}

void finit(void)
{
    reloc_t *rtmp = NULL;
    do {
        rtmp = reltab->next;
        if (reltab->name) 
            free(reltab->name);
        free(reltab);
        reltab = rtmp;
    } while (reltab);
    
    symbol_t *stmp = NULL;
    do {
        stmp = symtab->next;
        if (symtab->name) 
            free(symtab->name);
        free(symtab);
        symtab = stmp;
    } while (symtab);

    line_t *ltmp = NULL;
    do {
        ltmp = line_head->next;
        if (line_head->y64asm) 
            free(line_head->y64asm);
        free(line_head);
        line_head = ltmp;
    } while (line_head);
}

static void usage(char *pname)
{
    printf("Usage: %s [-v] file.ys\n", pname);
    printf("   -v print the readable output to screen\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    int rootlen;
    char infname[512];
    char outfname[512];
    int nextarg = 1;
    FILE *in = NULL, *out = NULL;
    
    if (argc < 2)
        usage(argv[0]);
    
    if (argv[nextarg][0] == '-') {
        char flag = argv[nextarg][1];
        switch (flag) {
          case 'v':
            screen = TRUE;
            nextarg++;
            break;
          default:
            usage(argv[0]);
        }
    }

    /* parse input file name */
    rootlen = strlen(argv[nextarg])-3;
    /* only support the .ys file */
    if (strcmp(argv[nextarg]+rootlen, ".ys"))
        usage(argv[0]);
    
    if (rootlen > 500) {
        err_print("File name too long");
        exit(1);
    }
 

    /* init */
    init();

    
    /* assemble .ys file */
    strncpy(infname, argv[nextarg], rootlen);
    strcpy(infname+rootlen, ".ys");
    in = fopen(infname, "r");
    if (!in) {
        err_print("Can't open input file '%s'", infname);
        exit(1);
    }
    
    if (assemble(in) < 0) {
        err_print("Assemble y64 code error");
        fclose(in);
        exit(1);
    }
    fclose(in);


    /* relocate binary code */
    if (relocate() < 0) {
        err_print("Relocate binary code error");
        exit(1);
    }


    /* generate .bin file */
    strncpy(outfname, argv[nextarg], rootlen);
    strcpy(outfname+rootlen, ".bin");
    out = fopen(outfname, "wb");
    if (!out) {
        err_print("Can't open output file '%s'", outfname);
        exit(1);
    }

    if (binfile(out) < 0) {
        err_print("Generate binary file error");
        fclose(out);
        exit(1);
    }
    fclose(out);
    
    /* print to screen (.yo file) */
    if (screen)
       print_screen(); 

    /* finit */
    finit();
    return 0;
}



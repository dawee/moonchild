#ifndef MOONCHILD_H
#define MOONCHILD_H

#include <stdint.h>

#define MOON_MAX_REGISTERS 10

#define OPBK_FLAG 0b00000001
#define OPCK_FLAG 0b00000010


#ifndef MOONCHILD_SIMULATOR
  #include <avr/pgmspace.h>
  typedef uint16_t PGMEM_ADDRESS;
  #define progmem_read(X, Y) pgm_read_byte_near(X + Y)
#else
  #define PROGMEM
  typedef void * PGMEM_ADDRESS;
  char progmem_read(PGMEM_ADDRESS mem_addr, uint16_t offset);
#endif


enum MOON_OPCODES {
  OPCODE_MOVE,
  OPCODE_LOADK,
  OPCODE_LOADKX,
  OPCODE_LOADBOOL,
  OPCODE_LOADNIL,
  OPCODE_GETUPVAL,
  OPCODE_GETTABUP,
  OPCODE_GETTABLE,
  OPCODE_SETTABUP,
  OPCODE_SETUPVAL,
  OPCODE_SETTABLE,
  OPCODE_NEWTABLE,
  OPCODE_SELF,
  OPCODE_ADD,
  OPCODE_SUB,
  OPCODE_MUL,
  OPCODE_MOD,
  OPCODE_POW,
  OPCODE_DIV,
  OPCODE_IDIV,
  OPCODE_BAND,
  OPCODE_BOR,
  OPCODE_BXOR,
  OPCODE_SHL,
  OPCODE_SHR,
  OPCODE_UNM,
  OPCODE_BNOT,
  OPCODE_NOT,
  OPCODE_LEN,
  OPCODE_CONCAT,
  OPCODE_JMP,
  OPCODE_EQ,
  OPCODE_LT,
  OPCODE_LE,
  OPCODE_TEST,
  OPCODE_TESTSET,
  OPCODE_CALL,
  OPCODE_TAILCALL,
  OPCODE_RETURN,
  OPCODE_FORLOOP,
  OPCODE_FORPREP,
  OPCODE_TFORCALL,
  OPCODE_TFORLOOP,
  OPCODE_SETLIST,
  OPCODE_CLOSURE,
  OPCODE_VARARG,
  OPCODE_EXTRAARG
};

enum MOON_TYPES {
  LUA_NIL,
  LUA_INT,
  LUA_NUMBER,
  LUA_STRING,
};

typedef struct {
  uint8_t type;
  PGMEM_ADDRESS data_addr;
} moon_value;

typedef struct {
  uint8_t opcode;
  uint8_t a;
  uint32_t b;
  uint8_t c;
  uint8_t flag;
} moon_instruction;

typedef struct {
  uint16_t func_name_size;
  PGMEM_ADDRESS func_name_addr;
  uint16_t instructions_count;
  PGMEM_ADDRESS instructions_addr;
  uint16_t constants_count;
  PGMEM_ADDRESS constants_addr;
} moon_prototype;

typedef struct {
  moon_prototype * prototype;
  int8_t registers[MOON_MAX_REGISTERS];
} moon_closure;


void moon_run_generated();
void moon_arch_run(PGMEM_ADDRESS prototype_addr);
void moon_run(PGMEM_ADDRESS prototype_addr, char * result);

#endif
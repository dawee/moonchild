#ifndef MOONCHILD_H
#define MOONCHILD_H

#include <stdint.h>
#include <stdio.h>

#define MOON_MAX_REGISTERS 10

#define OPBK_FLAG 0b00000001
#define OPCK_FLAG 0b00000010

typedef void * SRAM_ADDRESS;

#ifndef BOOL
  typedef uint8_t BOOL;

  enum BOOL_VALUES {FALSE, TRUE};
#endif


#ifndef MOONCHILD_SIMULATOR
  #include <avr/pgmspace.h>
  typedef uint16_t PGMEM_ADDRESS;
  #define progmem_read(X, Y) pgm_read_byte_near(X + Y)
#else
  #define PROGMEM
  #define moon_debug(...) printf(__VA_ARGS__)
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
  LUA_TRUE,
  LUA_FALSE,
  LUA_INT,
  LUA_NUMBER,
  LUA_STRING,
};

typedef int16_t CTYPE_LUA_INT;
typedef float CTYPE_LUA_NUMBER;

typedef struct {
  uint8_t type;
  uint16_t nodes;
} moon_value;

typedef struct {
  uint8_t type;
  uint16_t nodes;
  CTYPE_LUA_INT val;
} moon_int_value;

typedef struct {
  uint8_t type;
  uint16_t nodes;
  CTYPE_LUA_NUMBER val;
} moon_number_value;

typedef struct {
  uint8_t type;
  uint16_t nodes;
  SRAM_ADDRESS string_addr;
  uint16_t length;
} moon_string_value;

typedef struct {
  uint8_t opcode;
  uint8_t a;
  uint32_t b;
  uint8_t c;
  uint8_t flag;
} moon_instruction;

typedef struct {
  BOOL is_progmem;
  BOOL is_copy;
  SRAM_ADDRESS value_addr;
} moon_reference;

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
  moon_reference * registers[MOON_MAX_REGISTERS];
} moon_closure;


const moon_value MOON_NIL_VALUE PROGMEM = {.type = LUA_NIL, .nodes = 1};
const moon_value MOON_TRUE_VALUE PROGMEM = {.type = LUA_TRUE, .nodes = 1};
const moon_value MOON_FALSE_VALUE PROGMEM = {.type = LUA_FALSE, .nodes = 1};


void moon_run_generated();
void moon_arch_run(PGMEM_ADDRESS prototype_addr);
void moon_run(PGMEM_ADDRESS prototype_addr, char * result);

#endif
#ifndef MOONCHILD_H
#define MOONCHILD_H

#include <stdint.h>
#include <stdio.h>
#include <avr/pgmspace.h>


#define MOON_MAX_REGISTERS 20
#define MOON_MAX_UPVALUES 20

#define OPBK_FLAG 0b00000001
#define OPCK_FLAG 0b00000010

typedef void * SRAM_ADDRESS;


#ifndef BOOL
  typedef uint8_t BOOL;

  enum BOOL_VALUES {FALSE, TRUE};
#endif



#ifndef MOONCHILD_DEBUG
  #define moon_debug(...)
#else
  #define moon_debug(...) printf(__VA_ARGS__)
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


/*
 * Intructions Macros
 */

#define MOON_READ_OPCODE(ins) (uint8_t)(ins->raw & 0x3F)
#define MOON_READ_A(ins) (uint8_t)((ins->raw & 0x3FC0) >> 6)
#define MOON_READ_B(ins) (uint16_t)(((ins->raw & 0xFF800000) >> 23) & 0xFF)
#define MOON_READ_C(ins) (uint16_t)(((ins->raw & 0x7FC000) >> 14) & 0xFF)
#define MOON_READ_BX(ins) (uint32_t)((ins->raw & 0xFFFFC000) >> 14)
#define MOON_READ_SBX(ins) (int32_t)(((ins->raw & 0xFFFFC000) >> 14) - 0x1FFFF)
#define MOON_IS_OPBK(ins) ((((ins->raw & 0xFF800000) >> 23) & 0x00000100) == 0x100)
#define MOON_IS_OPCK(ins) ((((ins->raw & 0x007FC000) >> 14) & 0x00000100) == 0x100)

/*
 * Types
 */

enum MOON_TYPES {
  MOON_TYPE_NIL,
  MOON_TYPE_TRUE,
  MOON_TYPE_FALSE,
  MOON_TYPE_INT,
  MOON_TYPE_NUMBER,
  MOON_TYPE_STRING,
  MOON_TYPE_CLOSURE,
  MOON_TYPE_API,
};

typedef int16_t MOON_CTYPE_INT;
typedef float MOON_CTYPE_NUMBER;

typedef struct {
  uint8_t type;
  uint16_t nodes;
} moon_value;

typedef struct {
  uint8_t type;
  uint16_t nodes;
  MOON_CTYPE_INT val;
} moon_int_value;

typedef struct {
  uint8_t type;
  uint16_t nodes;
  MOON_CTYPE_NUMBER val;
} moon_number_value;

typedef struct {
  uint8_t type;
  uint16_t nodes;
  SRAM_ADDRESS string_addr;
  uint16_t length;
} moon_string_value;

typedef struct {
  uint32_t raw;
} moon_instruction;

typedef struct {
  BOOL is_progmem;
  BOOL is_copy;
  SRAM_ADDRESS value_addr;
} moon_reference;

typedef struct moon_hash_pair_t {
  moon_reference key_reference;
  moon_reference value_reference;
  struct moon_hash_pair_t * next;
} moon_hash_pair;

typedef struct {
  uint16_t count;
  moon_hash_pair * first;
  moon_hash_pair * last;
} moon_hash;

typedef struct {
  uint8_t in_stack;
  uint8_t idx;
} moon_upvalue;

typedef struct {
  uint8_t num_params;
  uint8_t is_varargs;
  uint8_t max_stack_size;
  uint16_t instructions_count;
  PGM_VOID_P instructions_addr;
  uint16_t constants_count;
  PGM_VOID_P constants_addr;
  uint16_t prototypes_count;
  PGM_VOID_P prototypes_addr;
  uint16_t upvalues_count;
  PGM_VOID_P upvalues_addr;
} moon_prototype;

typedef struct moon_closure_t {
  uint8_t type;
  uint16_t nodes;
  uint16_t top;
  uint16_t base;
  uint16_t pc;
  moon_hash in_upvalues;
  PGM_VOID_P prototype_addr;
  uint16_t prototype_addr_cursor;
  moon_reference * registers[MOON_MAX_REGISTERS];
  moon_reference * upvalues[MOON_MAX_UPVALUES];
  moon_reference result;
} moon_closure;

typedef struct {
  uint8_t type;
  uint16_t nodes;
  void (*func)(moon_closure *, BOOL);
} moon_api_value;


const moon_value MOON_NIL_VALUE PROGMEM;
const moon_value MOON_TRUE_VALUE PROGMEM;
const moon_value MOON_FALSE_VALUE PROGMEM;

/*
 * Redefinable Macros (monitor, simulator & test stuff)
 */

#ifndef MOON_MALLOC
  #define MOON_MALLOC(name, size) malloc(size)
#endif

#ifndef MOON_FREE
  #define MOON_FREE(name, pointer) free(pointer)
#endif

/*
 * Types Macros
 */

#define MOON_IS_NIL(ref) (((moon_value *) (ref)->value_addr)->type == MOON_TYPE_NIL)
#define MOON_IS_TRUE(ref) (((moon_value *) (ref)->value_addr)->type == MOON_TYPE_TRUE)
#define MOON_IS_FALSE(ref) (((moon_value *) (ref)->value_addr)->type == MOON_TYPE_FALSE)
#define MOON_IS_INT(ref) (((moon_value *) (ref)->value_addr)->type == MOON_TYPE_INT)
#define MOON_IS_NUMBER(ref) (((moon_value *) (ref)->value_addr)->type == MOON_TYPE_NUMBER)
#define MOON_IS_STRING(ref) (((moon_value *) (ref)->value_addr)->type == MOON_TYPE_STRING)
#define MOON_IS_CLOSURE(ref) (((moon_value *) (ref)->value_addr)->type == MOON_TYPE_CLOSURE)

#define MOON_IS_ARITHMETIC(ref) (MOON_IS_INT(ref) || MOON_IS_NUMBER(ref))

#define MOON_AS_VALUE(ref) ((moon_value *)((ref)->value_addr))
#define MOON_AS_NUMBER(ref) ((moon_number_value *)((ref)->value_addr))
#define MOON_AS_INT(ref) ((moon_int_value *)((ref)->value_addr))
#define MOON_AS_STRING(ref) ((moon_string_value *)((ref)->value_addr))
#define MOON_AS_CSTRING(ref) ((char *)(MOON_AS_STRING(ref)->string_addr))
#define MOON_AS_CLOSURE(ref) ((moon_closure *)((ref)->value_addr))
#define MOON_AS_API(ref) ((moon_api_value *)((ref)->value_addr))
#define MOON_AS_GETSET(ref) ((moon_getset_value *)((ref)->value_addr))


#define MOON_COPY_VALUE(dest, src, size) \
  (dest)->value_addr = (SRAM_ADDRESS) MOON_MALLOC("copy_value", size); \
  progmem_cpy((dest)->value_addr, (src)->value_addr, size, 0); \
  (dest)->is_copy = TRUE; \
  (dest)->is_progmem = FALSE;


void moon_init();
void moon_run_closure(moon_closure * closure, moon_closure * parent);
moon_closure * moon_create_closure(PGM_VOID_P prototype_addr, uint16_t prototype_addr_cursor, moon_closure * parent);
void moon_create_string_value(moon_reference * reference, const char * str);
BOOL moon_find_closure_value(moon_reference * result, moon_closure * closure, moon_reference * key_reference);
void moon_add_global_api_func(const char * key_str, void (*api_func)(moon_closure *, BOOL));
void moon_ref_to_cstr(char * result, moon_reference * reference);
void moon_create_value_copy(moon_reference * dest, moon_reference * src);
void moon_delete_value(moon_value * value);
void moon_set_to_nil(moon_reference * reference);
void moon_set_to_true(moon_reference * reference);
void moon_set_to_false(moon_reference * reference);
void moon_add_global(const char * key_str, moon_reference * value_reference);
void moon_create_int_value(moon_reference * reference, MOON_CTYPE_INT int_val);

#endif

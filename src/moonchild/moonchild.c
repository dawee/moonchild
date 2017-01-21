#include "moonchild.h"
#include "monitor.h"

#include <stdio.h>
#include <stdlib.h>

static moon_hash globals_hash;

const moon_value MOON_NIL_VALUE PROGMEM = {.type = LUA_NIL, .nodes = 1};
const moon_value MOON_TRUE_VALUE PROGMEM = {.type = LUA_TRUE, .nodes = 1};
const moon_value MOON_FALSE_VALUE PROGMEM = {.type = LUA_FALSE, .nodes = 1};

static void progmem_cpy(void * dest, PGM_VOID_P src, uint16_t size, uint16_t offset) {
  char * cdest = (char *)dest;

  for (uint16_t index = 0; index < size; ++index) {
    cdest[index] = pgm_read_byte_near((PGM_VOID_P)(((intptr_t) src) + offset + index));
  }
}

void moon_set_to_nil(moon_reference * reference) {
  reference->is_progmem = TRUE;
  reference->value_addr = (SRAM_ADDRESS) &MOON_NIL_VALUE;
}

void moon_set_to_true(moon_reference * reference) {
  reference->is_progmem = TRUE;
  reference->value_addr = (SRAM_ADDRESS) &MOON_TRUE_VALUE;
}

void moon_set_to_false(moon_reference * reference) {
  reference->is_progmem = TRUE;
  reference->value_addr = (SRAM_ADDRESS) &MOON_FALSE_VALUE;
}

void moon_create_int_value(moon_reference * reference, CTYPE_LUA_INT int_val) {
  reference->value_addr = (SRAM_ADDRESS) moon_malloc("moon_create_int_value", sizeof(moon_int_value));
  ((moon_int_value *) reference->value_addr)->type = LUA_INT;
  ((moon_int_value *) reference->value_addr)->val = int_val;
  ((moon_int_value *) reference->value_addr)->nodes = 1;
}

static void create_number_value(moon_reference * reference, CTYPE_LUA_NUMBER number_val) {
  reference->value_addr = (SRAM_ADDRESS) moon_malloc("create_number_value", sizeof(moon_number_value));
  ((moon_number_value *) reference->value_addr)->type = LUA_NUMBER;
  ((moon_number_value *) reference->value_addr)->val = number_val;
  ((moon_number_value *) reference->value_addr)->nodes = 1;
}

void moon_create_string_value(moon_reference * reference, const char * str) {
  uint16_t length = 0;

  while(str[length] != '\0') ++length;

  reference->value_addr = (SRAM_ADDRESS) moon_malloc("moon_create_string_value", sizeof(moon_string_value));

  MOON_AS_STRING(reference)->type = LUA_STRING;
  MOON_AS_STRING(reference)->nodes = 1;
  MOON_AS_STRING(reference)->string_addr = (SRAM_ADDRESS) moon_malloc("moon_create_string_value (cstr)", length + 1);
  MOON_AS_STRING(reference)->length = length;

  for (uint16_t index = 0; index < length; ++index) {
    MOON_AS_CSTRING(reference)[index] = str[index];
  }

  MOON_AS_CSTRING(reference)[length] = '\0';
}

static void create_api_value(moon_reference * reference, void (*api_func)(moon_closure *, BOOL)) {
  reference->value_addr = (SRAM_ADDRESS) moon_malloc("create_api_value", sizeof(moon_api_value));
  MOON_AS_API(reference)->type = LUA_API;
  MOON_AS_API(reference)->nodes = 1;
  MOON_AS_API(reference)->func = api_func;
}

static void init_registers(moon_closure * closure);
static void init_upvalues(moon_closure * closure);
static void create_registers(moon_closure * closure);
static void create_upvalues(moon_closure * closure, moon_closure * parent);

static void init_hash(moon_hash * hash);

moon_closure * moon_create_closure(PGM_VOID_P prototype_addr, uint16_t prototype_addr_cursor, moon_closure * parent) {
  moon_closure * closure = (moon_closure *) moon_malloc("moon_create_closure", sizeof(moon_closure));

  closure->type = LUA_CLOSURE;
  closure->nodes = 0;
  closure->prototype_addr = prototype_addr;
  closure->prototype_addr_cursor = prototype_addr_cursor;

  init_hash(&(closure->in_upvalues));
  init_registers(closure);
  init_upvalues(closure);
  create_registers(closure);
  if (parent != NULL) create_upvalues(closure, parent);
  moon_set_to_nil(&(closure->result));

  return closure;
}

static moon_prototype * create_prototype() {
  return (moon_prototype *) moon_malloc("create_prototype", sizeof(moon_prototype));
}

void moon_delete_value(moon_value * value) {
  if (value == NULL) return;

  if (value->type == LUA_STRING) {
    moon_free("moon_delete_value (cstr)", ((moon_string_value *) value)->string_addr);
  }

  moon_free("moon_delete_value", value);
  value = NULL;
}

static void create_progmem_value_copy(moon_reference * dest, moon_reference * src) {
  uint8_t type = pgm_read_byte_near(src->value_addr);
  moon_string_value string_value;

  switch(type) {
    case LUA_NIL:
    case LUA_TRUE:
    case LUA_FALSE:
      dest->value_addr = (SRAM_ADDRESS) moon_malloc("create_progmem_value_copy (NIL/TRUE/FALSE)", sizeof(moon_value));
      progmem_cpy(dest->value_addr, src->value_addr, sizeof(moon_value), 0);
      dest->is_copy = TRUE;
      dest->is_progmem = FALSE;
      break;

    case LUA_INT:
      dest->value_addr = (SRAM_ADDRESS) moon_malloc("create_progmem_value_copy, (INT)", sizeof(moon_int_value));
      progmem_cpy(dest->value_addr, src->value_addr, sizeof(moon_int_value), 0);
      dest->is_copy = TRUE;
      dest->is_progmem = FALSE;
      break;

    case LUA_NUMBER:
      dest->value_addr = (SRAM_ADDRESS) moon_malloc("create_progmem_value_copy NUMBER", sizeof(moon_number_value));
      progmem_cpy(dest->value_addr, src->value_addr, sizeof(moon_number_value), 0);
      dest->is_copy = TRUE;
      dest->is_progmem = FALSE;
      break;

    case LUA_STRING:
      dest->value_addr = (SRAM_ADDRESS) moon_malloc("create_progmem_value_copy STRING", sizeof(moon_string_value));
      dest->is_progmem = FALSE;

      progmem_cpy(&string_value, src->value_addr, sizeof(moon_string_value), 0);
      progmem_cpy(dest->value_addr, src->value_addr, sizeof(moon_string_value), 0);

      ((moon_string_value *) dest->value_addr)->string_addr = (SRAM_ADDRESS) moon_malloc("create_progmem_value_copy CSTRING", ((moon_string_value *) dest->value_addr)->length + 1);

      progmem_cpy(((moon_string_value *) dest->value_addr)->string_addr, string_value.string_addr, ((moon_string_value *) dest->value_addr)->length, 0);

      ((char *)(((moon_string_value *) dest->value_addr)->string_addr))[((moon_string_value *) dest->value_addr)->length] = '\0';

      dest->is_copy = TRUE;
      break;

    case LUA_API:
      dest->value_addr = (SRAM_ADDRESS) moon_malloc("create_progmem_value_copy API", sizeof(moon_api_value));
      progmem_cpy(dest->value_addr, src->value_addr, sizeof(moon_api_value), 0);
      dest->is_copy = TRUE;
      dest->is_progmem = FALSE;
      break;

    default:
      moon_debug("error: could not copy unknown type : %d\n", type);
      break;
  };
}

static void copy_reference(moon_reference * dest, moon_reference * src) {
  dest->is_progmem = src->is_progmem;
  dest->value_addr = src->value_addr;
}

void moon_create_value_copy(moon_reference * dest, moon_reference * src) {
  if (src->is_progmem == TRUE) {
    create_progmem_value_copy(dest, src);
  } else {
    copy_reference(dest, src);
  }
}

void moon_ref_to_cstr(char * result, moon_reference * reference) {
  moon_reference buf_ref;

  moon_create_value_copy(&buf_ref, reference);

  switch(((moon_value *) buf_ref.value_addr)->type) {
    case LUA_NIL:
      sprintf(result, "nil");
      break;
    case LUA_FALSE:
      sprintf(result, "false");
      break;
    case LUA_TRUE:
      sprintf(result, "true");
      break;
    case LUA_INT:
      sprintf(result, "%d", ((moon_int_value *) buf_ref.value_addr)->val);
      break;
    case LUA_NUMBER:
      sprintf(result, "~%d", (int)((moon_number_value *) buf_ref.value_addr)->val);
      break;
    case LUA_STRING:
      sprintf(result, "%s", (char *)(((moon_string_value *) buf_ref.value_addr)->string_addr));
      break;
    case LUA_CLOSURE:
      sprintf(result, "<closure>");
      break;
    case LUA_API:
      sprintf(result, "<native>");
      break;

    default:
      sprintf(result, "other type : %d", ((moon_value *) buf_ref.value_addr)->type);
      break;
  };

  if (buf_ref.is_copy == TRUE) moon_delete_value((moon_value *) buf_ref.value_addr);
}


static void read_constant_reference(moon_reference * reference, moon_prototype * prototype, uint16_t index) {
  progmem_cpy(reference, prototype->constants_addr, sizeof(moon_reference), sizeof(moon_reference) * index);
}

static void read_closure_prototype(moon_prototype * prototype, moon_closure * closure) {
  progmem_cpy(prototype, closure->prototype_addr, sizeof(moon_prototype), sizeof(moon_prototype) * closure->prototype_addr_cursor);
}

static void read_prototype_upvalue(moon_upvalue * upvalue, moon_prototype * prototype, uint16_t index) {
  progmem_cpy(upvalue, prototype->upvalues_addr, sizeof(moon_upvalue), sizeof(moon_upvalue) * index);
}

static BOOL equals_string(moon_reference * ref_a, moon_reference * ref_b) {
  BOOL result = TRUE;

  if (MOON_AS_STRING(ref_a)->length != MOON_AS_STRING(ref_b)->length) return FALSE;

  for (uint16_t index = 0; index < MOON_AS_STRING(ref_a)->length; ++index) {
    if (MOON_AS_CSTRING(ref_a)[index] != MOON_AS_CSTRING(ref_b)[index]) {
      result = FALSE;
      break;
    }
  }

  return result;
}

static BOOL equals(moon_reference * ref_a, moon_reference * ref_b) {
  BOOL result = FALSE;

  switch(MOON_AS_VALUE(ref_a)->type) {
    case LUA_NIL:
    case LUA_FALSE:
    case LUA_TRUE:
      result = (MOON_AS_VALUE(ref_a)->type == MOON_AS_VALUE(ref_b)->type) ? TRUE : FALSE;
      break;

    case LUA_INT:
      result = (
        (MOON_IS_INT(ref_b) && (MOON_AS_INT(ref_a)->val == MOON_AS_INT(ref_b)->val))
        || (MOON_IS_NUMBER(ref_b) && (MOON_AS_INT(ref_a)->val == MOON_AS_NUMBER(ref_b)->val))
      ) ? TRUE : FALSE;
      break;

    case LUA_NUMBER:
      result = (
        (MOON_IS_NUMBER(ref_b) && (MOON_AS_NUMBER(ref_a)->val == MOON_AS_NUMBER(ref_b)->val))
        || (MOON_IS_INT(ref_b) && (MOON_AS_NUMBER(ref_a)->val == MOON_AS_INT(ref_b)->val))
      ) ? TRUE : FALSE;
      break;

    case LUA_STRING:
      result = (MOON_IS_STRING(ref_b) && equals_string(ref_a, ref_b)) ? TRUE : FALSE;
      break;

    default:
      moon_debug("error: could not compute equals() of type '%d'", MOON_AS_VALUE(ref_a)->type);
      break;
  };

  return result;
}

static BOOL is_lower(moon_reference * ref_a, moon_reference * ref_b) {
  BOOL result = FALSE;

  if (!MOON_IS_ARITHMETIC(ref_a) || !MOON_IS_ARITHMETIC(ref_b)) return result;

  switch(MOON_AS_VALUE(ref_a)->type) {
    case LUA_INT:
      result = (
        (MOON_IS_INT(ref_b) && (MOON_AS_INT(ref_a)->val < MOON_AS_INT(ref_b)->val))
        || (MOON_IS_NUMBER(ref_b) && (MOON_AS_INT(ref_a)->val < MOON_AS_NUMBER(ref_b)->val))
      ) ? TRUE : FALSE;
      break;

    case LUA_NUMBER:
      result = (
        (MOON_IS_NUMBER(ref_b) && (MOON_AS_NUMBER(ref_a)->val < MOON_AS_NUMBER(ref_b)->val))
        || (MOON_IS_INT(ref_b) && (MOON_AS_NUMBER(ref_a)->val < MOON_AS_INT(ref_b)->val))
      ) ? TRUE : FALSE;
      break;

    default:
      moon_debug("error: could not compute equals() of type '%d'", MOON_AS_VALUE(ref_a)->type);
      break;
  };

  return result;
}

static BOOL pass_test(moon_reference * ref) {
  return (
    (MOON_IS_NIL(ref) || MOON_IS_FALSE(ref))
    || (MOON_IS_INT(ref) && (MOON_AS_INT(ref)->val == 0))
    || (MOON_IS_NUMBER(ref) && (MOON_AS_NUMBER(ref)->val == 0))
  ) ? FALSE : TRUE;
}

static void init_hash(moon_hash * hash) {
  hash->count = 0;
}

static BOOL find_hash_value(moon_reference * result, moon_hash * hash, moon_reference * key_reference) {
  uint16_t index = 0;
  BOOL found = FALSE;
  moon_reference buf_key_ref;
  moon_hash_pair * pair = hash->first;

  moon_set_to_nil(result);
  moon_create_value_copy(&buf_key_ref, key_reference);

  for (index = 0; index < hash->count; ++index) {
    found = equals(&(pair->key_reference), &buf_key_ref);

    if (found == TRUE) {
      copy_reference(result, &(pair->value_reference));
      break;
    }

    pair = pair->next;
  }

  if (buf_key_ref.is_copy == TRUE) moon_delete_value((moon_value *) buf_key_ref.value_addr);

  return found;
}

static void set_hash_pair(moon_hash * hash, moon_reference * key_reference, moon_reference * value_reference) {
  uint16_t index = 0;
  moon_hash_pair * new_pair = (moon_hash_pair *) moon_malloc("set_hash_pair", sizeof(moon_hash_pair));
  moon_hash_pair * pair = NULL;
  moon_hash_pair * previous = NULL;

  moon_create_value_copy(&(new_pair->key_reference), key_reference);
  copy_reference(&(new_pair->value_reference), value_reference);

  if (hash->count == 0) {
    hash->first = new_pair;
    hash->last = new_pair;
    hash->count = 1;
  } else {
    previous = NULL;
    pair = hash->first;

    for (index = 0; index < hash->count; ++index) {
      if (equals(&(pair->key_reference), &(new_pair->key_reference)) == TRUE) break;

      previous = pair;
      pair = pair->next;
    }

    if (index == hash->count) {
      hash->last->next = new_pair;
      hash->last = new_pair;
      new_pair->next = NULL;
      hash->count++;
    } else {
      moon_delete_value(MOON_AS_VALUE(&(pair->key_reference)));
      new_pair->next = pair->next;

      if (previous != NULL) previous->next = new_pair;
      if (hash->first == pair) hash->first = new_pair;
      if (hash->last == pair) hash->last = new_pair;
    }
  }
}

BOOL moon_find_closure_value(moon_reference * result, moon_closure * closure, moon_reference * key_reference) {
  if (find_hash_value(result, &(closure->in_upvalues), key_reference) == TRUE) return TRUE;

  return find_hash_value(result, &globals_hash, key_reference);
}

static void copy_constant_reference(moon_reference * dest, moon_prototype * prototype, uint16_t index) {
  moon_reference constant_reference;

  read_constant_reference(&constant_reference, prototype, index);
  copy_reference(dest, &constant_reference);
}

static void create_register(moon_closure * closure, uint16_t index) {
  if (closure->registers[index] != NULL) return;

  closure->registers[index] = (moon_reference *) moon_malloc("create_register", sizeof(moon_reference));
  moon_set_to_nil(closure->registers[index]);
}

static void init_registers(moon_closure * closure) {
  for (uint16_t index = 0; index < MOON_MAX_REGISTERS; ++index) {
    closure->registers[index] = NULL;
  }
}

static void init_upvalues(moon_closure * closure) {
  for (uint16_t index = 0; index < MOON_MAX_UPVALUES; ++index) {
    closure->upvalues[index] = NULL;
  }
}

static void create_registers(moon_closure * closure) {
  moon_prototype prototype;

  read_closure_prototype(&prototype, closure);

  for (uint16_t index = 0; index < prototype.max_stack_size; ++index) {
    create_register(closure, index);
  }
}

static void create_upvalues(moon_closure * closure, moon_closure * parent) {
  moon_prototype prototype;
  moon_upvalue upvalue;

  read_closure_prototype(&prototype, closure);

  for (uint16_t index = 0; index < prototype.upvalues_count; ++index) {
    read_prototype_upvalue(&upvalue, &prototype, index);

    if (upvalue.in_stack == 1) {
      closure->upvalues[index] = parent->registers[upvalue.idx];
    } else {
      closure->upvalues[index] = parent->upvalues[upvalue.idx];
    }
  }
}

static void delete_registers(moon_closure * closure) {
  moon_prototype prototype;

  read_closure_prototype(&prototype, closure);

  for (uint16_t index = 0; index < prototype.max_stack_size; ++index) {
    if (closure->registers[index] != NULL) {
      moon_free("delete_registers", closure->registers[index]);
      closure->registers[index] = NULL;
    }
  }
}

static void init_prototype(moon_prototype * prototype, PGM_VOID_P prototype_addr) {
  progmem_cpy(prototype, prototype_addr, sizeof(moon_prototype), 0);
}

static BOOL check_arithmetic_values(moon_value * value_a, moon_value * value_b) {
  if (value_a->type == LUA_NIL || value_b->type == LUA_NIL) {
    moon_debug("error: try to perform arithmetic on a nil value\n");
    return FALSE;
  }

  if (value_a->type == LUA_TRUE || value_b->type == LUA_TRUE || value_a->type == LUA_FALSE || value_b->type == LUA_FALSE) {
    moon_debug("error: try to perform arithmetic on a boolean value\n");
    return FALSE;
  }

  if (value_a->type == LUA_STRING || value_b->type == LUA_STRING) {
    moon_debug("error: try to perform arithmetic on a boolean value\n");
    return FALSE;
  }

  return TRUE;
}

static BOOL check_literal_values(moon_value * value_a, moon_value * value_b) {
  if (value_a->type == LUA_NIL || value_b->type == LUA_NIL) {
    moon_debug("error: try to perform literal manipulation on a nil value\n");
    return FALSE;
  }

  if (value_a->type == LUA_TRUE || value_b->type == LUA_TRUE || value_a->type == LUA_FALSE || value_b->type == LUA_FALSE) {
    moon_debug("error: try to perform literal manipulation on a boolean value\n");
    return FALSE;
  }

  if (value_a->type == LUA_INT || value_b->type == LUA_INT) {
    moon_debug("error: try to perform literal manipulation on a integer value\n");
    return FALSE;
  }

  if (value_a->type == LUA_NUMBER || value_b->type == LUA_NUMBER) {
    moon_debug("error: try to perform literal manipulation on a integer value\n");
    return FALSE;
  }

  return TRUE;
}

static void create_result_value(moon_reference * result, moon_value * value_a, moon_value * value_b, CTYPE_LUA_INT int_val, CTYPE_LUA_NUMBER number_val) {
  if (value_a->type == LUA_NUMBER || value_b->type == LUA_NUMBER) {
    create_number_value(result, number_val);
  } else {
    moon_create_int_value(result, int_val);
  }
}

static void read_instruction(moon_instruction * instruction, moon_prototype * prototype, uint16_t index) {
  progmem_cpy(instruction, prototype->instructions_addr, sizeof(moon_instruction), sizeof(moon_instruction) * index);
}

static void create_op_add_result(moon_reference * result, moon_value * value_a, moon_value * value_b) {
  CTYPE_LUA_INT int_val;
  CTYPE_LUA_NUMBER number_val;

  if (check_arithmetic_values(value_a, value_b) == FALSE) return;

  if (value_a->type == LUA_INT && value_b->type == LUA_INT) {
    int_val = ((moon_int_value *) value_a)->val + ((moon_int_value *) value_b)->val;
  } else if (value_a->type == LUA_INT && value_b->type == LUA_NUMBER) {
    number_val = ((moon_int_value *) value_a)->val + ((moon_number_value *) value_b)->val;
  } else if (value_a->type == LUA_NUMBER && value_b->type == LUA_INT) {
    number_val = ((moon_number_value *) value_a)->val + ((moon_int_value *) value_b)->val;
  } else if (value_a->type == LUA_NUMBER && value_b->type == LUA_NUMBER) {
    number_val = ((moon_number_value *) value_a)->val + ((moon_number_value *) value_b)->val;
  }

  create_result_value(result, value_a, value_b, int_val, number_val);
}

static void create_op_sub_result(moon_reference * result, moon_value * value_a, moon_value * value_b) {
  CTYPE_LUA_INT int_val;
  CTYPE_LUA_NUMBER number_val;

  if (check_arithmetic_values(value_a, value_b) == FALSE) return;

  if (value_a->type == LUA_INT && value_b->type == LUA_INT) {
    int_val = ((moon_int_value *) value_a)->val - ((moon_int_value *) value_b)->val;
  } else if (value_a->type == LUA_INT && value_b->type == LUA_NUMBER) {
    number_val = ((moon_int_value *) value_a)->val - ((moon_number_value *) value_b)->val;
  } else if (value_a->type == LUA_NUMBER && value_b->type == LUA_INT) {
    number_val = ((moon_number_value *) value_a)->val - ((moon_int_value *) value_b)->val;
  } else if (value_a->type == LUA_NUMBER && value_b->type == LUA_NUMBER) {
    number_val = ((moon_number_value *) value_a)->val - ((moon_number_value *) value_b)->val;
  }

  create_result_value(result, value_a, value_b, int_val, number_val);
}

static void create_op_div_result(moon_reference * result, moon_value * value_a, moon_value * value_b) {
  CTYPE_LUA_INT int_val;
  CTYPE_LUA_NUMBER number_val;

  if (check_arithmetic_values(value_a, value_b) == FALSE) return;

  if (value_a->type == LUA_INT && value_b->type == LUA_INT) {
    int_val = ((moon_int_value *) value_a)->val / ((moon_int_value *) value_b)->val;
  } else if (value_a->type == LUA_INT && value_b->type == LUA_NUMBER) {
    number_val = ((moon_int_value *) value_a)->val / ((moon_number_value *) value_b)->val;
  } else if (value_a->type == LUA_NUMBER && value_b->type == LUA_INT) {
    number_val = ((moon_number_value *) value_a)->val / ((moon_int_value *) value_b)->val;
  } else if (value_a->type == LUA_NUMBER && value_b->type == LUA_NUMBER) {
    number_val = ((moon_number_value *) value_a)->val / ((moon_number_value *) value_b)->val;
  }

  create_result_value(result, value_a, value_b, int_val, number_val);
}

static void create_op_mul_result(moon_reference * result, moon_value * value_a, moon_value * value_b) {
  CTYPE_LUA_INT int_val;
  CTYPE_LUA_NUMBER number_val;

  if (check_arithmetic_values(value_a, value_b) == FALSE) return;

  if (value_a->type == LUA_INT && value_b->type == LUA_INT) {
    int_val = ((moon_int_value *) value_a)->val * ((moon_int_value *) value_b)->val;
  } else if (value_a->type == LUA_INT && value_b->type == LUA_NUMBER) {
    number_val = ((moon_int_value *) value_a)->val * ((moon_number_value *) value_b)->val;
  } else if (value_a->type == LUA_NUMBER && value_b->type == LUA_INT) {
    number_val = ((moon_number_value *) value_a)->val * ((moon_int_value *) value_b)->val;
  } else if (value_a->type == LUA_NUMBER && value_b->type == LUA_NUMBER) {
    number_val = ((moon_number_value *) value_a)->val * ((moon_number_value *) value_b)->val;
  }

  create_result_value(result, value_a, value_b, int_val, number_val);
}

static void create_op_concat_result(moon_reference * result, moon_value * value_a, moon_value * value_b) {
  if (check_literal_values(value_a, value_b) == FALSE) return;

  uint16_t length = ((moon_string_value *) value_a)->length + ((moon_string_value *) value_b)->length;

  result->value_addr = (SRAM_ADDRESS) moon_malloc("create_op_concat_result", sizeof(moon_string_value));
  ((moon_string_value *) result->value_addr)->type = LUA_STRING;
  ((moon_string_value *) result->value_addr)->string_addr = (SRAM_ADDRESS) moon_malloc("create_op_concat_result (cstr)", length + 1);
  ((moon_string_value *) result->value_addr)->length = length;
  ((moon_string_value *) result->value_addr)->nodes = 1;

  sprintf(
    (char *) ((moon_string_value *) result->value_addr)->string_addr,
    "%s%s",
    (char *)((moon_string_value *) value_a)->string_addr,
    (char *)((moon_string_value *) value_b)->string_addr
  );
}


static void create_opb_buf(moon_reference * bufb_ref, moon_instruction * instruction, moon_closure * closure) {
  uint16_t instruction_b = MOON_READ_B(instruction);

  moon_reference const_ref;
  moon_prototype prototype;

  read_closure_prototype(&prototype, closure);

 if (MOON_IS_OPBK(instruction)) {
    read_constant_reference(&const_ref, &prototype, instruction_b);
    moon_create_value_copy(bufb_ref, &const_ref);
  } else {
    moon_create_value_copy(bufb_ref, closure->registers[instruction_b]);
  }
}

static void create_opc_buf(moon_reference * bufc_ref, moon_instruction * instruction, moon_closure * closure) {
  uint16_t instruction_c = MOON_READ_C(instruction);

  moon_reference const_ref;
  moon_prototype prototype;

  read_closure_prototype(&prototype, closure);

 if (MOON_IS_OPCK(instruction)) {
    read_constant_reference(&const_ref, &prototype, instruction_c);
    moon_create_value_copy(bufc_ref, &const_ref);
  } else {
    moon_create_value_copy(bufc_ref, closure->registers[instruction_c]);
  }
}

static void create_op_bufs(moon_reference * bufb_ref, moon_reference * bufc_ref, moon_instruction * instruction, moon_closure * closure) {
  create_opb_buf(bufb_ref, instruction, closure);
  create_opc_buf(bufc_ref, instruction, closure);
}

static void copy_op_bufs(moon_reference * bufb_ref, moon_reference * bufc_ref, moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);
  BOOL is_opbk = MOON_IS_OPBK(instruction);
  BOOL is_opck = MOON_IS_OPCK(instruction);

  moon_reference const_ref;
  moon_prototype prototype;

  read_closure_prototype(&prototype, closure);

  if (MOON_IS_OPCK(instruction)) {
    moon_create_value_copy(bufb_ref, closure->registers[instruction_b]);
    read_constant_reference(&const_ref, &prototype, instruction_c);
    copy_reference(bufc_ref, &const_ref);
  } else if (MOON_IS_OPBK(instruction)) {
    read_constant_reference(&const_ref, &prototype, instruction_b);
    copy_reference(bufb_ref, &const_ref);
    copy_reference(bufc_ref, closure->registers[instruction_c]);
  } else {
    copy_reference(bufb_ref, closure->registers[instruction_b]);
    copy_reference(bufc_ref, closure->registers[instruction_c]);
  }
}

static void run_instruction(moon_instruction * instruction, moon_closure * closure);

void moon_run_closure(moon_closure * closure, moon_closure * parent) {
  moon_prototype prototype;
  moon_instruction instruction;

  read_closure_prototype(&prototype, closure);

  closure->pc = 0;

  while(closure->pc < prototype.instructions_count) {
    read_instruction(&instruction, &prototype, closure->pc);
    run_instruction(&instruction, closure);
  }
}

static void copy_to_params(moon_closure * closure, moon_closure * sub_closure) {
  moon_prototype prototype;
  moon_prototype sub_prototype;

  read_closure_prototype(&prototype, closure);
  read_closure_prototype(&sub_prototype, sub_closure);

  for (uint16_t index = 0; index < (closure->top - closure->base); ++index) {
    copy_reference(sub_closure->registers[index], closure->registers[closure->base + index]);
  }
}

static void op_loadnil(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);

  moon_set_to_nil(closure->registers[instruction_a]);
}

static void op_loadbool(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);


  if (instruction_b == 1) {
    moon_set_to_true(closure->registers[instruction_a]);
  } else {
    moon_set_to_false(closure->registers[instruction_a]);
  }
}

static void op_loadk(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint32_t instruction_bx = MOON_READ_BX(instruction);

  moon_prototype prototype;
  moon_reference const_ref;

  read_closure_prototype(&prototype, closure);
  read_constant_reference(&const_ref, &prototype, instruction_bx);
  copy_reference(closure->registers[instruction_a], &const_ref);
}

static void op_add(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);

  moon_reference bufb_ref;
  moon_reference bufc_ref;
  moon_value * old_value = MOON_AS_VALUE(closure->registers[instruction_a]);
  BOOL old_value_is_progmem = closure->registers[instruction_a]->is_progmem;

  create_op_bufs(&bufb_ref, &bufc_ref, instruction, closure);
  create_op_add_result(closure->registers[instruction_a], (moon_value *) bufb_ref.value_addr, (moon_value *) bufc_ref.value_addr);

  closure->registers[instruction_a]->is_progmem = FALSE;

  if (old_value_is_progmem == FALSE) {
    old_value->nodes--;

    if (old_value->nodes <= 0) moon_delete_value(old_value);
  }

  if (bufb_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_sub(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);

  moon_reference bufb_ref;
  moon_reference bufc_ref;

  create_op_bufs(&bufb_ref, &bufc_ref, instruction, closure);
  create_op_sub_result(closure->registers[instruction_a], (moon_value *) bufb_ref.value_addr, (moon_value *) bufc_ref.value_addr);
  closure->registers[instruction_a]->is_progmem = FALSE;

  if (bufb_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_mul(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);

  moon_reference bufb_ref;
  moon_reference bufc_ref;

  create_op_bufs(&bufb_ref, &bufc_ref, instruction, closure);
  create_op_mul_result(closure->registers[instruction_a], (moon_value *) bufb_ref.value_addr, (moon_value *) bufc_ref.value_addr);
  closure->registers[instruction_a]->is_progmem = FALSE;

  if (bufb_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_div(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);

  moon_reference bufb_ref;
  moon_reference bufc_ref;

  create_op_bufs(&bufb_ref, &bufc_ref, instruction, closure);
  create_op_div_result(closure->registers[instruction_a], (moon_value *) bufb_ref.value_addr, (moon_value *) bufc_ref.value_addr);
  closure->registers[instruction_a]->is_progmem = FALSE;

  if (bufb_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_idiv(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);

  op_div(instruction, closure);

  if (MOON_IS_NUMBER(closure->registers[instruction_a])) {
    moon_create_int_value(closure->registers[instruction_a], (CTYPE_LUA_INT) MOON_AS_NUMBER(closure->registers[instruction_a])->val);
  }
}

static void op_move(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);

  copy_reference(closure->registers[instruction_a], closure->registers[instruction_b]);

  if (closure->registers[instruction_a]->is_progmem == FALSE) {
    ((moon_value *) closure->registers[instruction_a])->nodes++;
  }
}

static void op_concat(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);

  moon_reference bufb_ref;
  moon_reference bufc_ref;

  create_op_bufs(&bufb_ref, &bufc_ref, instruction, closure);
  create_op_concat_result(closure->registers[instruction_a], (moon_value *) bufb_ref.value_addr, (moon_value *) bufc_ref.value_addr);
  closure->registers[instruction_a]->is_progmem = FALSE;

  if (bufb_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_eq(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);

  moon_reference bufb_ref;
  moon_reference bufc_ref;

  create_op_bufs(&bufb_ref, &bufc_ref, instruction, closure);

  if ((instruction_a == 0 && (equals(&bufb_ref, &bufc_ref) == TRUE)) || (instruction_a == 1 && (equals(&bufb_ref, &bufc_ref) == FALSE))) {
    closure->pc++;
  }

  if (bufb_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_lt(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);

  moon_reference bufb_ref;
  moon_reference bufc_ref;

  create_op_bufs(&bufb_ref, &bufc_ref, instruction, closure);

  if ((instruction_a == 0 && (is_lower(&bufb_ref, &bufc_ref) == TRUE)) || (instruction_a == 1 && (is_lower(&bufb_ref, &bufc_ref) == FALSE))) {
    closure->pc++;
  }

  if (bufb_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_le(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);

  moon_reference bufb_ref;
  moon_reference bufc_ref;

  create_op_bufs(&bufb_ref, &bufc_ref, instruction, closure);

  if ((instruction_a == 0 && ((is_lower(&bufb_ref, &bufc_ref) == TRUE) || (equals(&bufb_ref, &bufc_ref) == TRUE)))
      || (instruction_a == 1 && ((is_lower(&bufb_ref, &bufc_ref) == FALSE) || (equals(&bufb_ref, &bufc_ref) == FALSE)))) {
    closure->pc++;
  }

  if (bufb_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_not(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);

  moon_reference bufb_ref;

  create_opb_buf(&bufb_ref, instruction, closure);

  if (pass_test(&bufb_ref) == TRUE) {
    moon_set_to_false(closure->registers[instruction_a]);
  } else {
    moon_set_to_true(closure->registers[instruction_a]);
  }

  if (bufb_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufb_ref.value_addr);
}

static void op_closure(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint32_t instruction_bx = MOON_READ_BX(instruction);

  moon_closure * sub_closure;
  moon_prototype prototype;

  read_closure_prototype(&prototype, closure);
  sub_closure = moon_create_closure(prototype.prototypes_addr, instruction_bx, closure);

  closure->registers[instruction_a]->value_addr = (SRAM_ADDRESS) sub_closure;
  closure->registers[instruction_a]->is_progmem = FALSE;
  closure->registers[instruction_a]->is_copy = FALSE;
}

static void op_settabup(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);

  moon_reference bufb_ref;
  moon_reference bufc_ref;

  copy_op_bufs(&bufb_ref, &bufc_ref, instruction, closure);
  set_hash_pair(&(closure->in_upvalues), &bufb_ref, &bufc_ref);

  if (bufb_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_gettabup(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);

  moon_reference bufc_ref;
  moon_reference const_ref;
  moon_prototype prototype;

  read_closure_prototype(&prototype, closure);

  if (MOON_IS_OPCK(instruction)) {
    read_constant_reference(&const_ref, &prototype, instruction_c);
    copy_reference(&bufc_ref, &const_ref);
  } else {
    copy_reference(&bufc_ref, closure->registers[instruction_c]);
  }

  moon_find_closure_value(closure->registers[instruction_a], closure, &bufc_ref);

  if (bufc_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_setupval(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint8_t instruction_b = MOON_READ_B(instruction);

  copy_reference(closure->upvalues[instruction_b], closure->registers[instruction_a]);
}

static void op_getupval(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);

  copy_reference(closure->registers[instruction_a], closure->upvalues[instruction_b]);
}

static void op_call(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);

  moon_closure * sub_closure;
  moon_reference bufa_ref;

  closure->base = instruction_a + 1;

  if (instruction_b > 1) closure->top = closure->base + (instruction_b - 1);

  if (MOON_AS_VALUE(closure->registers[instruction_a])->type != LUA_CLOSURE && MOON_AS_VALUE(closure->registers[instruction_a])->type != LUA_API) {
    moon_debug("error: trying to call a non closure type");
    return;
  }

  if (MOON_AS_VALUE(closure->registers[instruction_a])->type == LUA_API) {
    (*MOON_AS_API(closure->registers[instruction_a])->func)(closure, (instruction_b != 1) ? TRUE : FALSE);
  } else {
    sub_closure = (moon_closure *) closure->registers[instruction_a]->value_addr;

    if (instruction_b != 1) copy_to_params(closure, sub_closure);

    moon_run_closure(sub_closure, closure);

    if (instruction_c != 1) {
      // @TODO : manage multiple results (c > 2)
      copy_reference(closure->registers[instruction_a], &(sub_closure->result));
      if (instruction_c == 0) closure->top = instruction_a + 1;
    }

    // @TODO : delete registers managing upvalues
    // delete_registers(sub_closure);
    moon_set_to_nil(&(sub_closure->result));
  }

  if (bufa_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufa_ref.value_addr);
}

static void op_return(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);

  if (instruction_b == 1) return;  // @TODO : manage multipe results

  copy_reference(&(closure->result), closure->registers[instruction_a]);
}

static void op_jmp(moon_instruction * instruction, moon_closure * closure) {
  int32_t sbx = MOON_READ_SBX(instruction);

  closure->pc += sbx;
}

static void op_test(moon_instruction * instruction, moon_closure * closure) {
  moon_reference bufa_ref;

  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);

  moon_create_value_copy(&bufa_ref, closure->registers[instruction_a]);

  if ((instruction_c == 0 && (pass_test(&bufa_ref) == TRUE)) || (instruction_c == 1 && (pass_test(&bufa_ref) == FALSE))) {
    closure->pc++;
  }

  if (bufa_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufa_ref.value_addr);
}

static void op_testset(moon_instruction * instruction, moon_closure * closure) {
  moon_reference bufb_ref;

  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);

  moon_create_value_copy(&bufb_ref, closure->registers[instruction_b]);

  if ((instruction_c == 0 && (pass_test(&bufb_ref) == TRUE)) || (instruction_c == 1 && (pass_test(&bufb_ref) == FALSE))) {
    copy_reference(closure->registers[instruction_a], closure->registers[instruction_b]);
    closure->pc++;
  }

  if (bufb_ref.is_copy == TRUE) moon_delete_value((moon_value *) bufb_ref.value_addr);
}

static void run_instruction(moon_instruction * instruction, moon_closure * closure) {
  uint8_t opcode = MOON_READ_OPCODE(instruction);

  switch(opcode) {
    case OPCODE_LOADNIL:
      op_loadnil(instruction, closure);
      break;
    case OPCODE_LOADBOOL:
      op_loadbool(instruction, closure);
      break;
    case OPCODE_LOADK:
      op_loadk(instruction, closure);
      break;
    case OPCODE_ADD:
      op_add(instruction, closure);
      break;
    case OPCODE_SUB:
      op_sub(instruction, closure);
      break;
    case OPCODE_MUL:
      op_mul(instruction, closure);
      break;
    case OPCODE_DIV:
      op_div(instruction, closure);
      break;
    case OPCODE_IDIV:
      op_idiv(instruction, closure);
      break;
    case OPCODE_MOVE:
      op_move(instruction, closure);
      break;
    case OPCODE_CONCAT:
      op_concat(instruction, closure);
      break;
    case OPCODE_CLOSURE:
      op_closure(instruction, closure);
      break;
    case OPCODE_SETTABUP:
      op_settabup(instruction, closure);
      break;
    case OPCODE_GETTABUP:
      op_gettabup(instruction, closure);
      break;
    case OPCODE_SETUPVAL:
      op_setupval(instruction, closure);
      break;
    case OPCODE_GETUPVAL:
      op_getupval(instruction, closure);
      break;
    case OPCODE_TAILCALL:  // WEAK, @TODO : real tailcall
    case OPCODE_CALL:
      op_call(instruction, closure);
      break;
    case OPCODE_JMP:
      op_jmp(instruction, closure);
      break;
    case OPCODE_TEST:
      op_test(instruction, closure);
      break;
    case OPCODE_TESTSET:
      op_testset(instruction, closure);
      break;
    case OPCODE_EQ:
      op_eq(instruction, closure);
      break;
    case OPCODE_NOT:
      op_not(instruction, closure);
      break;
    case OPCODE_LT:
      op_lt(instruction, closure);
      break;
    case OPCODE_LE:
      op_le(instruction, closure);
      break;

    case OPCODE_RETURN:
      op_return(instruction, closure);
      break;

    default:
      moon_debug("unknown upcode : %d\n", opcode);
      break;
  };

  closure->pc++;
}

void moon_add_global(const char * key_str, moon_reference * value_reference) {
  moon_reference key_reference = {.is_progmem = FALSE, .is_copy = FALSE, .value_addr = NULL};

  moon_create_string_value(&key_reference, key_str);
  set_hash_pair(&globals_hash, &key_reference, value_reference);
}

void moon_add_global_api_func(const char * key_str, void (*api_func)(moon_closure *, BOOL)) {
  moon_reference reference;

  create_api_value(&reference, api_func);
  moon_add_global(key_str, &reference);
}

void moon_init() {
  init_hash(&globals_hash);
}

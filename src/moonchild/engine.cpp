#include "moonchild.h"

#include <stdio.h>
#include <stdlib.h>


static void progmem_cpy(void * dest, PGMEM_ADDRESS src, uint16_t size, uint16_t offset = 0) {
  char * cdest = (char *)dest;

  for (uint16_t index = 0; index < size; ++index) {
    cdest[index] = progmem_read(src, index + offset);
  }
}

static void set_to_nil(moon_reference * reference) {
  reference->is_progmem = TRUE;
  reference->value_addr = (SRAM_ADDRESS) &MOON_NIL_VALUE;
}

static void set_to_true(moon_reference * reference) {
  reference->is_progmem = TRUE;
  reference->value_addr = (SRAM_ADDRESS) &MOON_TRUE_VALUE;
}

static void set_to_false(moon_reference * reference) {
  reference->is_progmem = TRUE;
  reference->value_addr = (SRAM_ADDRESS) &MOON_FALSE_VALUE;
}

static void create_int_value(moon_reference * reference, CTYPE_LUA_INT int_val) {
  reference->value_addr = (SRAM_ADDRESS) malloc(sizeof(moon_int_value));
  ((moon_int_value *) reference->value_addr)->type = LUA_INT;
  ((moon_int_value *) reference->value_addr)->val = int_val;
  ((moon_int_value *) reference->value_addr)->nodes = 1;
}

static void create_number_value(moon_reference * reference, CTYPE_LUA_NUMBER number_val) {
  reference->value_addr = (SRAM_ADDRESS) malloc(sizeof(moon_number_value));
  ((moon_number_value *) reference->value_addr)->type = LUA_NUMBER;
  ((moon_number_value *) reference->value_addr)->val = number_val;
  ((moon_number_value *) reference->value_addr)->nodes = 1;
}

static void init_registers(moon_closure * closure);
static void init_upvalues(moon_closure * closure);
static void init_hash(moon_hash * hash);

static moon_closure * create_closure(PGMEM_ADDRESS prototype_addr, uint16_t prototype_addr_cursor = 0, moon_closure * parent = NULL) {
  moon_closure * closure = (moon_closure *) malloc(sizeof(moon_closure));

  closure->type = LUA_CLOSURE;
  closure->nodes = 0;
  closure->prototype_addr = prototype_addr;
  closure->prototype_addr_cursor = prototype_addr_cursor;

  init_hash(&(closure->up_values));
  init_registers(closure);
  init_upvalues(closure);
  set_to_nil(&(closure->result));

  return closure;
}

static moon_prototype * create_prototype() {
  return (moon_prototype *) malloc(sizeof(moon_prototype));
}

static void delete_value(moon_value * value) {
  if (value == NULL) return;

  if (value->type == LUA_STRING) {
    // @TODO : free string
  }

  free(value);
  value = NULL;
}

static void create_progmem_value_copy(moon_reference * dest, moon_reference * src) {
  uint8_t type = progmem_read(src->value_addr, 0);
  moon_string_value string_value;

  switch(type) {
    case LUA_NIL:
    case LUA_TRUE:
    case LUA_FALSE:
      dest->value_addr = (SRAM_ADDRESS) malloc(sizeof(moon_value));
      progmem_cpy(dest->value_addr, src->value_addr, sizeof(moon_value));
      dest->is_copy = TRUE;
      dest->is_progmem = FALSE;
      break;

    case LUA_INT:
      dest->value_addr = (SRAM_ADDRESS) malloc(sizeof(moon_int_value));
      progmem_cpy(dest->value_addr, src->value_addr, sizeof(moon_int_value));
      dest->is_copy = TRUE;
      dest->is_progmem = FALSE;
      break;

    case LUA_NUMBER:
      dest->value_addr = (SRAM_ADDRESS) malloc(sizeof(moon_number_value));
      progmem_cpy(dest->value_addr, src->value_addr, sizeof(moon_number_value));
      dest->is_copy = TRUE;
      dest->is_progmem = FALSE;
      break;

    case LUA_STRING:
      dest->value_addr = (SRAM_ADDRESS) malloc(sizeof(moon_string_value));
      dest->is_progmem = FALSE;

      progmem_cpy(&string_value, src->value_addr, sizeof(moon_string_value));
      progmem_cpy(dest->value_addr, src->value_addr, sizeof(moon_string_value));

      ((moon_string_value *) dest->value_addr)->string_addr = (SRAM_ADDRESS) malloc(((moon_string_value *) dest->value_addr)->length + 1);

      progmem_cpy(((moon_string_value *) dest->value_addr)->string_addr, string_value.string_addr, ((moon_string_value *) dest->value_addr)->length);

      ((char *)(((moon_string_value *) dest->value_addr)->string_addr))[((moon_string_value *) dest->value_addr)->length] = '\0';

      dest->is_copy = TRUE;
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

static void create_value_copy(moon_reference * dest, moon_reference * src) {
  if (src->is_progmem == TRUE) {
    create_progmem_value_copy(dest, src);
  } else {
    copy_reference(dest, src);
  }
}

static void ref_to_str(char * result, moon_reference * reference) {
  moon_reference buf_ref;

  create_value_copy(&buf_ref, reference);

  switch(((moon_value *) buf_ref.value_addr)->type) {
    case LUA_NIL:
      sprintf(result, "\nnil\n");
      break;
    case LUA_FALSE:
      sprintf(result, "\nfalse\n");
      break;
    case LUA_TRUE:
      sprintf(result, "\ntrue\n");
      break;
    case LUA_INT:
      sprintf(result, "\n%d\n", ((moon_int_value *) buf_ref.value_addr)->val);
      break;
    case LUA_NUMBER:
      sprintf(result, "\n~%d\n", (int)((moon_number_value *) buf_ref.value_addr)->val);
      break;
    case LUA_STRING:
      sprintf(result, "\n%s\n", (char *)(((moon_string_value *) buf_ref.value_addr)->string_addr));
      break;

    default:
      sprintf(result, "\nother type : %d\n", ((moon_value *) buf_ref.value_addr)->type);
      break;
  };

  if (buf_ref.is_copy == TRUE) delete_value((moon_value *) buf_ref.value_addr);
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

  if (MOON_AS_STRING(ref_a)->length != MOON_AS_STRING(ref_b)->length) return false;

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

static void set_hash_pair(moon_hash * hash, moon_reference * key_reference, moon_reference * value_reference) {
  moon_hash_pair * pair = (moon_hash_pair *) malloc(sizeof(moon_hash_pair));

  create_value_copy(&(pair->key_reference), key_reference);
  copy_reference(&(pair->value_reference), value_reference);

  if (hash->count == 0) {
    hash->first = pair;
  } else {
    hash->last->next = pair;
  }

  hash->last = pair;
  hash->count++;
}

static void find_hash_value(moon_reference * result, moon_hash * hash, moon_reference * key_reference) {
  BOOL found;
  char * key_str;
  char * input_key_str;
  moon_string_value * key_as_val;
  moon_reference buf_ref;
  moon_hash_pair * pair = hash->first;
  moon_string_value * input_key_as_val;

  set_to_nil(result);
  create_value_copy(&buf_ref, key_reference);

  input_key_as_val = (moon_string_value *) buf_ref.value_addr;
  input_key_str = (char *) input_key_as_val->string_addr;

  for (uint16_t index = 0; index < hash->count; ++index) {
    key_as_val = (moon_string_value *) (pair->key_reference).value_addr;
    key_str = (char *) key_as_val->string_addr;
    found = TRUE;

    if (key_as_val->length != input_key_as_val->length) continue;

    for (uint16_t car_index = 0; car_index < input_key_as_val->length; ++car_index) {
      if (key_str[car_index] != input_key_str[car_index]) {
        found = FALSE;
        break;
      }
    }

    if (found == TRUE) {
      copy_reference(result, &(pair->value_reference));
      break;
    }

    pair = pair->next;
  }

  if (buf_ref.is_copy == TRUE) delete_value((moon_value *) buf_ref.value_addr);
}


static void copy_constant_reference(moon_reference * dest, moon_prototype * prototype, uint16_t index) {
  moon_reference constant_reference;

  read_constant_reference(&constant_reference, prototype, index);
  copy_reference(dest, &constant_reference);
}

static void create_register(moon_closure * closure, uint16_t index) {
  if (closure->registers[index] != NULL) return;

  closure->registers[index] = (moon_reference *) malloc(sizeof(moon_reference));
  set_to_nil(closure->registers[index]);
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
      free(closure->registers[index]);
      closure->registers[index] = NULL;
    }
  }
}

static void init_prototype(moon_prototype * prototype, PGMEM_ADDRESS prototype_addr) {
  progmem_cpy(prototype, prototype_addr, sizeof(moon_prototype));
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
    create_int_value(result, int_val);
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

  result->value_addr = (SRAM_ADDRESS) malloc(sizeof(moon_string_value));
  ((moon_string_value *) result->value_addr)->type = LUA_STRING;
  ((moon_string_value *) result->value_addr)->string_addr = (SRAM_ADDRESS) malloc(length + 1);
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
    create_value_copy(bufb_ref, &const_ref);
  } else {
    create_value_copy(bufb_ref, closure->registers[instruction_b]);
  }
}

static void create_opc_buf(moon_reference * bufc_ref, moon_instruction * instruction, moon_closure * closure) {
  uint16_t instruction_c = MOON_READ_C(instruction);

  moon_reference const_ref;
  moon_prototype prototype;

  read_closure_prototype(&prototype, closure);

 if (MOON_IS_OPCK(instruction)) {
    read_constant_reference(&const_ref, &prototype, instruction_c);
    create_value_copy(bufc_ref, &const_ref);
  } else {
    create_value_copy(bufc_ref, closure->registers[instruction_c]);
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
    create_value_copy(bufb_ref, closure->registers[instruction_b]);
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

static void run_closure(moon_closure * closure, moon_closure * parent = NULL) {
  moon_prototype prototype;
  moon_instruction instruction;

  read_closure_prototype(&prototype, closure);
  create_registers(closure);
  if (parent != NULL) create_upvalues(closure, parent);

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

  set_to_nil(closure->registers[instruction_a]);
}

static void op_loadbool(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);


  if (instruction_b == 1) {
    set_to_true(closure->registers[instruction_a]);
  } else {
    set_to_false(closure->registers[instruction_a]);
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

  create_op_bufs(&bufb_ref, &bufc_ref, instruction, closure);
  create_op_add_result(closure->registers[instruction_a], (moon_value *) bufb_ref.value_addr, (moon_value *) bufc_ref.value_addr);
  closure->registers[instruction_a]->is_progmem = FALSE;

  if (bufb_ref.is_copy == TRUE) delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) delete_value((moon_value *) bufc_ref.value_addr);
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

  if (bufb_ref.is_copy == TRUE) delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) delete_value((moon_value *) bufc_ref.value_addr);
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

  if (bufb_ref.is_copy == TRUE) delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) delete_value((moon_value *) bufc_ref.value_addr);
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

  if (bufb_ref.is_copy == TRUE) delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_idiv(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);

  op_div(instruction, closure);

  if (MOON_IS_NUMBER(closure->registers[instruction_a])) {
    create_int_value(closure->registers[instruction_a], (CTYPE_LUA_INT) MOON_AS_NUMBER(closure->registers[instruction_a])->val);
  }
}

static void op_move(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);

  copy_reference(closure->registers[instruction_a], closure->registers[instruction_b]);

  if (! closure->registers[instruction_a]->is_progmem) {
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

  if (bufb_ref.is_copy == TRUE) delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) delete_value((moon_value *) bufc_ref.value_addr);
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

  if (bufb_ref.is_copy == TRUE) delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) delete_value((moon_value *) bufc_ref.value_addr);
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

  if (bufb_ref.is_copy == TRUE) delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) delete_value((moon_value *) bufc_ref.value_addr);
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

  if (bufb_ref.is_copy == TRUE) delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_not(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);

  moon_reference bufb_ref;

  create_opb_buf(&bufb_ref, instruction, closure);

  if (pass_test(&bufb_ref) == TRUE) {
    set_to_false(closure->registers[instruction_a]);
  } else {
    set_to_true(closure->registers[instruction_a]);
  }

  if (bufb_ref.is_copy == TRUE) delete_value((moon_value *) bufb_ref.value_addr);
}

static void op_closure(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint32_t instruction_bx = MOON_READ_BX(instruction);

  moon_closure * sub_closure;
  moon_prototype prototype;

  read_closure_prototype(&prototype, closure);
  sub_closure = create_closure(prototype.prototypes_addr, instruction_bx, closure);

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
  set_hash_pair(&(closure->up_values), &bufb_ref, &bufc_ref);

  if (bufb_ref.is_copy == TRUE) delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) delete_value((moon_value *) bufc_ref.value_addr);
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

  find_hash_value(closure->registers[instruction_a], &(closure->up_values), &bufc_ref);

  if (bufc_ref.is_copy == TRUE) delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_setupval(moon_instruction * instruction, moon_closure * closure) {
  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);

  copy_reference(closure->upvalues[instruction_a], closure->registers[instruction_b]);
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

  if (((moon_value *) closure->registers[instruction_a]->value_addr)->type != LUA_CLOSURE) {
    moon_debug("error: trying to call a non closure type");
    return;
  }

  sub_closure = (moon_closure *) closure->registers[instruction_a]->value_addr;

  create_registers(sub_closure);

  closure->base = instruction_a + 1;

  if (instruction_b != 1) {
    if (instruction_b > 1) closure->top = closure->base + (instruction_b - 1);

    copy_to_params(closure, sub_closure);
  }

  run_closure(sub_closure, closure);

  if (instruction_c != 1) {
    // @TODO : manage multiple results (c > 2)
    copy_reference(closure->registers[instruction_a], &(sub_closure->result));
    if (instruction_c == 0) closure->top = instruction_a + 1;
  }

  delete_registers(sub_closure);
  set_to_nil(&(sub_closure->result));

  if (bufa_ref.is_copy == TRUE) delete_value((moon_value *) bufa_ref.value_addr);
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

  create_value_copy(&bufa_ref, closure->registers[instruction_a]);

  if ((instruction_c == 0 && (pass_test(&bufa_ref) == TRUE)) || (instruction_c == 1 && (pass_test(&bufa_ref) == FALSE))) {
    closure->pc++;
  }

  if (bufa_ref.is_copy == TRUE) delete_value((moon_value *) bufa_ref.value_addr);
}

static void op_testset(moon_instruction * instruction, moon_closure * closure) {
  moon_reference bufb_ref;

  uint8_t instruction_a = MOON_READ_A(instruction);
  uint16_t instruction_b = MOON_READ_B(instruction);
  uint16_t instruction_c = MOON_READ_C(instruction);

  create_value_copy(&bufb_ref, closure->registers[instruction_b]);

  if ((instruction_c == 0 && (pass_test(&bufb_ref) == TRUE)) || (instruction_c == 1 && (pass_test(&bufb_ref) == FALSE))) {
    copy_reference(closure->registers[instruction_a], closure->registers[instruction_b]);
    closure->pc++;
  }

  if (bufb_ref.is_copy == TRUE) delete_value((moon_value *) bufb_ref.value_addr);
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


void moon_run(PGMEM_ADDRESS prototype_addr, char * result) {
  moon_closure * closure = create_closure(prototype_addr);

  run_closure(closure);
  ref_to_str(result, &(closure->result));
}

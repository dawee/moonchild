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

static void init_registers(moon_closure * closure);
static void init_hash(moon_hash * hash);

static moon_closure * create_closure(PGMEM_ADDRESS prototype_addr) {
  moon_closure * closure = (moon_closure *) malloc(sizeof(moon_closure));

  closure->type = LUA_CLOSURE;
  closure->prototype_addr = prototype_addr;

  init_hash(&(closure->up_values));
  init_registers(closure);
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
      break;

    case LUA_INT:
      dest->value_addr = (SRAM_ADDRESS) malloc(sizeof(moon_int_value));
      progmem_cpy(dest->value_addr, src->value_addr, sizeof(moon_int_value));
      dest->is_copy = TRUE;
      break;

    case LUA_NUMBER:
      dest->value_addr = (SRAM_ADDRESS) malloc(sizeof(moon_number_value));
      progmem_cpy(dest->value_addr, src->value_addr, sizeof(moon_number_value));
      dest->is_copy = TRUE;
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

static void read_constant_reference(moon_reference * reference, moon_prototype * prototype, uint16_t index) {
  progmem_cpy(reference, prototype->constants_addr, sizeof(moon_reference), sizeof(moon_reference) * index);
}

static void read_closure_prototype(moon_prototype * prototype, moon_closure * closure) {
  progmem_cpy(prototype, closure->prototype_addr, sizeof(moon_prototype));
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

static void create_registers(moon_closure * closure) {
  moon_prototype prototype;

  read_closure_prototype(&prototype, closure);

  for (uint16_t index = 0; index < prototype.max_stack_size; ++index) {
    create_register(closure, index);
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
    result->value_addr = (SRAM_ADDRESS) malloc(sizeof(moon_number_value));
    ((moon_number_value *) result->value_addr)->type = LUA_NUMBER;
    ((moon_number_value *) result->value_addr)->val = number_val;
    ((moon_number_value *) result->value_addr)->nodes = 1;
  } else {
    result->value_addr = (SRAM_ADDRESS) malloc(sizeof(moon_int_value));
    ((moon_int_value *) result->value_addr)->type = LUA_INT;
    ((moon_int_value *) result->value_addr)->val = int_val;
    ((moon_int_value *) result->value_addr)->nodes = 1;
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

static void create_op_bufs(moon_reference * bufb_ref, moon_reference * bufc_ref, moon_instruction * instruction, moon_closure * closure) {
  moon_reference const_ref;
  moon_prototype prototype;

  read_closure_prototype(&prototype, closure);

  if ((instruction->flag & OPCK_FLAG) == OPCK_FLAG) {
    create_value_copy(bufb_ref, closure->registers[instruction->b]);
    read_constant_reference(&const_ref, &prototype, instruction->c);
    create_value_copy(bufc_ref, &const_ref);
  } else if ((instruction->flag & OPBK_FLAG) == OPBK_FLAG) {
    read_constant_reference(&const_ref, &prototype, instruction->b);
    create_value_copy(bufb_ref, &const_ref);
    create_value_copy(bufc_ref, closure->registers[instruction->c]);
  } else {
    create_value_copy(bufb_ref, closure->registers[instruction->b]);
    create_value_copy(bufc_ref, closure->registers[instruction->c]);
  }
}

static void copy_op_bufs(moon_reference * bufb_ref, moon_reference * bufc_ref, moon_instruction * instruction, moon_closure * closure) {
  moon_reference const_ref;
  moon_prototype prototype;

  read_closure_prototype(&prototype, closure);

  if ((instruction->flag & OPCK_FLAG) == OPCK_FLAG) {
    create_value_copy(bufb_ref, closure->registers[instruction->b]);
    read_constant_reference(&const_ref, &prototype, instruction->c);
    copy_reference(bufc_ref, &const_ref);
  } else if ((instruction->flag & OPBK_FLAG) == OPBK_FLAG) {
    read_constant_reference(&const_ref, &prototype, instruction->b);
    copy_reference(bufb_ref, &const_ref);
    copy_reference(bufc_ref, closure->registers[instruction->c]);
  } else {
    copy_reference(bufb_ref, closure->registers[instruction->b]);
    copy_reference(bufc_ref, closure->registers[instruction->c]);
  }
}

static void run_instruction(moon_instruction * instruction, moon_closure * closure);

static void run_closure(moon_closure * closure) {
  moon_prototype prototype;
  moon_instruction instruction;

  read_closure_prototype(&prototype, closure);
  create_registers(closure);

  for (uint16_t index = 0; index < prototype.instructions_count; ++index) {
    read_instruction(&instruction, &prototype, index);
    run_instruction(&instruction, closure);
  }
}

static void copy_to_params(moon_closure * closure, moon_closure * sub_closure, uint16_t count) {
  moon_prototype prototype;
  moon_prototype sub_prototype;

  read_closure_prototype(&prototype, closure);
  read_closure_prototype(&sub_prototype, sub_closure);

  for (uint16_t index = 0; index < count; ++index) {
    copy_reference(sub_closure->registers[index], closure->registers[prototype.max_stack_size - index - 1]);
  }
}

static void op_loadnil(moon_instruction * instruction, moon_closure * closure) {
  set_to_nil(closure->registers[instruction->a]);
}

static void op_loadbool(moon_instruction * instruction, moon_closure * closure) {
  if (instruction->b == 1) {
    set_to_true(closure->registers[instruction->a]);
  } else {
    set_to_false(closure->registers[instruction->a]);
  }
}

static void op_loadk(moon_instruction * instruction, moon_closure * closure) {
  moon_prototype prototype;
  moon_reference const_ref;

  read_closure_prototype(&prototype, closure);
  read_constant_reference(&const_ref, &prototype, instruction->b);
  copy_reference(closure->registers[instruction->a], &const_ref);
}

static void op_add(moon_instruction * instruction, moon_closure * closure) {
  moon_reference bufb_ref;
  moon_reference bufc_ref;

  create_op_bufs(&bufb_ref, &bufc_ref, instruction, closure);
  create_op_add_result(closure->registers[instruction->a], (moon_value *) bufb_ref.value_addr, (moon_value *) bufc_ref.value_addr);
  closure->registers[instruction->a]->is_progmem = FALSE;

  if (bufb_ref.is_copy == TRUE) delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_sub(moon_instruction * instruction, moon_closure * closure) {
  moon_reference bufb_ref;
  moon_reference bufc_ref;

  create_op_bufs(&bufb_ref, &bufc_ref, instruction, closure);
  create_op_sub_result(closure->registers[instruction->a], (moon_value *) bufb_ref.value_addr, (moon_value *) bufc_ref.value_addr);
  closure->registers[instruction->a]->is_progmem = FALSE;

  if (bufb_ref.is_copy == TRUE) delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_mul(moon_instruction * instruction, moon_closure * closure) {
  moon_reference bufb_ref;
  moon_reference bufc_ref;

  create_op_bufs(&bufb_ref, &bufc_ref, instruction, closure);
  create_op_mul_result(closure->registers[instruction->a], (moon_value *) bufb_ref.value_addr, (moon_value *) bufc_ref.value_addr);
  closure->registers[instruction->a]->is_progmem = FALSE;

  if (bufb_ref.is_copy == TRUE) delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_div(moon_instruction * instruction, moon_closure * closure) {
  moon_reference bufb_ref;
  moon_reference bufc_ref;

  create_op_bufs(&bufb_ref, &bufc_ref, instruction, closure);
  create_op_div_result(closure->registers[instruction->a], (moon_value *) bufb_ref.value_addr, (moon_value *) bufc_ref.value_addr);
  closure->registers[instruction->a]->is_progmem = FALSE;

  if (bufb_ref.is_copy == TRUE) delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_move(moon_instruction * instruction, moon_closure * closure) {
  copy_reference(closure->registers[instruction->a], closure->registers[instruction->b]);

  if (! closure->registers[instruction->a]->is_progmem) {
    ((moon_value *) closure->registers[instruction->a])->nodes++;
  }
}

static void op_concat(moon_instruction * instruction, moon_closure * closure) {
  moon_reference bufb_ref;
  moon_reference bufc_ref;

  create_op_bufs(&bufb_ref, &bufc_ref, instruction, closure);
  create_op_concat_result(closure->registers[instruction->a], (moon_value *) bufb_ref.value_addr, (moon_value *) bufc_ref.value_addr);
  closure->registers[instruction->a]->is_progmem = FALSE;

  if (bufb_ref.is_copy == TRUE) delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_closure(moon_instruction * instruction, moon_closure * closure) {
  moon_closure * sub_closure;
  moon_prototype prototype;
  PGMEM_ADDRESS prototype_addr;

  read_closure_prototype(&prototype, closure);
  prototype_addr = prototype.prototypes_addr + (sizeof(moon_prototype) * instruction->b);
  sub_closure = create_closure(prototype_addr);

  closure->registers[instruction->a]->value_addr = (SRAM_ADDRESS) sub_closure;
  closure->registers[instruction->a]->is_progmem = FALSE;
  closure->registers[instruction->a]->is_copy = FALSE;
}

static void op_settabup(moon_instruction * instruction, moon_closure * closure) {
  moon_reference bufb_ref;
  moon_reference bufc_ref;

  copy_op_bufs(&bufb_ref, &bufc_ref, instruction, closure);
  set_hash_pair(&(closure->up_values), &bufb_ref, &bufc_ref);

  if (bufb_ref.is_copy == TRUE) delete_value((moon_value *) bufb_ref.value_addr);
  if (bufc_ref.is_copy == TRUE) delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_gettabup(moon_instruction * instruction, moon_closure * closure) {
  moon_reference bufc_ref;
  moon_reference const_ref;
  moon_prototype prototype;

  read_closure_prototype(&prototype, closure);

  if ((instruction->flag & OPCK_FLAG) == OPCK_FLAG) {
    read_constant_reference(&const_ref, &prototype, instruction->c);
    copy_reference(&bufc_ref, &const_ref);
  } else {
    copy_reference(&bufc_ref, closure->registers[instruction->c]);
  }

  find_hash_value(closure->registers[instruction->a], &(closure->up_values), &bufc_ref);

  if (bufc_ref.is_copy == TRUE) delete_value((moon_value *) bufc_ref.value_addr);
}

static void op_call(moon_instruction * instruction, moon_closure * closure) {
  moon_closure * sub_closure;
  moon_reference bufa_ref;

  if (((moon_value *) closure->registers[instruction->a]->value_addr)->type != LUA_CLOSURE) {
    moon_debug("error: trying to call a non closure type");
    return;
  }

  sub_closure = (moon_closure *) closure->registers[instruction->a]->value_addr;

  create_registers(sub_closure);

  if (instruction->b > 1) copy_to_params(closure, sub_closure, instruction->b - 1);
  run_closure(sub_closure);

  if (bufa_ref.is_copy == TRUE) delete_value((moon_value *) bufa_ref.value_addr);
}

static void run_instruction(moon_instruction * instruction, moon_closure * closure) {
  switch(instruction->opcode) {
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
    case OPCODE_CALL:
      op_call(instruction, closure);
      break;

    case OPCODE_RETURN:
      // @TODO : manage return value
      break;

    default:
      moon_debug("unknown upcode : %d\n", instruction->opcode);
      break;
  };

}


void moon_run(PGMEM_ADDRESS prototype_addr, char * result) {
  moon_closure * closure = create_closure(prototype_addr);
  moon_reference buf_ref;

  run_closure(closure);
  create_value_copy(&buf_ref, &(closure->result));

  switch(((moon_value *) buf_ref.value_addr)->type) {
    case LUA_NIL:
      sprintf(result, "nil\n");
      break;
    case LUA_FALSE:
      sprintf(result, "false\n");
      break;
    case LUA_TRUE:
      sprintf(result, "true\n");
      break;
    case LUA_INT:
      sprintf(result, "%d\n", ((moon_int_value *) buf_ref.value_addr)->val);
      break;
    case LUA_NUMBER:
      sprintf(result, "~%d\n", (int)((moon_number_value *) buf_ref.value_addr)->val);
      break;
    case LUA_STRING:
      sprintf(result, "%s\n", (char *)(((moon_string_value *) buf_ref.value_addr)->string_addr));
      break;

    default:
      sprintf(result, "other type : %d\n", ((moon_value *) buf_ref.value_addr)->type);
      break;
  };

  if (buf_ref.is_copy == TRUE) delete_value((moon_value *) buf_ref.value_addr);
}

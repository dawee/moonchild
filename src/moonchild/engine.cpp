#include "moonchild.h"

#include <stdio.h>
#include <stdlib.h>


static void progmem_cpy(void * dest, PGMEM_ADDRESS src, uint16_t size, uint16_t offset = 0) {
  char * cdest = (char *)dest;

  for (uint16_t index = 0; index < size; ++index) {
    cdest[index] = progmem_read(src, index + offset);
  }
}

static moon_closure * create_closure() {
  return (moon_closure *) malloc(sizeof(moon_closure));
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
  for (uint16_t index = 0; index < closure->prototype->max_stack_size; ++index) {
    create_register(closure, index);
  }
}

static void init_prototype(moon_prototype * prototype, PGMEM_ADDRESS prototype_addr) {
  progmem_cpy(prototype, prototype_addr, sizeof(moon_prototype));
}

static void init_closure(moon_closure * closure, PGMEM_ADDRESS prototype_addr) {
  closure->prototype = create_prototype();
  init_prototype(closure->prototype, prototype_addr);
  init_registers(closure);
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

static void create_op_bufs(moon_reference * buf1_ref, moon_reference * buf2_ref, moon_instruction * instruction, moon_closure * closure) {
  moon_reference const_ref;

  if ((instruction->flag & OPCK_FLAG) == OPCK_FLAG) {
    create_value_copy(buf1_ref, closure->registers[instruction->b]);
    read_constant_reference(&const_ref, closure->prototype, instruction->c);
    create_value_copy(buf2_ref, &const_ref);
  } else if ((instruction->flag & OPBK_FLAG) == OPBK_FLAG) {
    read_constant_reference(&const_ref, closure->prototype, instruction->b);
    create_value_copy(buf1_ref, &const_ref);
    create_value_copy(buf2_ref, closure->registers[instruction->c]);
  } else {
    create_value_copy(buf1_ref, closure->registers[instruction->b]);
    create_value_copy(buf2_ref, closure->registers[instruction->c]);
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
  moon_reference const_ref;

  read_constant_reference(&const_ref, closure->prototype, instruction->b);
  copy_reference(closure->registers[instruction->a], &const_ref);
}

static void op_add(moon_instruction * instruction, moon_closure * closure) {
  moon_reference buf1_ref;
  moon_reference buf2_ref;

  create_op_bufs(&buf1_ref, &buf2_ref, instruction, closure);
  create_op_add_result(closure->registers[instruction->a], (moon_value *) buf1_ref.value_addr, (moon_value *) buf2_ref.value_addr);
  closure->registers[instruction->a]->is_progmem = FALSE;

  if (buf1_ref.is_copy == TRUE) delete_value((moon_value *) buf1_ref.value_addr);
  if (buf2_ref.is_copy == TRUE) delete_value((moon_value *) buf2_ref.value_addr);
}

static void op_sub(moon_instruction * instruction, moon_closure * closure) {
  moon_reference buf1_ref;
  moon_reference buf2_ref;

  create_op_bufs(&buf1_ref, &buf2_ref, instruction, closure);
  create_op_sub_result(closure->registers[instruction->a], (moon_value *) buf1_ref.value_addr, (moon_value *) buf2_ref.value_addr);
  closure->registers[instruction->a]->is_progmem = FALSE;

  if (buf1_ref.is_copy == TRUE) delete_value((moon_value *) buf1_ref.value_addr);
  if (buf2_ref.is_copy == TRUE) delete_value((moon_value *) buf2_ref.value_addr);
}

static void op_mul(moon_instruction * instruction, moon_closure * closure) {
  moon_reference buf1_ref;
  moon_reference buf2_ref;

  create_op_bufs(&buf1_ref, &buf2_ref, instruction, closure);
  create_op_mul_result(closure->registers[instruction->a], (moon_value *) buf1_ref.value_addr, (moon_value *) buf2_ref.value_addr);
  closure->registers[instruction->a]->is_progmem = FALSE;

  if (buf1_ref.is_copy == TRUE) delete_value((moon_value *) buf1_ref.value_addr);
  if (buf2_ref.is_copy == TRUE) delete_value((moon_value *) buf2_ref.value_addr);
}

static void op_div(moon_instruction * instruction, moon_closure * closure) {
  moon_reference buf1_ref;
  moon_reference buf2_ref;

  create_op_bufs(&buf1_ref, &buf2_ref, instruction, closure);
  create_op_div_result(closure->registers[instruction->a], (moon_value *) buf1_ref.value_addr, (moon_value *) buf2_ref.value_addr);
  closure->registers[instruction->a]->is_progmem = FALSE;

  if (buf1_ref.is_copy == TRUE) delete_value((moon_value *) buf1_ref.value_addr);
  if (buf2_ref.is_copy == TRUE) delete_value((moon_value *) buf2_ref.value_addr);
}

static void op_move(moon_instruction * instruction, moon_closure * closure) {
  copy_reference(closure->registers[instruction->a], closure->registers[instruction->b]);

  if (! closure->registers[instruction->a]->is_progmem) {
    ((moon_value *) closure->registers[instruction->a])->nodes++;
  }
}

static void op_concat(moon_instruction * instruction, moon_closure * closure) {
  moon_reference buf1_ref;
  moon_reference buf2_ref;

  create_op_bufs(&buf1_ref, &buf2_ref, instruction, closure);
  create_op_concat_result(closure->registers[instruction->a], (moon_value *) buf1_ref.value_addr, (moon_value *) buf2_ref.value_addr);
  closure->registers[instruction->a]->is_progmem = FALSE;

  if (buf1_ref.is_copy == TRUE) delete_value((moon_value *) buf1_ref.value_addr);
  if (buf2_ref.is_copy == TRUE) delete_value((moon_value *) buf2_ref.value_addr);
}

static void run_instruction(moon_closure * closure, uint16_t index) {
  moon_instruction instruction;

  read_instruction(&instruction, closure->prototype, index);

  switch(instruction.opcode) {
    case OPCODE_LOADNIL:
      op_loadnil(&instruction, closure);
      break;
    case OPCODE_LOADBOOL:
      op_loadbool(&instruction, closure);
      break;
    case OPCODE_LOADK:
      op_loadk(&instruction, closure);
      break;
    case OPCODE_ADD:
      op_add(&instruction, closure);
      break;
    case OPCODE_SUB:
      op_sub(&instruction, closure);
      break;
    case OPCODE_MUL:
      op_mul(&instruction, closure);
      break;
    case OPCODE_DIV:
      op_div(&instruction, closure);
      break;
    case OPCODE_MOVE:
      op_move(&instruction, closure);
      break;
    case OPCODE_CONCAT:
      op_concat(&instruction, closure);
      break;
    case OPCODE_RETURN:
      // @TODO : manage return value
      break;

    default:
      moon_debug("unknown upcode : %d\n", instruction.opcode);
      break;
  };

}


void moon_run(PGMEM_ADDRESS prototype_addr, char * result) {
  moon_closure * closure = create_closure();
  moon_reference buf_ref;

  init_closure(closure, prototype_addr);

  for (uint16_t index = 0; index < closure->prototype->instructions_count; ++index) {
    run_instruction(closure, index);
  }

  create_value_copy(&buf_ref, closure->registers[0]);

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

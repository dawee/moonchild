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

  free(value);
  value = NULL;
}

static void create_progmem_value_copy(moon_reference * dest, moon_reference * src) {
  uint8_t type = progmem_read(src->value_addr, 0);

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

    default:
      moon_debug("error: could not copy unknown type : %d", type);
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

static void init_prototype(moon_prototype * prototype, PGMEM_ADDRESS prototype_addr) {
  progmem_cpy(prototype, prototype_addr, sizeof(moon_prototype));
}

static void init_closure(moon_closure * closure, PGMEM_ADDRESS prototype_addr) {
  closure->prototype = create_prototype();
  init_prototype(closure->prototype, prototype_addr);
  init_registers(closure);
}

static BOOL check_arithmetic_values(moon_value * valueA, moon_value * valueB) {
  if (valueA->type == LUA_NIL || valueB->type == LUA_NIL) {
    moon_debug("error: try to perform arithmetic on a nil value");
    return FALSE;
  }

  if (valueA->type == LUA_TRUE || valueB->type == LUA_TRUE || valueA->type == LUA_FALSE || valueB->type == LUA_FALSE) {
    moon_debug("error: try to perform arithmetic on a boolean value");
    return FALSE;
  }

  if (valueA->type == LUA_STRING || valueB->type == LUA_STRING) {
    moon_debug("error: try to perform arithmetic on a boolean value");
    return FALSE;
  }

  return TRUE;
}

static void create_result_value(moon_reference * result, moon_value * valueA, moon_value * valueB, CTYPE_LUA_INT int_val, CTYPE_LUA_NUMBER number_val) {
  if (valueA->type == LUA_NUMBER || valueB->type == LUA_NUMBER) {
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

static void create_op_add_result(moon_reference * result, moon_value * valueA, moon_value * valueB) {
  CTYPE_LUA_INT int_val;
  CTYPE_LUA_NUMBER number_val;

  if (check_arithmetic_values(valueA, valueB) == FALSE) return;

  if (valueA->type == LUA_INT && valueB->type == LUA_INT) {
    int_val = ((moon_int_value *) valueA)->val + ((moon_int_value *) valueB)->val;
  } else if (valueA->type == LUA_INT && valueB->type == LUA_NUMBER) {
    number_val = ((moon_int_value *) valueA)->val + ((moon_number_value *) valueB)->val;
  } else if (valueA->type == LUA_NUMBER && valueB->type == LUA_INT) {
    number_val = ((moon_number_value *) valueA)->val + ((moon_int_value *) valueB)->val;
  } else if (valueA->type == LUA_NUMBER && valueB->type == LUA_NUMBER) {
    number_val = ((moon_number_value *) valueA)->val + ((moon_number_value *) valueB)->val;
  }

  create_result_value(result, valueA, valueB, int_val, number_val);
}

static void create_op_sub_result(moon_reference * result, moon_value * valueA, moon_value * valueB) {
  CTYPE_LUA_INT int_val;
  CTYPE_LUA_NUMBER number_val;

  if (check_arithmetic_values(valueA, valueB) == FALSE) return;

  if (valueA->type == LUA_INT && valueB->type == LUA_INT) {
    int_val = ((moon_int_value *) valueA)->val - ((moon_int_value *) valueB)->val;
  } else if (valueA->type == LUA_INT && valueB->type == LUA_NUMBER) {
    number_val = ((moon_int_value *) valueA)->val - ((moon_number_value *) valueB)->val;
  } else if (valueA->type == LUA_NUMBER && valueB->type == LUA_INT) {
    number_val = ((moon_number_value *) valueA)->val - ((moon_int_value *) valueB)->val;
  } else if (valueA->type == LUA_NUMBER && valueB->type == LUA_NUMBER) {
    number_val = ((moon_number_value *) valueA)->val - ((moon_number_value *) valueB)->val;
  }

  create_result_value(result, valueA, valueB, int_val, number_val);
}

static void create_op_div_result(moon_reference * result, moon_value * valueA, moon_value * valueB) {
  CTYPE_LUA_INT int_val;
  CTYPE_LUA_NUMBER number_val;

  if (check_arithmetic_values(valueA, valueB) == FALSE) return;

  if (valueA->type == LUA_INT && valueB->type == LUA_INT) {
    int_val = ((moon_int_value *) valueA)->val / ((moon_int_value *) valueB)->val;
  } else if (valueA->type == LUA_INT && valueB->type == LUA_NUMBER) {
    number_val = ((moon_int_value *) valueA)->val / ((moon_number_value *) valueB)->val;
  } else if (valueA->type == LUA_NUMBER && valueB->type == LUA_INT) {
    number_val = ((moon_number_value *) valueA)->val / ((moon_int_value *) valueB)->val;
  } else if (valueA->type == LUA_NUMBER && valueB->type == LUA_NUMBER) {
    number_val = ((moon_number_value *) valueA)->val / ((moon_number_value *) valueB)->val;
  }

  create_result_value(result, valueA, valueB, int_val, number_val);
}

static void create_op_mul_result(moon_reference * result, moon_value * valueA, moon_value * valueB) {
  CTYPE_LUA_INT int_val;
  CTYPE_LUA_NUMBER number_val;

  if (check_arithmetic_values(valueA, valueB) == FALSE) return;

  if (valueA->type == LUA_INT && valueB->type == LUA_INT) {
    int_val = ((moon_int_value *) valueA)->val * ((moon_int_value *) valueB)->val;
  } else if (valueA->type == LUA_INT && valueB->type == LUA_NUMBER) {
    number_val = ((moon_int_value *) valueA)->val * ((moon_number_value *) valueB)->val;
  } else if (valueA->type == LUA_NUMBER && valueB->type == LUA_INT) {
    number_val = ((moon_number_value *) valueA)->val * ((moon_int_value *) valueB)->val;
  } else if (valueA->type == LUA_NUMBER && valueB->type == LUA_NUMBER) {
    number_val = ((moon_number_value *) valueA)->val * ((moon_number_value *) valueB)->val;
  }

  create_result_value(result, valueA, valueB, int_val, number_val);
}


static void prepare_op_bufs(moon_reference * buf1_ref, moon_reference * buf2_ref, moon_instruction * instruction, moon_closure * closure) {
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

static void op_loadk(moon_instruction * instruction, moon_closure * closure) {
  moon_reference const_ref;

  create_register(closure, instruction->a);
  read_constant_reference(&const_ref, closure->prototype, instruction->b);
  copy_reference(closure->registers[instruction->a], &const_ref);
}

static void op_add(moon_instruction * instruction, moon_closure * closure) {
  moon_reference buf1_ref;
  moon_reference buf2_ref;

  create_register(closure, instruction->a);
  prepare_op_bufs(&buf1_ref, &buf2_ref, instruction, closure);
  create_op_add_result(closure->registers[instruction->a], (moon_value *) buf1_ref.value_addr, (moon_value *) buf2_ref.value_addr);
  closure->registers[instruction->a]->is_progmem = FALSE;

  if (buf1_ref.is_copy == TRUE) delete_value((moon_value *) buf1_ref.value_addr);
  if (buf2_ref.is_copy == TRUE) delete_value((moon_value *) buf2_ref.value_addr);
}

static void op_sub(moon_instruction * instruction, moon_closure * closure) {
  moon_reference buf1_ref;
  moon_reference buf2_ref;

  create_register(closure, instruction->a);
  prepare_op_bufs(&buf1_ref, &buf2_ref, instruction, closure);
  create_op_sub_result(closure->registers[instruction->a], (moon_value *) buf1_ref.value_addr, (moon_value *) buf2_ref.value_addr);
  closure->registers[instruction->a]->is_progmem = FALSE;

  if (buf1_ref.is_copy == TRUE) delete_value((moon_value *) buf1_ref.value_addr);
  if (buf2_ref.is_copy == TRUE) delete_value((moon_value *) buf2_ref.value_addr);
}

static void op_mul(moon_instruction * instruction, moon_closure * closure) {
  moon_reference buf1_ref;
  moon_reference buf2_ref;

  create_register(closure, instruction->a);
  prepare_op_bufs(&buf1_ref, &buf2_ref, instruction, closure);
  create_op_mul_result(closure->registers[instruction->a], (moon_value *) buf1_ref.value_addr, (moon_value *) buf2_ref.value_addr);
  closure->registers[instruction->a]->is_progmem = FALSE;

  if (buf1_ref.is_copy == TRUE) delete_value((moon_value *) buf1_ref.value_addr);
  if (buf2_ref.is_copy == TRUE) delete_value((moon_value *) buf2_ref.value_addr);
}

static void op_div(moon_instruction * instruction, moon_closure * closure) {
  moon_reference buf1_ref;
  moon_reference buf2_ref;

  create_register(closure, instruction->a);
  prepare_op_bufs(&buf1_ref, &buf2_ref, instruction, closure);
  create_op_div_result(closure->registers[instruction->a], (moon_value *) buf1_ref.value_addr, (moon_value *) buf2_ref.value_addr);
  closure->registers[instruction->a]->is_progmem = FALSE;

  if (buf1_ref.is_copy == TRUE) delete_value((moon_value *) buf1_ref.value_addr);
  if (buf2_ref.is_copy == TRUE) delete_value((moon_value *) buf2_ref.value_addr);
}

static void run_instruction(moon_closure * closure, uint16_t index) {
  moon_instruction instruction;

  read_instruction(&instruction, closure->prototype, index);

  switch(instruction.opcode) {
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
    default:
      break;
  };

}


void moon_run(PGMEM_ADDRESS prototype_addr, char * result) {
  moon_closure * closure = create_closure();
  moon_int_value int_value;
  moon_number_value number_value;

  init_closure(closure, prototype_addr);

  for (uint16_t index = 0; index < closure->prototype->instructions_count; ++index) {
    run_instruction(closure, index);
  }

  uint8_t type = (uint8_t) progmem_read(closure->registers[0]->value_addr, 0);

  if (type == LUA_INT) {
    progmem_cpy(&int_value, closure->registers[0]->value_addr, sizeof(moon_int_value));
    sprintf(result, "%d\n", int_value.val);
  } else if (type == LUA_NUMBER) {    
    progmem_cpy(&number_value, closure->registers[0]->value_addr, sizeof(moon_number_value));
    sprintf(result, "~%d\n", (int)number_value.val);
  } else {
    sprintf(result, "other type : %d\n", type);
  }

}

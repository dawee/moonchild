module.exports = proto => `
#include "moonchild.h"

const char ${proto.baseName}_func_name[] PROGMEM = "${proto.funcName}";

const moon_instruction ${proto.baseName}_instructions[] PROGMEM = {
${proto.instructions.map(
  instruction => `  {.opcode = ${instruction.opcodeName}, .a = ${instruction.a}, .b = ${instruction.b}, .c = ${instruction.c}},`
).join('\n')}
};

${proto.constants.map(
  (constant, i) => `const ${constant.ctype} ${proto.baseName}_constants_value_${i} PROGMEM = ${constant.data};`
).join('\n')}

const moon_value ${proto.baseName}_constants[] PROGMEM = {
${proto.constants.map(
  (constant, i) => `  {.type = ${constant.type}, .data_addr = (PGMEM_ADDRESS) &${proto.baseName}_constants_value_${i}},`
).join('\n')}
};

const moon_prototype ${proto.baseName}_prototype PROGMEM = {
  .func_name_size = ${proto.funcName.length},
  .func_name_addr = (PGMEM_ADDRESS) ${proto.baseName}_func_name,
  .instructions_count = ${proto.instructions.length},
  .instructions_addr = (PGMEM_ADDRESS) ${proto.baseName}_instructions,
  .constants_count = ${proto.constants.length},
  .constants_addr = (PGMEM_ADDRESS) ${proto.baseName}_constants,
};

void moon_run_generated() {
  moon_arch_run((PGMEM_ADDRESS) &${proto.baseName}_prototype);
}
`;

module.exports = proto => `
#include "moonchild.h"

const char ${proto.baseName}_func_name[] PROGMEM = "${proto.funcName}";

const moon_instruction ${proto.baseName}_instructions[] PROGMEM = {
${proto.instructions.map(
  instruction => `  {.opcode = ${instruction.opcodeName}, .a = ${instruction.a}, .b = ${instruction.b}, .c = ${instruction.c}, .flag = ${instruction.flag}},`
).join('\n')}
};

${proto.constants.map((constant, i) => `
const ${constant.typeStruct} ${proto.baseName}_constants_value_${i} PROGMEM = {
  .type = ${constant.type}, .nodes = 1, .val = ${constant.val},
};
`).join('\n')}

const moon_reference ${proto.baseName}_constants[] PROGMEM = {
${proto.constants.map(
  (constant, i) => `  {.progmem = TRUE, .value_addr = (SRAM_ADDRESS) &${proto.baseName}_constants_value_${i}},`
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

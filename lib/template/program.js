const dumpValConst = (proto, constant, i) => `
const ${constant.typeStruct} ${proto.baseName}_constants_value_${i} PROGMEM = {
  .type = ${constant.type}, .nodes = 1, .val = ${constant.val},
};
`;

const dumpStringConst = (proto, constant, i) => `
const char ${proto.baseName}_constants_value_${i}_string[] PROGMEM = "${constant.val}";
const ${constant.typeStruct} ${proto.baseName}_constants_value_${i} PROGMEM = {
  .type = ${constant.type}, .nodes = 1, .string_addr = (SRAM_ADDRESS) ${proto.baseName}_constants_value_${i}_string, .length = ${constant.val.length}
};
`;

const dumpConst = (proto, constant, i) => {
  if (constant.typeStruct === 'moon_string_value') return dumpStringConst(proto, constant, i);

  return dumpValConst(proto, constant, i);
};

const dumpPrototypeReferences = proto => `
${proto.protos.map(child => dumpPrototypeReferences(child)).join('\n')}

const moon_prototype ${proto.baseName}_prototypes[] PROGMEM = {
${proto.protos.map(child => `  {${dumpPrototypeContent(child)}},`).join('\n')}
};


const moon_instruction ${proto.baseName}_instructions[] PROGMEM = {
${proto.instructions.map(
  instruction => `  {.opcode = ${instruction.opcodeName}, .a = ${instruction.a}, .b = ${instruction.b}, .c = ${instruction.c}, .flag = ${instruction.flag}},`
).join('\n')}
};

${proto.constants.map((constant, i) => dumpConst(proto, constant, i)).join('\n')}

const moon_reference ${proto.baseName}_constants[] PROGMEM = {
${proto.constants.map(
  (constant, i) => `  {.is_progmem = TRUE, .is_copy = FALSE, .value_addr = (SRAM_ADDRESS) &${proto.baseName}_constants_value_${i}},`
).join('\n')}
};
`

const dumpPrototypeContent = proto => `
  .num_params = ${proto.numParams},
  .is_varargs = ${proto.isVarargs},
  .max_stack_size = ${proto.maxStackSize},
  .instructions_count = ${proto.instructions.length},
  .instructions_addr = (PGMEM_ADDRESS) ${proto.baseName}_instructions,
  .constants_count = ${proto.constants.length},
  .constants_addr = (PGMEM_ADDRESS) ${proto.baseName}_constants,
  .prototypes_count = ${proto.protos.length},
  .prototypes_addr = (PGMEM_ADDRESS) ${proto.baseName}_prototypes,
`;

const dumpPrototype = proto => `
${dumpPrototypeReferences(proto)}
const moon_prototype ${proto.baseName}_prototype PROGMEM = {${dumpPrototypeContent(proto)}};
`

module.exports = proto => `
#include "moonchild.h"
${dumpPrototype(proto)}
void moon_run_generated() {
  moon_arch_run((PGMEM_ADDRESS) &${proto.baseName}_prototype);
}
`;

const dumpInstructionParams = instruction => ['a', 'b', 'c', 'bx', 'sbx'].map(param => {
  if (! (param in instruction)) return null;

  return `${param} = ${instruction[param]}`
}).filter(expression => expression !== null).join(', ');

const dumpInstructionComment = instruction => (
  `opcode = ${instruction.opcodeName} (${dumpInstructionParams(instruction)}) {${instruction.flagNames.join('|')}}`
);

const dumpValConst = (proto, constant, i) => `
static const ${constant.typeStruct} ${proto.baseName}_constants_value_${i} PROGMEM = {
  .type = ${constant.type}, .nodes = 1, .val = ${constant.val},
};
`;

const dumpStringConst = (proto, constant, i) => `
static const char ${proto.baseName}_constants_value_${i}_string[] PROGMEM = "${constant.val}";
static const ${constant.typeStruct} ${proto.baseName}_constants_value_${i} PROGMEM = {
  .type = ${constant.type}, .nodes = 1, .string_addr = (SRAM_ADDRESS) ${proto.baseName}_constants_value_${i}_string, .length = ${constant.val.length}
};
`;

const dumpConst = (proto, constant, i) => {
  if (constant.isStatic) return;
  if (constant.typeStruct === 'moon_string_value') return dumpStringConst(proto, constant, i);

  return dumpValConst(proto, constant, i);
};

const dumpConstArrayElement = (proto, constant, i) => {
  return `{.is_progmem = TRUE, .is_copy = FALSE, .value_addr = (SRAM_ADDRESS) &${constant.isStatic ? constant.staticAddr : `${proto.baseName}_constants_value_${i}`}},`;
}

const dumpConstArray = proto => (
`static const moon_reference ${proto.baseName}_constants[] PROGMEM = {
${proto.constants.map((constant, i) => `  ${dumpConstArrayElement(proto, constant, i)}`).join('\n')}
};`
)

const dumpUpvaluesArray = proto => (
`static const moon_upvalue ${proto.baseName}_upvalues[] PROGMEM = {
${proto.upvalues.map((upvalue, i) => `  {.in_stack = ${upvalue.inStack}, .idx = ${upvalue.idx}},`).join('\n')}
};`
)

const dumpPrototypesArray = proto => `
static const moon_prototype ${proto.baseName}_prototypes[] PROGMEM = {
${proto.protos.map(child => `  {${dumpPrototypeContent(child)}},`).join('\n')}
};
`;

const dumpPrototypeReferences = proto => `
${proto.protos.map(child => dumpPrototypeReferences(child)).join('\n')}
${proto.protos.length > 0 ? dumpPrototypesArray(proto) : ''}
static const moon_instruction ${proto.baseName}_instructions[] PROGMEM = {
${proto.instructions.map(
  instruction => `  {.raw = ${instruction.raw}}, // ${dumpInstructionComment(instruction)}`
).join('\n')}
};

${proto.constants.map((constant, i) => dumpConst(proto, constant, i)).join('\n')}

${proto.constants.length > 0 ? dumpConstArray(proto) : ''}

${proto.upvalues.length > 0 ? dumpUpvaluesArray(proto) : ''}
`

const dumpPrototypeContent = proto => `
  .num_params = ${proto.numParams},
  .is_varargs = ${proto.isVarargs},
  .max_stack_size = ${proto.maxStackSize},
  .instructions_count = ${proto.instructions.length},
  .instructions_addr = (PGM_VOID_P) ${proto.baseName}_instructions,
  .constants_count = ${proto.constants.length},
  .constants_addr = (PGM_VOID_P) ${proto.constants.length > 0 ? `${proto.baseName}_constants` : 'NULL'},
  .prototypes_count = ${proto.protos.length},
  .prototypes_addr = (PGM_VOID_P) ${proto.protos.length > 0 ? `${proto.baseName}_prototypes` : 'NULL'},
  .upvalues_count = ${proto.upvalues.length},
  .upvalues_addr = (PGM_VOID_P) ${proto.upvalues.length > 0 ? `${proto.baseName}_upvalues` : 'NULL'},
`;

const dumpPrototype = proto => `
${dumpPrototypeReferences(proto)}
static const moon_prototype ${proto.baseName}_prototype PROGMEM = {${dumpPrototypeContent(proto)}};
`

const dumpSourceContent = content => content.split('\n').map(
  line => ` * ${line}`
).join('\n');

module.exports = project => `
/*
 * [SOURCE: ${project.fileName}]
 *
${dumpSourceContent(project.content)}
 *
 */

#include "moonchild.h"
${dumpPrototype(project.proto)}
void moon_run_generated() {
  moon_arch_run((PGM_VOID_P) &${project.proto.baseName}_prototype);
}
`;

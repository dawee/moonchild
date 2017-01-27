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
};
`
)

const dumpUpvaluesArray = proto => (
`static const moon_upvalue ${proto.baseName}_upvalues[] PROGMEM = {
${proto.upvalues.map((upvalue, i) => `  {.in_stack = ${upvalue.inStack}, .idx = ${upvalue.idx}},`).join('\n')}
};
`
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
 *                             :           :
 *                            t#,         t#,     L.
 *                           ;##W.       ;##W.    EW:        ,ft
 *             ..       :   :#L:WE      :#L:WE    E##;       t#E
 *            ,W,     .Et  .KG  ,#D    .KG  ,#D   E###t      t#E
 *           t##,    ,W#t  EE    ;#f   EE    ;#f  E#fE#f     t#E
 *          L###,   j###t f#.     t#i f#.     t#i E#t D#G    t#E
 *        .E#j##,  G#fE#t :#G     GK  :#G     GK  E#t  f#E.  t#E
 *       ;WW; ##,:K#i E#t  ;#L   LW.   ;#L   LW.  E#t   t#K: t#E
 *      j#E.  ##f#W,  E#t   t#f f#:     t#f f#:   E#t    ;#W,t#E
 *    .D#L    ###K:   E#t    f#D#;       f#D#;    E#t     :K#D#E
 *   :K#t     ##D.    E#t     G#t         G#t     E#t      .E##E
 *   ...      #G      ..       t           t      ..         G#E
 *
 *
 *                                                                    ED.
 *                                  .,                                E#Wi
 *                                 ,Wt .    .      t              i   E###G.
 *                                i#D. Di   Dt     Ej            LE   E#fD#W;
 *                               f#f   E#i  E#i    E#,          L#E   E#t t##L
 *                             .D#i    E#t  E#t    E#t         G#W.   E#t  .E#K,
 *                            :KW,     E#t  E#t    E#t        D#K.    E#t    j##f
 *                            t#f      E########f. E#t       E#K.     E#t    :E#K:
 *    Lua 5.3.3                ;#G     E#j..K#j... E#t     .E#E.      E#t   t##L
 *                              :KE.   E#t  E#t    E#t    .K#E        E#t .D#W;
 *                               .DW:  E#t  E#t    E#t   .K#D         E#tiW#G.
 *    for Arduino boards           L#, f#t  f#t    E#t  .W#G          E#K##i
 *                                  jt  ii   ii    E#t :W##########Wt E##D.
 *                                                 ,;. :,,,,,,,,,,,,,.E#t
 *                                                                    L:
 * [SOURCE: ${project.fileName}]
 *
${dumpSourceContent(project.content)}
 *
 */

#include "moonchild.h"
${dumpPrototype(project.proto)}

moon_closure * create_${project.proto.baseName}_closure() {
  return moon_create_closure((PGM_VOID_P) &${project.proto.baseName}_prototype, 0, NULL);
}

#ifndef MOONCHILD_PROJECT_MAIN

int main() {
  moon_closure * closure;
  moon_init();
  closure = create_${project.proto.baseName}_closure();
  moon_run_closure(closure, NULL);
  return 0;
}

#endif
`;

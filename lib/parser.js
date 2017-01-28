const bignum = require('bignum');
const reverseBuf = require('buffer-reverse');

const INT_SIZE = 4;
const LUA_INTEGER_SIZE = 8;
const LUA_NUMBER_SIZE = 8;
const OPBK_FLAG = 0b00000001;
const OPCK_FLAG = 0b00000010;

const OPCODES_NAMES = [
  'OPCODE_MOVE',
  'OPCODE_LOADK',
  'OPCODE_LOADKX',
  'OPCODE_LOADBOOL',
  'OPCODE_LOADNIL',
  'OPCODE_GETUPVAL',
  'OPCODE_GETTABUP',
  'OPCODE_GETTABLE',
  'OPCODE_SETTABUP',
  'OPCODE_SETUPVAL',
  'OPCODE_SETTABLE',
  'OPCODE_NEWTABLE',
  'OPCODE_SELF',
  'OPCODE_ADD',
  'OPCODE_SUB',
  'OPCODE_MUL',
  'OPCODE_MOD',
  'OPCODE_POW',
  'OPCODE_DIV',
  'OPCODE_IDIV',
  'OPCODE_BAND',
  'OPCODE_BOR',
  'OPCODE_BXOR',
  'OPCODE_SHL',
  'OPCODE_SHR',
  'OPCODE_UNM',
  'OPCODE_BNOT',
  'OPCODE_NOT',
  'OPCODE_LEN',
  'OPCODE_CONCAT',
  'OPCODE_JMP',
  'OPCODE_EQ',
  'OPCODE_LT',
  'OPCODE_LE',
  'OPCODE_TEST',
  'OPCODE_TESTSET',
  'OPCODE_CALL',
  'OPCODE_TAILCALL',
  'OPCODE_RETURN',
  'OPCODE_FORLOOP',
  'OPCODE_FORPREP',
  'OPCODE_TFORCALL',
  'OPCODE_TFORLOOP',
  'OPCODE_SETLIST',
  'OPCODE_CLOSURE',
  'OPCODE_VARARG',
  'OPCODE_EXTRAAR',
];

const OPCODES = {};

for (const [val, name] of OPCODES_NAMES.entries()) {
  OPCODES[name] = val;
}

class LuacParser {

  constructor(project, buf) {
    this.buf = buf;
    this.cursor = 0;
    this.project = project;

    this.iABx = [
      OPCODES.OPCODE_LOADK,
      OPCODES.OPCODE_LOADKX,
      OPCODES.OPCODE_CLOSURE,
    ];

    this.iAsBx = [
      OPCODES.OPCODE_JMP,
      OPCODES.OPCODE_FORLOOP,
      OPCODES.OPCODE_FORPREP,
      OPCODES.OPCODE_TFORLOOP,
    ];

    this.constants = {
      0: () => ({
        staticAddr: 'MOON_NIL_VALUE',
        isStatic: true,
      }),
      1: () => this.readBool(),
      3: () => ({
        typeStruct: 'moon_number_value',
        type: 'MOON_TYPE_NUMBER',
        val: this.readLuaNumber(),
      }),
      4: () => ({
        typeStruct: 'moon_string_value',
        type: 'MOON_TYPE_STRING',
        val: this.readString(),
      }),
      19: () => ({
        typeStruct: 'moon_int_value',
        type: 'MOON_TYPE_INT',
        val: this.readLuaInteger(),
      }),
    };

  }

  readBool() {
    const val = this.readByte();

    return {
      staticAddr: val === 1 ? 'MOON_TRUE_VALUE' : 'MOON_FALSE_VALUE',
      isStatic: true,
    };
  }

  readByte() {
    const byte = this.buf[this.cursor];

    this.cursor++;
    return byte;
  }

  skipBytes(count) {
    this.cursor += count;
  }

  readFuncName() {
    const funcName = this.readString();
    const baseName = funcName.replace(/\W+/g, "_");

    return {funcName, baseName};
  }

  readBuf(size) {
    const buf = this.buf.slice(this.cursor, this.cursor + size);

    this.cursor += size;
    return buf;
  }

  readString() {
    const length = this.readByte() - 1;

    if (length === 0) this.readByte();

    return length > 0 ? this.readBuf(length).toString() : '';
  }

  readInt() {
    return this.readBuf(INT_SIZE).readInt32LE();
  }

  readLuaNumber() {
    return this.readBuf(LUA_NUMBER_SIZE).readDoubleLE();
  }

  readLuaInteger() {
    let memo = bignum(0);
    const buf = this.readBuf(LUA_INTEGER_SIZE);

    for (const [index, byte] of buf.entries()) {
      memo = memo.add(bignum.pow(256, index).mul(byte));

      if (index == buf.length - 1 && (byte & 0x80 === 0x80)) {
        memo = memo.sub(bignum.pow(256, buf.length));
      }
    }

    return memo.toNumber();
  }

  readInstructions() {
    const instructionsCount = this.readInt();
    const instructions = [];

    for (let index = 0; index < instructionsCount; ++index) {
      const instruction = {flag: 0};
      const rawBuf = this.readBuf(INT_SIZE);
      const raw = rawBuf.readInt32LE();

      instruction.raw = `0x${reverseBuf(rawBuf).toString('hex')}`;
      instruction.opcode = raw & 0x3F;
      instruction.opcodeName = OPCODES_NAMES[instruction.opcode];
      instruction.a = (raw & 0x3FC0) >> 6;
      instruction.flagNames = [];

      if (this.iABx.indexOf(instruction.opcode) >= 0) {
        instruction.bx = (raw & 0xFFFFC000) >> 14;
      } else if (this.iAsBx.indexOf(instruction.opcode) >= 0) {
        instruction.sbx =  Math.abs((raw & 0xFFFFC000) >> 14) - 0x1FFFF;
      } else {
        instruction.b = (raw & 0xFF800000) >> 23;
        instruction.c = (raw & 0x7FC000) >> 14;

        if ((instruction.b & 0x100) === 0x100) {
          instruction.b &= 0xFF;
          instruction.flag |= OPBK_FLAG;
          instruction.flagNames.push('OPBK_FLAG')
        }

        if ((instruction.c & 0x100) === 0x100) {
          instruction.c &= 0xFF;
          instruction.flag |= OPCK_FLAG;
          instruction.flagNames.push('OPCK_FLAG')
        }
      }

      instructions.push(instruction);
    }

    return instructions;
  }

  readConstants() {
    const constantsCount = this.readInt();
    const constants = [];

    for (let index = 0; index < constantsCount; ++index) {
      const ctype = this.readByte();

      if (! (ctype in this.constants)) throw {message: `Unknown constant type : "${ctype}"`};

      constants.push(this.constants[ctype]());
    }

    return constants;
  }

  readUpvalues() {
    const upvaluesCount = this.readInt();
    const upvalues = [];

    for (let index = 0; index < upvaluesCount; ++index) {
      const inStack = this.readByte();
      const idx = this.readByte();

      upvalues.push({inStack, idx});
    }

    return upvalues;
  }

  readSizeUpvalues() {
    const sizeUpvaluesCount = this.readInt();
    const sizeUpvalues = [];

    for (let index = 0; index < sizeUpvaluesCount; ++index) {
      sizeUpvalues.push(this.readString());
    }

    return sizeUpvalues;
  }

  readSourceLinePositions() {
    const sourceLinePositionsCount = this.readInt();
    const sourceLinePositions = [];

    for (let index = 0; index < sourceLinePositionsCount; ++index) {
      sourceLinePositions.push(this.readInt());
    }

    return sourceLinePositions;
  }

  readLocalVars() {
    const localVarsCount = this.readInt();
    const localVars = [];

    for (let index = 0; index < localVarsCount; ++index) {
      const name = this.readString();
      const startpc = this.readInt();
      const endpc = this.readInt();

      localVars.push({name, startpc, endpc});
    }

    return localVars;
  }

  readPrototypes(parent) {
    const protos = [];
    const protosCount = this.readInt();

    if (protosCount === 0) return [];

    for (let index = 0; index < protosCount; ++index) {
      protos.push(this.readPrototype(parent, index));
    }

    return protos;
  }

  readPrototype(parent, index) {
    const proto = {};
    const {funcName} = this.readFuncName();

    if (!parent) {
      Object.assign(proto, {funcName, baseName: this.project.baseName});
    } else {
      proto.funcName = parent.funcName;
      proto.baseName = `${parent.baseName}_${index}`;
    }

    proto.firstLine = this.readInt();
    proto.lastLine = this.readInt();
    proto.numParams = this.readByte();
    proto.isVarargs = this.readByte();
    proto.maxStackSize = this.readByte();
    proto.instructions = this.readInstructions();
    proto.constants = this.readConstants();
    proto.upvalues = this.readUpvalues();
    proto.protos = this.readPrototypes(proto);
    proto.sourceLinePositions = this.readSourceLinePositions();
    proto.localVars = this.readLocalVars();
    proto.sizeupValues = this.readSizeUpvalues();

    return proto;
  }

  parse() {
    this.skipBytes(34);  // header
    return this.readPrototype();
  }

}

module.exports = {LuacParser};

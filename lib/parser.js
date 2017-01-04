const binary = require('./binary');


const INT_SIZE = 4;
const LUA_INTEGER_SIZE = 8;
const LUA_NUMBER_SIZE = 8;


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

  constructor(buf) {
    this.buf = buf;
    this.cursor = 0;
    this.proto = {};
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
    this.proto.funcNameSize = this.readByte() - 1;
    this.proto.funcName = '';

    for (let index = 0; index < this.proto.funcNameSize; ++index) {
      this.proto.funcName += String.fromCharCode(this.readByte());
    }

    this.proto.baseName = this.proto.funcName.replace(/\W+/g, "_");
  }

  readBuf(size) {
    const buf = this.buf.slice(this.cursor, this.cursor + size);
    
    this.cursor += size;
    return buf;
  }

  readInt() {
    return this.readWord(INT_SIZE, 'word32ls');
  }

  readWord(size, method) {
    return binary.parse(this.readBuf(size))[method]('res').vars.res;
  }

  readLuaInteger() {
    return this.readWord(LUA_INTEGER_SIZE, 'word64ls');
  }

  readLuaNumber() {
    return this.readBuf(LUA_NUMBER_SIZE).readDoubleLE();
  }

  readInstructions() {
    this.proto.instructionsCount = this.readInt();
    this.proto.instructions = [];

    for (let index = 0; index < this.proto.instructionsCount; ++index) {
      const instruction = {a: 0, b: 0, c: 0};
      const raw = this.readInt();

      instruction.opcode = raw & 0x3F;
      instruction.opcodeName = OPCODES_NAMES[instruction.opcode];
      instruction.a = (raw & 0x3FC0) >> 6;

      if ([OPCODES.OPCODE_LOADK, OPCODES.OPCODE_LOADKX].indexOf(instruction.opcode) >= 0) {
        instruction.b = (raw & 0xFFFFC000) >> 14;
      } else {
        instruction.b = (raw & 0xFF800000) >> 23;
        instruction.c = (raw & 0x7FC000) >> 14;
      }

      this.proto.instructions.push(instruction);
    }
  }

  readConstants() {
    this.proto.constantsCount = this.readInt();
    this.proto.constants = [];

    for (let index = 0; index < this.proto.constantsCount; ++index) {
      const type = this.readByte();

      this.proto.constants.push({
        type: {
          3: 'LUA_NUMBER',
          4: 'LUA_STRING',
          19: 'LUA_INT',
        }[type],
        ctype: {
          3: 'float',
          4: 'char *',
          19: 'int32_t',
        }[type],
        data:  {
          3: () => this.readLuaNumber(),
          19: () => this.readLuaInteger(),
        }[type]()
      });
    }
  }

  parse() {
    this.proto = {};
    this.skipBytes(33);  // header
    this.skipBytes(1);  // sizeup values

    this.readFuncName();

    this.skipBytes(4);  // first line defined
    this.skipBytes(4);  // last line defined
    this.skipBytes(1);  // numparams
    this.skipBytes(1);  // isvarargs
    this.skipBytes(1);  // maxstacksize

    this.readInstructions();
    this.readConstants();

    return this.proto;
  }

}

module.exports = {LuacParser};
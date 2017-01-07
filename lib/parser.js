const bignum = require('bignum');


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

  constructor(buf) {
    this.buf = buf;
    this.cursor = 0;
    this.proto = {};

    this.constants = {
      3: () => ({
        typeStruct: 'moon_number_value',
        type: 'LUA_NUMBER',
        val: this.readLuaNumber(),
      }),
      4: () => ({
        typeStruct: 'moon_string_value',
        type: 'LUA_STRING',
        val: this.readString(),
      }),
      19: () => ({
        typeStruct: 'moon_int_value',
        type: 'LUA_INT',
        val: this.readLuaInteger(),
      }),
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

  readString() {
    const length = this.readByte() - 1;

    return this.readBuf(length).toString();
  }

  readInt() {
    return this.readBuf(INT_SIZE).readInt32LE();
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

  readLuaNumber() {
    return this.readBuf(LUA_NUMBER_SIZE).readDoubleLE();
  }

  readInstructions() {
    this.proto.instructionsCount = this.readInt();
    this.proto.instructions = [];

    for (let index = 0; index < this.proto.instructionsCount; ++index) {
      const instruction = {a: 0, b: 0, c: 0, flag: 0};
      const raw = this.readInt();

      instruction.opcode = raw & 0x3F;
      instruction.opcodeName = OPCODES_NAMES[instruction.opcode];
      instruction.a = (raw & 0x3FC0) >> 6;

      if ([OPCODES.OPCODE_LOADK, OPCODES.OPCODE_LOADKX].indexOf(instruction.opcode) >= 0) {
        instruction.b = (raw & 0xFFFFC000) >> 14;
      } else {
        instruction.b = (raw & 0xFF800000) >> 23;
        instruction.c = (raw & 0x7FC000) >> 14;

        if ((instruction.b & 0x100) === 0x100) {
          instruction.b &= 0xFF;
          instruction.flag |= OPBK_FLAG;
        }

        if ((instruction.c & 0x100) === 0x100) {
          instruction.c &= 0xFF;
          instruction.flag |= OPCK_FLAG;
        }
      }

      this.proto.instructions.push(instruction);
    }
  }

  readConstants() {
    this.proto.constantsCount = this.readInt();
    this.proto.constants = [];

    for (let index = 0; index < this.proto.constantsCount; ++index) {
      const ctype = this.readByte();

      if (! (ctype in this.constants)) throw {message: `Unknown constant type : "${ctype}"`};

      this.proto.constants.push(this.constants[ctype]());
    }
  }

  parse() {
    this.proto = {};
    this.skipBytes(33);  // header
    this.skipBytes(1);  // sizeup values

    this.readFuncName();

    const protoParser = new PrototypeParser(this.readBuf(this.buf.length - this.cursor));

    Object.assign(this.proto, protoParser.parse());

    return this.proto;
  }

}

class PrototypeParser extends LuacParser {

  parse() {
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
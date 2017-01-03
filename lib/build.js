const os = require('os');
const hat = require('hat');
const path = require('path');
const sketchTemplate = require('./template/sketch');
const programTemplate = require('./template/program');
const actions = require('./actions');


const build = module.exports = filename => {
  const basename = path.basename(filename, '.lua');
  const buildpath = path.join(path.dirname(filename), 'build', basename);
  const sketchname = path.join(buildpath, `${basename}.ino`);
  const programname = path.join(buildpath, `program.cpp`);
  const luacTMP = path.join(os.tmpdir(), hat());
  const libSRC = path.join(__dirname, '..', 'src', 'moonchild');
  const libDST = path.join(buildpath); 


  return actions.mkdirp(buildpath)
    .then(() => actions.luac(filename, luacTMP))
    .catch(err => console.error('Error : Failed to compile Lua script\n', err))
    .then(() => actions.copyTree(libSRC, libDST))
    .catch(err => console.error('Error : Failed copy moonchild lib\n', err))
    .then(() => actions.readLuacBuffer(luacTMP))
    .catch(err => console.error('Error : Failed to read luac output\n', err))
    .then(proto => actions.writeTemplate(programTemplate, programname, proto))
    .catch(err => console.error('Error : Failed to write program\n', err))
    .then(() => actions.writeTemplate(sketchTemplate, sketchname))
    .catch(err => console.error('Error : Failed to write sketch\n', err))
};


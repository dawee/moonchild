const os = require('os');
const hat = require('hat');
const path = require('path');
const programTemplate = require('./template/program');
const makefileTemplate = require('./template/makefile');
const actions = require('./actions');

const exitError = (msg, err) => {
  console.error(msg);
  console.error(err);
  process.exit(1);
};

const build = module.exports = fileName => {
  const baseName = path.basename(fileName, '.lua');
  const buildpath = path.join(path.dirname(fileName), 'build');
  const programName = path.join(buildpath, `${baseName}.c`);
  const makefileName = path.join(buildpath, `Makefile`);
  const luacTMP = path.join(os.tmpdir(), hat());
  const project = {moonRoot: path.resolve(path.join(__dirname, '..')), fileName, baseName, buildpath, programName};

  return actions.mkdirp(buildpath)
    .then(() => actions.luac(fileName, luacTMP))
    .catch(err => exitError('Error : Failed to compile Lua script\n', err))
    .then(() => actions.appendFileContent(project, fileName))
    .catch(err => exitError('Error : Failed to read lua source\n', err))
    .then(() => actions.readLuacBuffer(project, luacTMP))
    .catch(err => exitError('Error : Failed to read luac output\n', err))
    .then(() => actions.writeTemplate(makefileTemplate, makefileName, project))
    .catch(err => exitError('Error : Failed to write makefile\n', err))
    .then(() => actions.writeTemplate(programTemplate, programName, project))
    .catch(err => exitError('Error : Failed to write program\n', err))
};

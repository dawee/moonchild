const fs = require('fs');
const ncp = require('ncp');
const path = require('path');
const _mkdirp = require('mkdirp');
const {exec} = require('child_process');
const {buildToFileSync} = require('luac');
const {LuacParser} = require('../lib/parser');
const find = require('find');

const mkdirp = dir => new Promise(resolve => _mkdirp(dir, () => resolve()));

const getBinDir = () => new Promise(
  resolve => exec(`npm --prefix ${path.join(__dirname, '..')} bin`, (err, out) => resolve(out.replace(/\s+/g, '')))
);

const writeTemplate = (template, filename, ctx = {}) => new Promise(
  (resolve, reject) => fs.writeFile(filename, template(ctx), err => {
    if (!!err) return reject(err);

    resolve();
  })
);

const luac = (infile, outfile) => getBinDir().then(binDir => new Promise(resolve => {
  buildToFileSync(infile, outfile);
  resolve();
}));

const readLuacBuffer = (project, filename) => new Promise(
  (resolve, reject) => fs.readFile(filename, (err, buf) => {
    if (!!err) return reject(err);

    let res = null;
    const parser = new LuacParser(buf);

    try {
      project.proto = parser.parse();
    } catch(err) {
      return reject(err);
    }

    resolve();
  })
);

const copyTree = (src, dst) => new Promise((resolve, reject) => {
  const promises = [];

  find.eachfile(src, file => {
    const writePath = path.join(dst, path.relative(src, file.replace(/\.c$/, '.cpp')));
    const promise = mkdirp(path.dirname(writePath)).then(() => new Promise(done => {
      const read = fs.createReadStream(file);
      const write = fs.createWriteStream(writePath);

      read.pipe(write);
      write.on('finish', done);
    }));

    promises.push(promise);
  }).end(() => Promise.all(promises).then(resolve));
});

const appendFileContent = (ctx, filename) => new Promise(
  (resolve, reject) => fs.readFile(filename, 'utf8', (err, content) => {
    if (!!err) return reject(err);

    ctx.content = content;
    resolve();
  })

);

module.exports = {
  mkdirp,
  getBinDir,
  writeTemplate,
  luac,
  readLuacBuffer,
  copyTree,
  appendFileContent,
};

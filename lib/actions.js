const fs = require('fs');
const ncp = require('ncp');
const path = require('path');
const _mkdirp = require('mkdirp');
const {exec} = require('child_process');
const {LuacParser} = require('../lib/parser');


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

const luac = (infile, outfile) => getBinDir().then(binDir => new Promise(
  (resolve, reject) => exec(`${binDir}/luac -o ${outfile} ${infile}`, (err) => {
    if (!!err) return reject(err);

    resolve();
  })
));

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

const copyTree = (src, dst) => new Promise(
  (resolve, reject) => ncp(src, dst, err => {
    if (!!err) return reject(err);

    resolve();
  })
);

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
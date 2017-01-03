const os = require('os');
const fs = require('fs');
const hat = require('hat');
const path = require('path');
const ncp = require('ncp');
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

const readLuacBuffer = filename => new Promise(
  (resolve, reject) => fs.readFile(filename, (err, buf) => {
    if (!!err) return reject(err);

    let res = null;
    const parser = new LuacParser(buf);

    try {
      res = parser.parse();
    } catch(err) {
      return reject(err);
    }

    resolve(res);
  })
);

const copyTree = (src, dst) => new Promise(
  (resolve, reject) => ncp(src, dst, err => {
    if (!!err) return reject(err);

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
};
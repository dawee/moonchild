module.exports = project => `
#
# Makefile for '${project.fileName}' build
#

moon_root := ${project.moonRoot}
project_basename = ${project.baseName}
`

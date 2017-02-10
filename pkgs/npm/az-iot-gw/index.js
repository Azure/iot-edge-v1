#!/usr/bin/env node

'use strict'

var fs = require('fs');
var path = require('path');

var spawn = require('child_process').spawn;

(function() {
  var exe_file_path = null;
  var exe_args = [];

  // Start from Windows
  if (process.platform === 'win32') {
    exe_file_path = path.resolve(__dirname, './bin/win/gw.exe');
  }

  if (process.argv.length >= 3) {
    var exe_config_path = process.argv[2];
    exe_args.push(exe_config_path);
  }

  const options = { 
    stdio: 'inherit'
  };

  var child = spawn(exe_file_path, exe_args, options);

  child.on('close', (code) => {
    if (code) {
      console.error('Gateway child process exited with code = ' + code);
    }
  });
})();
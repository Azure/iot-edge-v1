// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

const channelUriPrefix = require('./constants.js').channelUriPrefix;

let EventEmitter = require('events').EventEmitter;
let nano = require('nanomsg');
let fs = require('fs');

class Channel extends EventEmitter {
  constructor() {
    super();
    this.socket = nano.socket('pair');
    this.socket.on('data', data => this.emit('data', data));
  }

  connect(id) {
    let addr = (typeof(id) === 'string') ? channelUriPrefix + id : id.uri;
    try {
      fs.unlinkSync(addr.replace(channelUriPrefix, ''));
    }
    catch (e) { }
    this.socket.bind(addr);
  }

  send(msg) {
    this.socket.send(msg);
  }

  disconnect() {
    this.socket.linger();
    this.socket.close();
  }
}

module.exports = Channel;
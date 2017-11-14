// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

const channelUriPrefix = require('../lib/constants.js').channelUriPrefix;

let nano = require('nanomsg');
let unlink = require('fs').unlink;

class TestEndpoint {
  constructor(id) {
    this.id = id;
    this.socket = nano.socket('pair');
    this.socket.connect(channelUriPrefix + id);
    this._received = new Promise((resolve) => {
      this.socket.on('data', data => resolve(data));
    });
  }

  send(msg) { this.socket.send(msg); }

  receive() { return this._received; }

  close() {
    this.socket.close();
    unlink(this.id);
  }
}

module.exports = TestEndpoint;
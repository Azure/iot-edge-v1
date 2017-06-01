// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

let Channel = require('./channel.js');
let codec = require('./codec.js');

class MessageChannel extends Channel {
  constructor() {
    super();
    super.on('data', (data) => {
      this.emit('message', codec.decodeModuleMessage(data));
    });
  }

  send(msg) {
    let buf = codec.encodeModuleMessage(msg);
    super.send(buf);
  }
}

module.exports = MessageChannel;
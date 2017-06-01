// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

let Channel = require('./channel.js');
let codec = require('./codec.js');

class ControlChannel extends Channel {
  constructor() {
    super();
    super.on('data', (data) => {
      let msg = codec.decodeControlMessage(data);
      if (msg.type === codec.controlMessageTypes.create) {
        msg = codec.decodeCreateMessage(data);
        this.emit('create', msg.messageChannelUri, msg.args);
      }
      else if (msg.type === codec.controlMessageTypes.start) {
        this.emit('start');
      }
      else if (msg.type === codec.controlMessageTypes.destroy) {
        this.emit('destroy');
      }
    });
  }

  send(op, arg) {
    let buf;
    if (op === 'create') {
      buf = codec.encodeCreateReply(arg);
    }
    else if (op === 'detach') {
      buf = codec.encodeDetachMessage();
    }
    else {
      throw new TypeError("Control channel cannot send message of type '" + op + "'");
    }

    super.send(buf);
  }
}

module.exports = ControlChannel;
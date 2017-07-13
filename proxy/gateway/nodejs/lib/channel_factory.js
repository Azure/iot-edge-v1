// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

let MessageChannel = require('./message_channel.js');
let ControlChannel = require('./control_channel.js');

class ChannelFactory {
  static createMessageChannel() {
    return new MessageChannel();
  }

  static createControlChannel() {
    return new ControlChannel();
  }
}

module.exports = ChannelFactory;
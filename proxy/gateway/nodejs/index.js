// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

let ChannelFactory = require('./lib/channel_factory.js');
let AttachedError = require('./lib/exceptions.js').AttachedError;

class ProxyGateway {
  constructor(module, channelFactory) {
    if (module === undefined) throw new TypeError("Missing 'module' argument");
    this.module = module;
    this.channelFactory = channelFactory || ChannelFactory;
    this.controlChannel = null;
    this.messageChannel = null;
  }

  _disconnectMessageChannel() {
    if (this.messageChannel) {
      this.messageChannel.disconnect();
      this.messageChannel = null;
    }
  }

  onMessage(msg) {
    this.module.receive(msg);
  }

  onCreate(messageChannelId, args) {
    if (this.messageChannel) this.onDestroy();
    this.messageChannel = this.channelFactory.createMessageChannel();
    this.messageChannel.connect(messageChannelId);
    this.messageChannel.on('message', this.onMessage.bind(this));
    let broker = {
      publish: (msg) => this.messageChannel.send(msg)
    };
    let result = this.module.create(broker, args);
    this.controlChannel.send('create', result);
  }

  onStart() {
    if (typeof(this.module.start) === 'function') {
      this.module.start();
    }
  }

  onDestroy() {
    this.module.destroy();
    this._disconnectMessageChannel();
  }

  attach(controlChannelId) {
    if (this.controlChannel) throw new AttachedError('Already attached');
    this.controlChannel = this.channelFactory.createControlChannel();
    this.controlChannel.connect(controlChannelId);
    this.controlChannel.on('create', this.onCreate.bind(this));
    this.controlChannel.on('start', this.onStart.bind(this));
    this.controlChannel.on('destroy', this.onDestroy.bind(this));
  }

  detach() {
    this.controlChannel.send('detach');
    this._disconnectMessageChannel();
    this.controlChannel.disconnect();
    this.controlChannel = null;
  }
}

module.exports = ProxyGateway;
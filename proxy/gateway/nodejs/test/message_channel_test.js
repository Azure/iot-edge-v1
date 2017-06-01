// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

let TestEndpoint = require('./test_endpoint.js');
let MessageChannel = require('../lib/message_channel.js');
let makeModuleMessage = require('./test_messages.js').makeModuleMessage;

require('chai').should();

describe('MessageChannel', () => {
  it('can receive a message', () => {
    let sender = new TestEndpoint('abc');

    let ch = new MessageChannel();
    ch.connect('abc');

    return new Promise((resolve) => {
      let moduleMessage = makeModuleMessage();

      ch.on('message', (msg) => {
        ch.disconnect();
        sender.close();

        msg.should.eql(moduleMessage.object);

        resolve();
      });

      sender.send(moduleMessage.buffer);
    });
  });

  it('can send a message', () => {
    let ch = new MessageChannel();
    let listener = new TestEndpoint('abc');

    listener.receive().then(() => {
      ch.disconnect();
      listener.close();
    });

    let message = makeModuleMessage();

    ch.connect('abc');
    ch.send(message.object);

    return listener.receive()
      .should.eventually.deep.equal(message.buffer);
  });
});
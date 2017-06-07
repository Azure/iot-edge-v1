// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

let TestEndpoint = require('./test_endpoint.js');
let MessageChannel = require('../lib/message_channel.js');
let makeModuleMessage = require('./test_messages.js').makeModuleMessage;
let uuid = require('uuid');

require('chai').should();

describe('MessageChannel', () => {
  let uri = uuid.v4();

  it('can receive a message', () => {
    let sender = new TestEndpoint(uri);

    let ch = new MessageChannel();
    ch.connect(uri);

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
    let listener = new TestEndpoint(uri);

    listener.receive().then(() => {
      ch.disconnect();
      listener.close();
    });

    let message = makeModuleMessage();

    ch.connect(uri);
    ch.send(message.object);

    return listener.receive()
      .should.eventually.deep.equal(message.buffer);
  });
});
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

let TestEndpoint = require('./test_endpoint.js');
let ControlChannel = require('../lib/control_channel.js');
let makeCreateMessage = require('./test_messages.js').makeCreateMessage;
let makeCreateReply = require('./test_messages.js').makeCreateReply;
let makeStartMessage = require('./test_messages.js').makeStartMessage;
let makeDestroyMessage = require('./test_messages.js').makeDestroyMessage;
let makeDetachMessage = require('./test_messages.js').makeDetachMessage;
let uuid = require('uuid');

require('chai').should();

describe('ControlChannel', () => {
  describe('#send', () => {
    it("throws TypeError if 'op' is not 'create' or 'detach'", () => {
      let fn = () => {
        let ch = new ControlChannel();
        ch.send('invalid');
      };

      fn.should.throw(TypeError, "Control channel cannot send message of type 'invalid'");
    });
  });

  let uri = uuid.v4();

  it("can receive a 'create' message", () => {
    let sender = new TestEndpoint(uri);

    let ch = new ControlChannel();
    ch.connect(uri);

    return new Promise((resolve) => {
      let createMessage = makeCreateMessage();

      ch.on('create', (uri, args) => {
        ch.disconnect();
        sender.close();

        uri.should.eql(createMessage.object.messageChannelUri);
        args.should.eql(createMessage.object.args);

        resolve();
      });

      sender.send(createMessage.buffer);
    });
  });

  it("can send a 'create' reply message", () => {
    let ch = new ControlChannel();
    let listener = new TestEndpoint(uri);

    listener.receive().then(() => {
      ch.disconnect();
      listener.close();
    });

    ch.connect(uri);
    ch.send('create', true);

    let expected = makeCreateReply(true).buffer;

    return listener.receive()
      .should.eventually.deep.equal(expected);
  });

  it("can receive a 'start' message", () => {
    let sender = new TestEndpoint(uri);

    let ch = new ControlChannel();
    ch.connect(uri);

    return new Promise((resolve) => {
      ch.on('start', () => {
        ch.disconnect();
        sender.close();
        resolve();
      });

      sender.send(makeStartMessage().buffer);
    });
  });

  it("can receive a 'destroy' message", () => {
    let sender = new TestEndpoint(uri);

    let ch = new ControlChannel();
    ch.connect(uri);

    return new Promise((resolve) => {
      ch.on('destroy', () => {
        ch.disconnect();
        sender.close();
        resolve();
      });

      sender.send(makeDestroyMessage().buffer);
    });
  });

  it("can send a 'detach' message", () => {
    let ch = new ControlChannel();
    let listener = new TestEndpoint(uri);

    listener.receive().then(() => {
      ch.disconnect();
      listener.close();
    });

    ch.connect(uri);
    ch.send('detach');

    let expected = makeDetachMessage().buffer;

    return listener.receive()
      .should.eventually.deep.equal(expected);
  });
});
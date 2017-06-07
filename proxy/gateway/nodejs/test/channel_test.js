// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

let chai = require('chai');
let Channel = require('../lib/channel.js');
let TestEndpoint = require('./test_endpoint.js');
let EventEmitter = require('events').EventEmitter;
let uuid = require('uuid');

chai.use(require('chai-as-promised'));
chai.should();

function parentClassNameOf(obj) {
  return Object.getPrototypeOf(obj.constructor).name;
}

describe('Channel', () => {
  let uri = uuid.v4();

  it('can send a message', () => {
    let listener = new TestEndpoint(uri);

    let ch = new Channel();
    ch.connect(uri);
    ch.send('hello');

    listener.receive().then(() => {
      ch.disconnect();
      listener.close();
    });

    return listener.receive()
      .should.eventually.deep.equal(Buffer.from('hello'));
  });

  it('can receive a message', (done) => {
    let sender = new TestEndpoint(uri);

    let ch = new Channel();
    ch.connect(uri);
    sender.send('hello');

    ch.on('data', data => {
      data.should.deep.equal(Buffer.from('hello'));
      ch.disconnect();
      sender.close();
      done();
    });
  });

  it('acts like an EventEmitter', () => {
    let ch = new Channel();
    parentClassNameOf(ch).should.equal('EventEmitter');
    // EventEmitter.on() returns EventEmitter, for chaining
    ch.on('data', () => {}).should.be.an.instanceOf(EventEmitter);
  });
});
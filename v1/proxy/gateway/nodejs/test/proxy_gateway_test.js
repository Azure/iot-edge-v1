// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/* jshint expr: true */

'use strict';

let sinon = require('sinon');
let EventEmitter = require('events').EventEmitter;
let ProxyGateway = require('../.');
let AttachedError = require('../lib/exceptions.js').AttachedError;

require('chai').should();

class TestChannel extends EventEmitter {
  constructor(onConnect, onDisconnect) {
    super();
    this.id = '';
    this.onConnect = onConnect;
    this.onDisconnect = onDisconnect;
    this.sendCalledWith = {};
  }

  connect(id) {
    this.id = id;
    this.onConnect(this);
  }

  disconnect() {
    this.onDisconnect(this);
  }

  send(arg1, arg2) {
    this.sendCalledWith = (arg2 === undefined) ?
      { arg1 } :
      { arg1, arg2 };
  }
}

class TestChannelFactory {
  constructor() {
    this.channels = new Map();
  }

  _create() {
    let addChannel = (channel) => this.channels.set(channel.id, channel);
    let removeChannel = (channel) => this.channels.delete(channel.id);
    return new TestChannel(addChannel, removeChannel);
  }

  createMessageChannel() {
    return this._create();
  }

  createControlChannel() {
    return this._create();
  }

  has(id) {
    return this.channels.has(id);
  }

  get(id) {
    return this.channels.get(id);
  }
}

class TestModule {
  constructor() {
    this.failCreate = false;
    this.created = false;
    this.broker = null;
    this.started = false;
    this.destroyed = false;
    this.args = '';
    this.received = '';
  }

  create(broker, args) {
    this.args = args;
    this.created = true;
    this.broker = broker;
    return !this.failCreate;
  }

  start() {
    this.started = true;
  }

  destroy() {
    this.destroyed = true;
  }

  receive(msg) {
    this.received = msg;
  }

  _publish() {
    this.broker.publish('test message');
  }
}

describe('ProxyGateway', () => {
  let chfac = null;
  beforeEach(() => {
    chfac = new TestChannelFactory();
  });

  describe('#constructor', () => {
    it("requires a 'module' argument", () => {
      let fn = () => new ProxyGateway();  // module is undefined
      fn.should.throw(TypeError, "Missing 'module' argument");
    });
  });

  describe('#attach', () => {
    let proxy;
    beforeEach(() => {
      proxy = new ProxyGateway({}, chfac);
    });

    it('connects the control channel', () => {
      proxy.attach('ctl');
      chfac.has('ctl').should.be.true;
    });

    it("sets up a listener for the 'create' event", () => {
      proxy.attach('ctl');
      chfac.get('ctl').listenerCount('create').should.equal(1);
    });

    it("sets up a listener for the 'start' event", () => {
      proxy.attach('ctl');
      chfac.get('ctl').listenerCount('start').should.equal(1);
    });

    it("sets up a listener for the 'destroy' event", () => {
      proxy.attach('ctl');
      chfac.get('ctl').listenerCount('destroy').should.equal(1);
    });

    it('throws if already attached', () => {
      proxy.attach('ctl');
      let fn = () => proxy.attach('ctl');
      fn.should.throw(AttachedError, 'Already attached');
    });

    it("doesn't throw if detached", () => {
      proxy.attach('ctl');
      proxy.detach();
      let fn = () => proxy.attach('ctl');
      fn.should.not.throw(Error);
    });
  });

  describe('#detach', () => {
    let module;
    let proxy;
    beforeEach(() => {
      module = new TestModule();
      proxy = new ProxyGateway(module, chfac);
      proxy.attach('ctl');
    });

    it('notifies the control channel that the application is detaching', () => {
      let ch = chfac.get('ctl'); // save a ref because detach() deletes it
      proxy.detach();
      ch.sendCalledWith.should.eql({ arg1: 'detach' });
    });

    it('disconnects the message channel if needed', () => {
      proxy.onCreate("don't care");
      proxy.detach();
      chfac.has('msg').should.be.false;
    });

    it('disconnects the control channel', () => {
      proxy.detach();
      chfac.has('ctl').should.be.false;
    });
  });

  describe('#onCreate', () => {
    let module;
    let proxy;
    let onCreate;
    let onDestroy;
    beforeEach(() => {
      module = new TestModule();
      proxy = new ProxyGateway(module, chfac);
      onCreate = sinon.spy(proxy, 'onCreate');
      onDestroy = sinon.spy(proxy, 'onDestroy');
      proxy.attach('ctl');
    });

    it("is called when the control channel emits 'create'", () => {
      chfac.get('ctl').emit('create');
      onCreate.called.should.be.true;
    });

    it('cleans up the existing module and message channel if it was already created', () => {
      proxy.onCreate('msg');
      onDestroy.called.should.be.false;
      module.destroyed.should.be.false;

      proxy.onCreate('msg');
      onDestroy.called.should.be.true;
      module.destroyed.should.be.true;
      chfac.has('msg').should.be.true;
    });

    it("connects the message channel with the given id", () => {
      proxy.onCreate('msg');
      chfac.has('msg').should.be.true;
    });

    it("sets up a listener for the 'message' event", () => {
      proxy.onCreate('msg');
      chfac.get('msg').listenerCount('message').should.equal(1);
    });

    it("creates the module", () => {
      let args = "arguments to onCreate";
      proxy.onCreate('msg', args);
      module._publish();
      module.created.should.be.true;
      module.args.should.equal(args);
      chfac.get('msg').sendCalledWith.should.eql({ arg1: 'test message' });
    });

    it('notifies the control channel when the module is created', () => {
      proxy.onCreate("don't care");
      chfac.get('ctl').sendCalledWith.should.eql({ arg1: 'create', arg2: true });
    });

    it('notifies the control channel when module creation fails', () => {
      module.failCreate = true;
      proxy.onCreate("don't care");
      chfac.get('ctl').sendCalledWith.should.eql({ arg1: 'create', arg2: false });
    });
  });

  describe('#onMessage', () => {
    let module;
    let proxy;
    let onMessage;
    beforeEach(() => {
      module = new TestModule();
      proxy = new ProxyGateway(module, chfac);
      onMessage = sinon.spy(proxy, 'onMessage');
      proxy.attach('ctl');
    });

    it("is called when the message channel emits 'message'", () => {
      proxy.onCreate('msg');
      chfac.get('msg').emit('message');
      onMessage.called.should.be.true;
    });

    it('gives the message to the module', () => {
      proxy.onCreate('msg');
      chfac.get('msg').emit('message', 'content');
      module.received.should.equal('content');
    });
  });

  describe('#onStart', () => {
    let module;
    let proxy;
    let onStart;
    beforeEach(() => {
      module = new TestModule();
      proxy = new ProxyGateway(module, chfac);
      onStart = sinon.spy(proxy, 'onStart');
      proxy.attach('ctl');
    });

    it("is called when the control channel emits 'start'", () => {
      chfac.get('ctl').emit('start');
      onStart.called.should.be.true;
    });

    it("doesn't call start if the module doesn't implement it", () => {
      module.start = undefined;
      proxy.onStart();
      module.started.should.be.false;
    });

    it('starts the module', () => {
      proxy.onStart();
      module.started.should.be.true;
    });
  });

  describe('#onDestroy', () => {
    let module;
    let proxy;
    let onDestroy;
    beforeEach(() => {
      module = new TestModule();
      proxy = new ProxyGateway(module, chfac);
      onDestroy = sinon.spy(proxy, 'onDestroy');
      proxy.attach('ctl');
    });

    it("is called when the control channle emits 'destroy'", () => {
      chfac.get('ctl').emit('destroy');
      onDestroy.called.should.be.true;
    });

    it('destroys the module', () => {
      proxy.onDestroy();
      module.destroyed.should.be.true;
    });

    it("disconnects the message channel", () => {
      proxy.onCreate('msg');
      proxy.onDestroy();
      chfac.has('msg').should.be.false;
    });
  });
});
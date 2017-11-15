// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/* jshint expr: true */

'use strict';

const channelUriPrefix = require('../lib/constants.js').channelUriPrefix;

let codec = require('../lib/codec.js');
let DecodeError = require('../lib/exceptions.js').DecodeError;
let EncodeError = require('../lib/exceptions.js').EncodeError;
let makeControlMessage = require('./test_messages.js').makeControlMessage;
let makeMessageChannelUri = require('./test_messages.js').makeMessageChannelUri;
let makeCreateMessage = require('./test_messages.js').makeCreateMessage;
let makeCreateReply = require('./test_messages.js').makeCreateReply;
let makeModuleMessageProperties = require('./test_messages.js').makeModuleMessageProperties;
let makeModuleMessage = require('./test_messages.js').makeModuleMessage;

require('chai').should();

function fuzzMessage(buf, index, value) {
  buf[index] = value;
  return buf;
}

describe('Codec', () => {
  describe('#encodeControlMessage', () => {
    it('encodes a control message', () => {
      let msg = makeControlMessage();
      codec.encodeControlMessage(msg.object).should.eql(msg.buffer);
    });
  });

  describe('#decodeControlMessage', () => {
    it('decodes a control message', () => {
      let msg = makeControlMessage();
      codec.decodeControlMessage(msg.buffer).should.eql(msg.object);
    });

    it("throws DecodeError if the message doesn't have header bytes", () => {
      let messages = [
        fuzzMessage(makeControlMessage().buffer, 0, 0x77), // replace 1st byte in valid message
        fuzzMessage(makeControlMessage().buffer, 1, 0x77), // replace 2nd byte in valid message
        makeControlMessage().buffer.slice(2)               // header bytes are missing
      ];

      messages.forEach(msg => {
        let fn = () => codec.decodeControlMessage(msg);
        fn.should.throw(DecodeError, 'Header bytes are missing or incorrect');
      }, this);
    });

    it('throws RangeError if the message is too short', () => {
      let msg = makeControlMessage().buffer.slice(0, -1);
      let fn = () => codec.decodeControlMessage(msg);
      fn.should.throw(RangeError);
    });
  });

  describe('#decodeMessageChannelUri', () => {
    it('decodes a message channel URI', () => {
      let uri = makeMessageChannelUri('abc');
      codec.decodeMessageChannelUri(uri.buffer).value.should.eql(uri.object);
    });

    it('returns the number of bytes read from the buffer', () => {
      let uri = makeMessageChannelUri('abc');
      codec.decodeMessageChannelUri(uri.buffer).length.should.equal(uri.buffer.length);
    });

    it('reads the right number of bytes for the URI string', () => {
      let buf = Buffer.allocUnsafe(128).fill('X');
      let uri = makeMessageChannelUri('abc', buf);
      codec.decodeMessageChannelUri(uri.buffer).value.should.eql(uri.object);
    });

    it('throws RangeError if the buffer is too short', () => {
      let uris = [
        makeMessageChannelUri('abc').buffer.slice(0, 4), // missing last byte of size
        makeMessageChannelUri('abc').buffer.slice(0, -1) // missing last byte of URI string
      ];

      uris.forEach(uri => {
        let fn = () => codec.decodeMessageChannelUri(uri);
        fn.should.throw(RangeError);
      }, this);
    });

    it("throws DecodeError if the URI doesn't start with 'ipc://'", () => {
      let uri = makeMessageChannelUri('abc', s => s.replace(channelUriPrefix, ''));
      let fn = () => codec.decodeMessageChannelUri(uri.buffer);
      fn.should.throw(DecodeError, "Message channel URI does not start with 'ipc://': abc");
    });

    it('optionally reads the buffer from the given offset', () => {
      let buf = Buffer.alloc(128);
      let uri = makeMessageChannelUri('abc', buf, 10);
      codec.decodeMessageChannelUri(uri.buffer, 10).value.should.eql(uri.object);
    });
  });

  describe('#decodeCreateMessage', () => {
    it("decodes a 'create' message", () => {
      let msg = makeCreateMessage();
      codec.decodeCreateMessage(msg.buffer).should.eql(msg.object);
    });

    it('throws RangeError if the module arguments are too short', () => {
      let msg = makeCreateMessage().buffer.slice(0, -1);
      let fn = () => codec.decodeCreateMessage(msg);
      fn.should.throw(RangeError);
    });

    it('reads the right number of bytes for the module args', () => {
      let msg = makeCreateMessage(128);
      codec.decodeCreateMessage(msg.buffer).should.eql(msg.object);
    });
  });

  describe('#encodeCreateReply', () => {
    it("encodes a successful 'create' reply message", () => {
      let msg = makeCreateReply(true);
      codec.encodeCreateReply(msg.object).should.eql(msg.buffer);
    });

    it("encodes a failed 'create' reply message", () => {
      let msg = makeCreateReply(false);
      codec.encodeCreateReply(false).should.eql(msg.buffer);
    });
  });

  describe('#encodeModuleMessageProperties', () => {
    it('returns an empty buffer when given an empty object', () => {
      let result = codec.encodeModuleMessageProperties({});
      result.buffer.should.eql(new Buffer(0));
      result.count.should.equal(0);
    });

    it('throws when a property value is null', () => {
      let fn = () => {
        let props = { key: null };
        console.log(codec.encodeModuleMessageProperties(props));
      };

      fn.should.throw(EncodeError, "Property 'key' has value 'null'");
    });

    it('encodes module message properties', () => {
      let props = makeModuleMessageProperties();
      let result = codec.encodeModuleMessageProperties(props.object);
      result.buffer.should.eql(props.buffer);
      result.count.should.equal(props.count);
    });
  });

  describe('#decodeModuleMessageProperties', () => {
    it('returns an empty object when given an empty buffer', () => {
      codec.decodeModuleMessageProperties(new Buffer(0), 1).properties
        .should.be.empty;
    });

    it('returns an empty object when asked to decode zero properties', () => {
      codec.decodeModuleMessageProperties(new Buffer(1), 0).properties
        .should.be.empty;
    });

    it('throws when a property key is not null-terminated', () => {
      let fn = () => {
        let buf = Buffer.from('key');
        codec.decodeModuleMessageProperties(buf, 1);
      };

      fn.should.throw(DecodeError, 'Property key was not null-terminated');
    });

    it('throws when a property value is not null-terminated', () => {
      let fn = () => {
        let buf = Buffer.from('key\0value');
        codec.decodeModuleMessageProperties(buf, 1);
      };

      fn.should.throw(DecodeError, 'Property value was not null-terminated');
    });

    it('throws when a property has no value', () => {
      let fn = () => {
        let buf = Buffer.from('key\0');
        codec.decodeModuleMessageProperties(buf, 1);
      };

      fn.should.throw(Error);
    });

    it('only decodes `count` properties', () => {
      let buf = Buffer.from('key1\0val1\0key2\0val2\0');
      codec.decodeModuleMessageProperties(buf, 1).properties
        .should.eql({ 'key1': 'val1' });
    });

    it('decodes multiple properties', () => {
      let props = makeModuleMessageProperties();
      codec.decodeModuleMessageProperties(props.buffer, props.count).properties
        .should.eql(props.object);
    });

    it('returns the offset of the first byte after the properties', () => {
      let props = makeModuleMessageProperties();
      codec.decodeModuleMessageProperties(props.buffer, 2).offset
        .should.equal(props.buffer.length);
    });
  });

  describe('encodeModuleMessage', () => {
    it('throws if `properties` is null', () => {
      let fn = () => {
        let msg = { properties: null, content: Buffer.alloc(0) };
        codec.encodeModuleMessage(msg);
      };

      fn.should.throw(EncodeError, "Module message 'properties' is 'null'");
    });

    ['hello', 123, [], null].forEach((content) => {
      it('throws if `content` is not a Buffer', () => {
        let fn = () => {
          let msg = { properties: {}, content };
          codec.encodeModuleMessage(msg);
        };

        fn.should.throw(EncodeError, "Module message 'content' is not a Buffer");
      });
    });

    it('encodes an empty message from an empty object', () => {
      let object = { properties: {}, content: Buffer.alloc(0) };

      let buffer = Buffer.alloc(14);
      buffer.writeUInt16BE(0xA160, 0);
      buffer.writeUInt32BE(buffer.length, 2);
      buffer.writeUInt32BE(0, 6);
      buffer.writeUInt32BE(0, 10);

      codec.encodeModuleMessage(object)
        .should.eql(buffer);
    });

    it('encodes a module message', () => {
      let msg = makeModuleMessage();
      codec.encodeModuleMessage(msg.object)
        .should.eql(msg.buffer);
    });
  });

  describe('decodeModuleMessage', () => {
    it("throws when the message doesn't start with header bytes 0xA160", () => {
      let fn = () => {
        let buf = Buffer.from('hello');
        codec.decodeModuleMessage(buf);
      };

      fn.should.throw(DecodeError, 'Header bytes are missing or incorrect');
    });

    it('decodes a message', () => {
      let msg = makeModuleMessage();
      codec.decodeModuleMessage(msg.buffer)
        .should.eql(msg.object);
    });
  });
});
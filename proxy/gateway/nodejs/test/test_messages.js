// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

const channelUriPrefix = require('../lib/constants.js').channelUriPrefix;

let nano = require('nanomsg');
let controlMessageTypes = require('../lib/codec.js').controlMessageTypes;
let controlMessageReply = require('../lib/codec.js').controlMessageReply;
let header = require('../lib/codec.js').headerBytes;

function makeControlMessage(type = controlMessageTypes.create) {
  let buffer = Buffer.alloc(4);
  buffer.writeUInt8(header.first, 0);
  buffer.writeUInt8(header.second.control, 1);
  buffer.writeUInt8(1, 2); // CONTROL_MESSAGE.version
  buffer.writeUInt8(type, 3); // CONTROL_MESSAGE.type

  let object = { version: 1, type: type };

  return { buffer, object };
}

function makeMessageChannelUri(uri, buffer, offset = 0) {
  if (!uri.startsWith(channelUriPrefix)) uri = channelUriPrefix + uri;
  // 2nd arg is either a caller-provided Buffer, or a URI string mutator function
  if (typeof(buffer) === 'function') {
    let mutator = buffer;
    uri = mutator(uri);
    buffer = null;
  }

  let nullTerm = s => s + '\0'; // value from gateway is null-terminated
  let len = Buffer.from(nullTerm(uri), 'utf8').length;
  if (!buffer) buffer = Buffer.alloc(5 + len);
  buffer.writeUInt8(nano._bindings.NN_PAIR, offset);
  buffer.writeUInt32BE(len, offset + 1);
  buffer.write(nullTerm(uri), offset + 5);

  let object = { type: 'pair', uri };

  return { buffer, object };
}

function makeCreateMessage(bufferLength) {
  let uri = makeMessageChannelUri('abc');

  let buffers = [
    makeControlMessage().buffer,
    Buffer.alloc(4),  // total message size
    Buffer.from([1]),
    uri.buffer,
    Buffer.alloc(4),  // args size
    Buffer.from('xyz')
  ];

  let buffer = Buffer.concat(buffers, bufferLength);

  let offset = buffer.writeUInt32BE(buffer.length, buffers[0].length);
  buffer.writeUInt32BE(buffers[5].length, offset + buffers[2].length + buffers[3].length);

  let object = {
    control: { version: 1, type: controlMessageTypes.create },
    version: 1,
    messageChannelUri: uri.object,
    args: Buffer.from('xyz')
  };

  return { buffer, object };
}

function makeCreateReply(createSucceeded) {
  let result = createSucceeded ?
    controlMessageReply.created :
    controlMessageReply.createError;

  let object = {
    control: { version: 1, type: controlMessageTypes.reply },
    result: result
  };

  let buffers = [
    makeControlMessage(controlMessageTypes.reply).buffer,
    Buffer.alloc(4),  // total message size
    Buffer.from([result])
  ];

  let buffer = Buffer.concat(buffers);

  buffer.writeUInt32BE(buffer.length, buffers[0].length);

  return { buffer, object };
}

function makeStartMessage() {
  return makeControlMessage(controlMessageTypes.start);
}

function makeDestroyMessage() {
  return makeControlMessage(controlMessageTypes.destroy);
}

function makeDetachMessage() {
  let object = {
    control: { version: 1, type: controlMessageTypes.reply },
    result: controlMessageReply.detach
  };

  let buffers = [
    makeControlMessage(controlMessageTypes.reply).buffer,
    Buffer.alloc(4),  // total message size
    Buffer.from([controlMessageReply.detach])
  ];

  let buffer = Buffer.concat(buffers);

  buffer.writeUInt32BE(buffer.length, buffers[0].length);

  return { buffer, object };
}

function makeModuleMessageProperties() {
  let count = 2;

  let object = {
    key1: "value1",
    key2: "value2"
  };

  let buffer = Buffer.from('key1\0value1\0key2\0value2\0');

  return { object, buffer, count };
}

function makeModuleMessage() {
  let properties = makeModuleMessageProperties();

  let object = {
    properties: properties.object,
    content: new Buffer('hello')
  };

  let length = properties.buffer.length + object.content.length + 14;
  let buffer = Buffer.alloc(length);
  let offset = buffer.writeUInt8(header.first, 0);
  offset = buffer.writeUInt8(header.second.message, 1);
  offset = buffer.writeUInt32BE(buffer.length, offset);
  offset = buffer.writeUInt32BE(properties.count, offset);
  offset += properties.buffer.copy(buffer, offset);
  offset = buffer.writeUInt32BE(object.content.length, offset);
  object.content.copy(buffer, offset);

  return { buffer, object };
}

module.exports = {
  makeControlMessage,
  makeMessageChannelUri,
  makeCreateMessage,
  makeCreateReply,
  makeStartMessage,
  makeDestroyMessage,
  makeDetachMessage,
  makeModuleMessageProperties,
  makeModuleMessage
};
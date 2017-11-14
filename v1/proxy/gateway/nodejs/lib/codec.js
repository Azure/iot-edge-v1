// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

// The formats of various control/module messages are documented at proxy/message_format.md.

const channelUriPrefix = require('./constants.js').channelUriPrefix;

let nano = require('nanomsg');
let DecodeError = require('./exceptions.js').DecodeError;
let EncodeError = require('./exceptions.js').EncodeError;

let headerBytes = {
  first: 0xA1,
  second: {
    message: 0x60,
    control: 0x6C
  }
};

let controlMessageTypes = {
    error: 0,
    create: 1,                 
    reply: 2,
    start: 3,
    destroy: 4
};

let controlMessageReply = {
  detach: -1,
  created: 0,
  createError: 1
};

function _uriTypeToString(type) {
    if (type === nano._bindings.NN_PAIR) return 'pair';
    return '';
}

function encodeControlMessage(obj) {
  let buffer = Buffer.alloc(4);
  buffer.writeUInt8(headerBytes.first, 0);
  buffer.writeUInt8(headerBytes.second.control, 1);
  buffer.writeUInt8(obj.version, 2);
  buffer.writeUInt8(obj.type, 3);
  return buffer;
}

function decodeControlMessage(buf) {
  if (buf.readUInt8(0) !== headerBytes.first ||
    buf.readUInt8(1) !== headerBytes.second.control) {
    throw new DecodeError('Header bytes are missing or incorrect');
  }
  return {
    version: buf.readUInt8(2),
    type: buf.readUInt8(3)
  };
}

function decodeMessageChannelUri(buf, offset = 0) {
  let length = 5 + buf.readUInt32BE(offset + 1);
  if (offset + length > buf.length) throw new RangeError();
  let result = {
    value: {
      type: _uriTypeToString(buf.readUInt8(offset)),
      uri: buf.toString('utf8', offset + 5, offset + length - 1)
    },
    length: length
  };
  if (!result.value.uri.startsWith(channelUriPrefix)) {
    throw new DecodeError(
      "Message channel URI does not start with '" + channelUriPrefix + "': " + result.value.uri);
  }
  return result;
}

function decodeCreateMessage(buf) {
  let uriResult = decodeMessageChannelUri(buf, 9);
  let startArgs = 9 + uriResult.length + 4;
  let endArgs = startArgs + buf.readUInt32BE(startArgs - 4);
  if (buf.length < endArgs) throw new RangeError();
  return {
    control: decodeControlMessage(buf),
    version: buf.readUInt8(8),
    messageChannelUri: uriResult.value,
    args: buf.slice(startArgs, endArgs)
  };
}

function _encodeReplyMessage(type) {
  let control = { version: 1, type: controlMessageTypes.reply };

  let buffers = [
    encodeControlMessage(control),
    Buffer.alloc(4),  // total message size
    Buffer.from([type])
  ];

  let buffer = Buffer.concat(buffers);
  buffer.writeUInt32BE(buffer.length, buffers[0].length);

  return buffer;
}

function encodeCreateReply(succeeded) {
  let type = succeeded ?
    controlMessageReply.created :
    controlMessageReply.createError;
  return _encodeReplyMessage(type);
}

function encodeDetachMessage() {
  return _encodeReplyMessage(controlMessageReply.detach);
}

function encodeModuleMessageProperties(obj) {
  let parts = [];
  let keys = Object.keys(obj);
  keys.forEach((key) => {
    if (obj[key] === null) {
      throw new EncodeError("Property '" + key + "' has value 'null'");
    }
    parts.push(key + '\0' + obj[key] + '\0');
  });

  let buffer = Buffer.from(parts.join(''));

  return { buffer, count: keys.length };
}

function decodeModuleMessageProperties(buf, count) {
  let offset = 0;
  let properties = {};

  while (offset < buf.length && count-- > 0) {
    let term = buf.indexOf('\0', offset);
    if (term === -1) {
      throw new DecodeError('Property key was not null-terminated');
    }
    let key = buf.toString('utf8', offset, term);

    offset = ++term;
    term = buf.indexOf('\0', offset);
    if (term === -1) {
      throw new DecodeError('Property value was not null-terminated');
    }
    let value = buf.toString('utf8', offset, term);

    offset = ++term;
    properties[key] = value;
  }

  return { properties, offset };
}

function encodeModuleMessage(msg) {
  if (msg.properties === null) {
    throw new EncodeError("Module message 'properties' is 'null'");
  }
  if (!Buffer.isBuffer(msg.content)) {
    throw new EncodeError("Module message 'content' is not a Buffer");
  }

  let properties = encodeModuleMessageProperties(msg.properties);
  let length = properties.buffer.length + msg.content.length + 14;
  let buffer = Buffer.alloc(length);
  let offset = buffer.writeUInt8(headerBytes.first, 0);
  offset = buffer.writeUInt8(headerBytes.second.message, 1);
  offset = buffer.writeUInt32BE(buffer.length, offset);
  offset = buffer.writeUInt32BE(properties.count, offset);
  offset += properties.buffer.copy(buffer, offset);
  offset = buffer.writeUInt32BE(msg.content.length, offset);
  msg.content.copy(buffer, offset);

  return buffer;
}

function decodeModuleMessage(buf) {
  if (buf.readUInt8(0) !== headerBytes.first ||
    buf.readUInt8(1) !== headerBytes.second.message) {
    throw new DecodeError('Header bytes are missing or incorrect');
  }

  let count = buf.readUInt32BE(6);
  let obj = decodeModuleMessageProperties(buf.slice(10), count);
  let contentStart = 10 + obj.offset + 4;
  let contentEnd = contentStart + buf.readUInt32BE(contentStart - 4);

  return {
    properties: obj.properties,
    content: buf.slice(contentStart, contentEnd)
  };
}

module.exports = {
  headerBytes,
  controlMessageTypes,
  controlMessageReply,
  encodeControlMessage,
  decodeControlMessage,
  decodeMessageChannelUri,
  decodeCreateMessage,
  encodeCreateReply,
  encodeDetachMessage,
  encodeModuleMessageProperties,
  decodeModuleMessageProperties,
  encodeModuleMessage,
  decodeModuleMessage
};
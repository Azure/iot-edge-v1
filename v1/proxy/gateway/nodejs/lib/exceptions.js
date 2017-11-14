// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

class AttachedError extends Error {
  constructor(message) {
    super(message);
    this.name = 'AttachedError';
  }
}

class DecodeError extends Error {
  constructor(message) {
    super(message);
    this.name = 'DecodeError';
  }
}

class EncodeError extends Error {
  constructor(message) {
    super(message);
    this.name = 'EncodeError';
  }
}

module.exports = {
    AttachedError,
    DecodeError,
    EncodeError
};
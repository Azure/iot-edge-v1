// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

let ProxyGateway = require('../../proxy/gateway/nodejs/index.js');

// This gateway module simply publishes whatever it receives.
let forward = {
    broker: null,
    create: function (broker) {
        this.broker = broker;
        return true;
    },
    start: function () {},
    receive: function (message) {
        let buf = Buffer.from(message.content);
        console.log(`forward.receive - ${buf.toString('utf8')}`);
        this.broker.publish(message);
    },
    destroy: function() {
        this.broker = null;
    }
};

if (!process.argv[2]) {
    console.log('USAGE: node path/to/app.js <control-id>');
    process.exit(1);
}

let gateway = new ProxyGateway(forward);
gateway.attach(process.argv[2]);

process.stdin.setRawMode(true);
process.stdin
    .resume()
    .setEncoding('utf8')
    .on('data', function () { 
        gateway.detach();
        process.exit();
    });
Modules implemented in Node.js must *default-export* (e.g. 
`module.exports = { ... };`) the object that implements the `GatewayModule` 
interface, which consists of four simple functions, as described below.

The API definition is provided using TypeScript syntax for expository purposes:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ts
/**
 * A string-to-string associative collection.
 */
interface StringMap {
    [key: string]: string;
}

/**
 * A message object that can be published to a message broker or
 * is received from a message broker.
 */
interface Message {
    properties: StringMap;
    content: Uint8Array;
}

/**
 * A message broker object, passed to the module when it is created, and used
 * by the module to send messages.
 */
interface Broker {
    /** Sends a message to the broker. */
    publish: (message: Message) => boolean;
}

/**
 * This interface is expected to be implemented by the module author. This is
 * where the core module functionality resides. The methods in this interface
 * have a 1:1 mapping with the functions expected of a regular C gateway module.
 */
interface GatewayModule {
    /** Called by the gateway when it loads this module. Receives as arguments a
     *  broker for sending messages, and any module-specific configuration.
     *  Returns true if creation succeeded, false otherwise.
     */
    create: (broker: Broker, configuration: any) => boolean;

    /** Called by the gateway when it is safe for the module to begin sending
     *  and receiving messages.
     */
    start: () => void;

    /** Called by the broker when it has received a message intended for this
     *  module.
     */
    receive: (message: Message) => void;

    /** Called by the gateway just prior to unloading this module. Messages
     *  cannot be sent from or directed to this module after the gateway has
     *  called destroy().
     */
    destroy: () => void;
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

'use strict';

module.exports = {
    messageBus: null,
    configuration: null,

    create: function (messageBus, configuration) {
        this.messageBus = messageBus;
        this.configuration = configuration;

        return true;
    },

    receive: function (message) {
        console.log(`printer.receive - ${message.content.join(', ')}`);
    },

    destroy: function () {
        console.log('printer.destroy');
    }
};
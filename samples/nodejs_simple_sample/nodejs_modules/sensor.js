'use strict';

module.exports = {
    messageBus: null,
    configuration: null,

    create: function (messageBus, configuration) {
        this.messageBus = messageBus;
        this.configuration = configuration;

        setInterval(() => {
            this.messageBus.publish({
                properties: {
                    'source': 'sensor'
                },
                content: new Uint8Array([
                    Math.random() * 50,
                    Math.random() * 50
                ])
            });
        }, 500);

        return true;
    },

    receive: function(message) {
    },

    destroy: function() {
        console.log('sensor.destroy');
    }
};

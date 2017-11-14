/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

import com.microsoft.azure.gateway.core.IGatewayModule;

/**
 * A remote module configuration that should contain the identifier used to
 * connect to the Gateway, the module class and the message version. The
 * identifier has to be the same value that was configured on the Gateway side.
 * The module class has to be an implementation of IGatewayModule. Version is
 * the messages version that has to be compatible with the message version the
 * Gateway is sending. Default value is set to 1.
 * 
 * {@code ModuleConfiguration} has no public constructor. To create an instance
 * of the configuration use the {@code ModuleConfiguration.Builder}:
 * 
 * <pre>
 *  {@code
 *  String id = "control-id";
 *  byte version = 1;
 *  ModuleConfiguration.Builder configBuilder = new ModuleConfiguration.Builder();
 *  configBuilder.setIdentifier(id);
 *  configBuilder.setModuleClass(Printer.class);
 *  configBuilder.setModuleVersion(version);
 *      
 *  ModuleConfiguration config = configBuilder.build();
 * }}
 * </pre>
 *
 */
public class ModuleConfiguration {
    private final String identifier;
    private final Class<? extends IGatewayModule> moduleClass;
    private final byte version;

    // Codes_SRS_MODULE_CONFIGURATION_24_001: [ ModuleConfiguration shall not have a private constructor . ]
    private ModuleConfiguration(String identifier, Class<? extends IGatewayModule> moduleClass, byte version) {
        this.identifier = identifier;
        this.moduleClass = moduleClass;
        this.version = version;
    }

    // Codes_SRS_MODULE_CONFIGURATION_24_003: [ It shall return the control channel identification. ]
    public String getIdentifier() {
        return identifier;
    }

    // Codes_SRS_MODULE_CONFIGURATION_24_004: [ It shall return the module class. ]
    public Class<? extends IGatewayModule> getModuleClass() {
        return moduleClass;
    }

    // Codes_SRS_MODULE_CONFIGURATION_24_005: [ It shall return the version. ]
    public byte getVersion() {
        return version;
    }

    /**
     * A builder for {@code ModuleConfiguration}.
     *
     */
    // Codes_SRS_MODULE_CONFIGURATION_24_002: [ ModuleConfiguration shall have a Builder that creates new instances of ModuleConfiguration . ]
    public static class Builder {
        private final static byte DEFAULT_VERSION = 1;

        private String identifier;
        private Class<? extends IGatewayModule> moduleClass;
        private byte version;

        public Builder() {
        }

        // Codes_SRS_MODULE_CONFIGURATION_BUILDER_24_006: [ It shall save `moduleClass` into member field.  ]
        public Builder setModuleClass(Class<? extends IGatewayModule> moduleClass) {
            this.moduleClass = moduleClass;
            return this;
        }

        // Codes_SRS_MODULE_CONFIGURATION_BUILDER_24_007: [ It shall save `identifier` into member field.  ]
        public Builder setIdentifier(String identifier) {
            this.identifier = identifier;
            return this;
        }

        // Codes_SRS_MODULE_CONFIGURATION_BUILDER_24_008: [ It shall save `version` into member field.  ]
        public Builder setModuleVersion(byte version) {
            this.version = version;
            return this;
        }

        /**
         * Creates an {@code ModuleConfiguration} instance.
         *
         * @throws IllegalArgumentException
         *             If the identifier is null, the moduleClass is null or if
         *             version is negative
         */
        public ModuleConfiguration build() {
            // Codes_SRS_MODULE_CONFIGURATION_BUILDER_24_010: [ If identifier is null it shall throw IllegalArgumentException. ]
            // Codes_SRS_MODULE_CONFIGURATION_BUILDER_24_011: [ If moduleClass is null it shall throw IllegalArgumentException. ]
            if (this.identifier == null || this.moduleClass == null)
                throw new IllegalArgumentException("Identifier and module class are required.");
            
            // Codes_SRS_MODULE_CONFIGURATION_BUILDER_24_012: [ If version is negative it shall throw IllegalArgumentException. ]
            if (this.version < 0)
                throw new IllegalArgumentException("Version can not have negative.");
                
            // Codes_SRS_MODULE_CONFIGURATION_BUILDER_24_013: [ If version is not set it shall use default version. ]   
            if (this.version == 0)
                this.version = DEFAULT_VERSION;

            // Codes_SRS_MODULE_CONFIGURATION_BUILDER_24_009: [ It shall create a new ModuleConfiguration instance. ]
            return new ModuleConfiguration(this.identifier, this.moduleClass, this.version);
        }
    }
}
/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class ModuleConfigurationTest {

    private static final byte INVALID_VERSION = -1;
    private static final byte VERSION = 1;
    private static final Class<TestModuleImplementsInterface> MODULE_CLASS = TestModuleImplementsInterface.class;
    private static final String IDENTIFIER = "Test";

    // Tests_SRS_MODULE_CONFIGURATION_24_001: [ ModuleConfiguration shall not have a private constructor . ]
    // Tests_SRS_MODULE_CONFIGURATION_24_002: [ ModuleConfiguration shall have a Builder that creates new instances of ModuleConfiguration . ]
    // Tests_SRS_MODULE_CONFIGURATION_BUILDER_24_006: [ It shall save `moduleClass` into member field.  ]
    // Tests_SRS_MODULE_CONFIGURATION_BUILDER_24_007: [ It shall save `identifier` into member field.  ]
    // Tests_SRS_MODULE_CONFIGURATION_BUILDER_24_008: [ It shall save `version` into member field.  ]
    // Tests_SRS_MODULE_CONFIGURATION_BUILDER_24_009: [ It shall create a new ModuleConfiguration instance. ]
    // Tests_SRS_MODULE_CONFIGURATION_24_003: [ It shall return the control channel identification. ]
    // Tests_SRS_MODULE_CONFIGURATION_24_004: [ It shall return the module class. ]
    // Tests_SRS_MODULE_CONFIGURATION_24_005: [ It shall return the version. ]
    @Test
    public void buildSuccess() {
        ModuleConfiguration.Builder builder = new ModuleConfiguration.Builder();
        builder.setIdentifier(IDENTIFIER).setModuleClass(MODULE_CLASS).setModuleVersion(VERSION);
        
        ModuleConfiguration config = builder.build();
        
        assertEquals(IDENTIFIER, config.getIdentifier());
        assertEquals(MODULE_CLASS, config.getModuleClass());
        assertEquals(VERSION, config.getVersion());
    }

    // Tests_SRS_MODULE_CONFIGURATION_24_001: [ ModuleConfiguration shall not have a private constructor . ]
    // Tests_SRS_MODULE_CONFIGURATION_24_002: [ ModuleConfiguration shall have a Builder that creates new instances of ModuleConfiguration . ]
    // Tests_SRS_MODULE_CONFIGURATION_BUILDER_24_006: [ It shall save `moduleClass` into member field.  ]
    // Tests_SRS_MODULE_CONFIGURATION_BUILDER_24_007: [ It shall save `identifier` into member field.  ]
    // Tests_SRS_MODULE_CONFIGURATION_BUILDER_24_009: [ It shall create a new ModuleConfiguration instance. ]
    // Tests_SRS_MODULE_CONFIGURATION_24_003: [ It shall return the control channel identification. ]
    // Tests_SRS_MODULE_CONFIGURATION_24_004: [ It shall return the module class. ]
    // Tests_SRS_MODULE_CONFIGURATION_BUILDER_24_013: [ If version is not set it shall use default version. ] 
    @Test
    public void buildSetDefaultVersionIfVersionNotSet() {
        ModuleConfiguration.Builder builder = new ModuleConfiguration.Builder();
        builder.setIdentifier(IDENTIFIER).setModuleClass(MODULE_CLASS);

        ModuleConfiguration config = builder.build();

        assertEquals(config.getIdentifier(), IDENTIFIER);
        assertEquals(config.getModuleClass(), MODULE_CLASS);
        assertEquals(config.getVersion(), VERSION);
    }
    
    // Tests_SRS_MODULE_CONFIGURATION_24_001: [ ModuleConfiguration shall not have a private constructor . ]
    // Tests_SRS_MODULE_CONFIGURATION_24_002: [ ModuleConfiguration shall have a Builder that creates new instances of ModuleConfiguration . ]
    // Tests_SRS_MODULE_CONFIGURATION_BUILDER_24_006: [ It shall save `moduleClass` into member field.  ]
    // Tests_SRS_MODULE_CONFIGURATION_BUILDER_24_008: [ It shall save `version` into member field.  ]
    // Tests_SRS_MODULE_CONFIGURATION_BUILDER_24_010: [ If identifier is null it shall throw IllegalArgumentException. ]
    @Test(expected = IllegalArgumentException.class)
    public void buildShouldFailIfIdentifierNotSet() {
        ModuleConfiguration.Builder builder = new ModuleConfiguration.Builder();
        builder.setModuleClass(MODULE_CLASS).setModuleVersion(VERSION);
        
        builder.build();
    }

    // Tests_SRS_MODULE_CONFIGURATION_24_001: [ ModuleConfiguration shall not have a private constructor . ]
    // Tests_SRS_MODULE_CONFIGURATION_24_002: [ ModuleConfiguration shall have a Builder that creates new instances of ModuleConfiguration . ]
    // Tests_SRS_MODULE_CONFIGURATION_BUILDER_24_007: [ It shall save `identifier` into member field.  ]
    // Tests_SRS_MODULE_CONFIGURATION_BUILDER_24_008: [ It shall save `version` into member field.  ]
    // Tests_SRS_MODULE_CONFIGURATION_BUILDER_24_011: [ If moduleClass is null it shall throw IllegalArgumentException. ]
    @Test(expected = IllegalArgumentException.class)
    public void buildShouldFailIfModuleClassNotSet() {
        ModuleConfiguration.Builder builder = new ModuleConfiguration.Builder();
        builder.setIdentifier(IDENTIFIER).setModuleVersion(VERSION);
        
        builder.build();
    }
   
    // Tests_SRS_MODULE_CONFIGURATION_24_001: [ ModuleConfiguration shall not have a private constructor . ]
    // Tests_SRS_MODULE_CONFIGURATION_24_002: [ ModuleConfiguration shall have a Builder that creates new instances of ModuleConfiguration . ]
    // Tests_SRS_MODULE_CONFIGURATION_BUILDER_24_006: [ It shall save `moduleClass` into member field.  ]
    // Tests_SRS_MODULE_CONFIGURATION_BUILDER_24_007: [ It shall save `identifier` into member field.  ]
    // Tests_SRS_MODULE_CONFIGURATION_BUILDER_24_008: [ It shall save `version` into member field.  ]
    // Tests_SRS_MODULE_CONFIGURATION_BUILDER_24_012: [ If version is negative it shall throw IllegalArgumentException. ]
    @Test(expected = IllegalArgumentException.class)
    public void buildShouldFailIfVersionIsNegative() {
        ModuleConfiguration.Builder builder = new ModuleConfiguration.Builder();
        builder.setIdentifier(IDENTIFIER).setModuleClass(MODULE_CLASS).setModuleVersion(INVALID_VERSION);

        builder.build();
    }
}

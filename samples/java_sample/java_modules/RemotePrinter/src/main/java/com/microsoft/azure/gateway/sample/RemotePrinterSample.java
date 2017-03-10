package com.microsoft.azure.gateway.sample;

import java.io.IOException;

import com.microsoft.azure.gateway.remote.ConnectionException;
import com.microsoft.azure.gateway.remote.ModuleConfiguration;
import com.microsoft.azure.gateway.remote.ProxyGateway;

public class RemotePrinterSample {
    public static void main(String[] args) {
        if (args.length < 1)
            throw new IllegalArgumentException("Please provide the control message identifier");

        byte version = 1;
        ModuleConfiguration.Builder configBuilder = new ModuleConfiguration.Builder();
        configBuilder.setIdentifier(args[0]);
        configBuilder.setModuleClass(Printer.class);
        configBuilder.setModuleVersion(version);

        ProxyGateway moduleProxy = new ProxyGateway(configBuilder.build());
        try {
            moduleProxy.attach();
        } catch (ConnectionException e) {
            e.printStackTrace();
        }   
        
        try {
            System.out.println("Press ENTER to stop");
            System.in.read();
        } catch (IOException e) {
            e.printStackTrace();
        }
        
        moduleProxy.detach();
    }
}

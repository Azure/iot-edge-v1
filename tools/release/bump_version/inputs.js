module.exports = [
    ///////////////////////////////////////////////////
    // Azure IoT Gateway SDK
    ///////////////////////////////////////////////////
    {
        "taskType": "regexReplaceTask",
        "filePath": "tools/docs/c/Doxyfile",
        "search": "(PROJECT\\_NUMBER)([ ]+\\= )(.*)",
        "replaceString": function(versions) {
            return '$1' + '$2' + versions.gateway.core;
        }
    },
    {
        "taskType": "regexReplaceTask",
        "filePath": "CMakeLists.txt",
        "search": "(set\\(GATEWAY\\_VERSION[\\s]+)(\\d+.\\d+.\\d+)([\\s]+.*)",
        "replaceString": function(versions){
            return '$1' + versions.gateway.core + '$3';
        }
    },
    ///////////////////////////////////////////////////
    // Azure IoT Gateway SDK - DotNET binding
    ///////////////////////////////////////////////////
    {
        "taskType": "regexReplaceTask",
        "filePath": "bindings/dotnet/dotnet-binding/E2ETestModule/Properties/AssemblyInfo.cs",
        "search": "(AssemblyInformationalVersion\\(\").*(\"\\)\\])",
        "replaceString": function(versions) {
            return '$1' + versions.bindings.dotnet + '$2';
        }
    },
    {
        "taskType": "regexReplaceTask",
        "filePath": "bindings/dotnet/dotnet-binding/Microsoft.Azure.IoT.Gateway/Properties/AssemblyInfo.cs",
        "search": "(AssemblyInformationalVersion\\(\").*(\"\\)\\])",
        "replaceString": function(versions) {
            return '$1' + versions.bindings.dotnet + '$2';
        }
    },
    {
        "taskType": "regexReplaceTask",
        "filePath": "bindings/dotnet/dotnet-binding/Microsoft.Azure.IoT.Gateway.Test/Properties/AssemblyInfo.cs",
        "search": "(AssemblyInformationalVersion\\(\").*(\"\\)\\])",
        "replaceString": function(versions) {
            return '$1' + versions.bindings.dotnet + '$2';
        }
    },    
    {
        "taskType": "regexReplaceTask",
        "filePath": "bindings/dotnet/dotnet-binding/PrinterModule/Properties/AssemblyInfo.cs",
        "search": "(AssemblyInformationalVersion\\(\").*(\"\\)\\])",
        "replaceString": function(versions) {
            return '$1' + versions.bindings.dotnet + '$2';
        }
    },
    {
        "taskType": "regexReplaceTask",
        "filePath": "bindings/dotnet/dotnet-binding/SensorModule/Properties/AssemblyInfo.cs",
        "search": "(AssemblyInformationalVersion\\(\").*(\"\\)\\])",
        "replaceString": function(versions) {
            return '$1' + versions.bindings.dotnet + '$2';
        }
    },
    {
        "taskType": "regexReplaceTask",
        "filePath": "tools/docs/dotnet/Doxyfile",
        "search": "(PROJECT\\_NUMBER)([ ]+\\= )(.*)",
        "replaceString": function(versions) {
            return '$1' + '$2' + versions.bindings.dotnet;
        }
    },
    ///////////////////////////////////////////////////
    // Azure IoT Gateway SDK - Java binding
    ///////////////////////////////////////////////////    
    {
        "taskType": "xmlReplaceTask",
        "filePath": "bindings/java/gateway-java-binding/pom.xml",
        "search": "//mavenns:project/mavenns:version",
        "nsmap": {"mavenns": "http://maven.apache.org/POM/4.0.0"},
        "replaceString": "bindings.java",
    },
    {
        "taskType": "xmlReplaceTask",
        "filePath": "samples/java_sample/java_modules/Printer/pom.xml",
        "search": "//mavenns:project/mavenns:version",
        "nsmap": {"mavenns": "http://maven.apache.org/POM/4.0.0"},
        "replaceString": "bindings.java"
    },
    {
        "taskType": "xmlReplaceTask",
        "filePath": "samples/java_sample/java_modules/Sensor/pom.xml",
        "search": "//mavenns:project/mavenns:version",
        "nsmap": {"mavenns": "http://maven.apache.org/POM/4.0.0"},
        "replaceString": "bindings.java"
    },
    {
        "taskType": "regexReplaceTask",
        "filePath": "samples/java_sample/src/java_sample_lin.json",
        "search": "\\d+\.\\d+\.\\d+",
        "replaceString": "bindings.java"
    },
    {
        "taskType": "regexReplaceTask",
        "filePath": "samples/java_sample/src/java_sample_win.json",
        "search": "\\d+\.\\d+\.\\d+",
        "replaceString": "bindings.java"
    }
];

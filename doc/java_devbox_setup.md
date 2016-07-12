# Prepare your development environment (Java)

This document describes how to prepare your development environment to use the *Microsoft Azure IoT Gateway SDK* for Java module development.

- [Java JDK SE](#installjava)
- [Maven 3](#installmaven)
- [Azure IoT Gateway SDK for Java](#installgw)
	- [Build from source](#installgwsource)
	- [Include using Maven](#installgwmaven)
- [Application Samples](#samplecode)

<a name="installjava"/>
## Install Java JDK SE
To use the SDK and run the samples you will need a **32-bit** installation of a **JDK** (it is insufficient to only have the JRE installed).

### Windows
For downloads and installation instructions go here: http://www.oracle.com/technetwork/java/javase/downloads/index.html

#### Set up environment variables
- Please make sure that the `JAVA_HOME` environment variable includes the full path to the `jdk1.x.x` directory. (Example: JAVA_HOME=C:\\Program Files\\Java\\jdk1.8.0_92)
- Please make sure that the `PATH` environment variable includes the full path to the `jdk1.x.x\jre\bin` directory. (Example: %JAVA_HOME%\\bin)
- Please make sure that the `PATH` environment variable includes the full path to the `jdk1.x.x\jre\bin\server` directory. (Example: %JAVA_HOME%\\jre\\bin\\server)

You can test whether your `PATH` variable is set correctly by restarting your console and running `java -version`.

### Linux
Depending on your Linux distribution, you can install the JDK using *apt-get*, alternatively, you may visit the link above to find the download and instructions for **Linux x86**.

#### Set up environment variables
- Please make sure that the `PATH` environment variable includes the full path to the bin folder containing java as well as the server directory containing the jvm.

	```
	which java
	echo $PATH
	```
	Ensure that the bin directory shown by the ```which java``` command matches one of the directories shown in your $PATH variable and that the server directory containing the jvm is present in your $PATH.
	If it is not:
	```
	export PATH=/path/to/java/bin:$PATH
    export PATH=/path/to/java/jre/lib/amd64/server:$PATH
	```

- Please make sure that the `JAVA_HOME` environment variable includes the full path to the jdk.

	```
	update-alternatives --config java
	```
	Take note of the jdk location. ```update-alternatives``` will show something similar to ***/usr/lib/jvm/java-8-openjdk-amd64/jre/bin/java***. The jdk directory would then be ***/usr/lib/jvm/java-8-openjdk-amd64/***.

	```
	export JAVA_HOME=/path/to/jdk
	```


<a name="installmaven"/>
## Install Maven
Using **_Maven 3_** is the recommended way to install the interfaces and classes for the Java binding mechanism for the **Microsoft Azure IoT Gateway SDK**.

### Windows
For downloads and installation instructions go here: https://maven.apache.org/download.cgi

#### Set up environment variables
- Please make sure that the `PATH` environment variable includes the full path to the `apache-maven-3.x.x\bin` directory. (Example: F:\\Setups\\apache-maven-3.3.3\\bin). The `apache-maven-3.x.x` directory is where Maven 3 is installed.

You can verify that the environment variables necessary to run **_Maven 3_** have been set correctly by restarting your console and running `mvn --version`.

### Linux
On Linux, Maven 3 can be installed as follows:

```
sudo apt-get update
sudo apt-get install maven
```

#### Set up environment variables

Please verify the following:

Ensure the `PATH` environment variable contains the full path to the bin folder containing **_Maven 3_**.

	```
	which mvn
	echo $PATH
	```
Ensure that the bin directory shown by the ```which mvn``` command matches one of the directories shown in your $PATH variable.
	If it does not:
	```
	export PATH=/path/to/mvn/bin:$PATH
	```

You can verify that the environment variables necessary to run **_Maven 3_** have been set correctly by running `mvn --version`.

<a name="installgw"/>
## Install Azure IoT Gateway SDK

- There are two ways to get the .jar library for the Java module bindings for the Microsoft Azure IoT Gateway SDK. You may either download the source code and build on your machine, or include the project as a dependency in your project if your project is a Maven project. Both methods are described below.

<a name="installgwsource">
### Build Java bindings for the Azure IoT Gateway SDK from sources
- Get a copy of the **Azure IoT Gateway SDK** from GitHub if you have not already done so. You should fetch a copy of the source from the **master** branch of the GitHub repository: <https://github.com/Azure/azure-iot-gateway-sdk>
- When you have obtained a copy of the source, you can build the bindings for Java.

Open a command prompt and use the following commands for the steps above:

```
	git clone https://github.com/Azure/azure-iot-gateway-sdk.git
	cd bindings/java/gateway-java-binding
	mvn clean install
```

The compiled JAR file with all dependencies bundled in can then be found at:

```
{IoT gateway SDK root}/bindings/java/gateway-java-binding/target/gateway-java-binding-{version}-with-deps.jar
```

When you're ready to build your own module in Java, include this JAR file in your project to get the interfaces and classes that you need.

<a name="installgwmaven">
### Get the Java bindings for the Azure IoT Gateway SDK from Maven (as a dependency)
_This is the recommended method of including the Azure IoT Gateway SDK in your project, however this method will only work if your project is a Maven project_

_For a guide on creating a maven project, see here: https://maven.apache.org/guides/getting-started/ _

- Navigate to http://search.maven.org, search for **com.microsoft.azure.gateway** and take note of the latest version number (or the version number of whichever version of the sdk you desire to use).

In your main pom.xml file, add the Azure IoT Gateway SDK Java binding as a dependency using your desired version as follows:
```
<dependency>
    <groupId>com.microsoft.azure.gateway</groupId>
    <artifactId>gateway-java-binding</artifactId>
    <version>1.0.0</version>
	<!--This is the current version number as of the writing of this document. Yours may be different.-->
</dependency>
```

<a name="samplecode">
## Sample applications

This repository contains a sample gateway that includes modules written in Java that illustrate how to write modules in Java and use the Microsoft Azure IoT Gateway SDK. For more information, see the [readme][readme].

To learn how to run a simple *Getting started* gateway that passes messages around, see [How-To (Java)][lnk-getstarted].

[readme]: ../README.md
[lnk-getstarted]: java_how_to.md

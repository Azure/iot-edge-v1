Creating the Azure IoT Edge Snap
===================================

A "snap" is a self-contained unit, that can be installed on any platform capable of running `snapd`, the Ubuntu Snap Daemon. Snaps can be created manually, but *should* be created using `snapcraft`, a tool used to programmatically generate snaps.

The following document will walk through creating the Azure IoT Edge snap package. Snap packages have capabilities far beyond those discussed in this document, but the components required to construct the Azure IoT Edge snap will be covered in detail.

> #### Vocabulary:
>
> ***Snap**: A self-contained package described by `snap.yaml`, that can be uploaded to the store and downloaded to any device capable of running the snap daemon, `snapd`.*
>
> ***Snapcraft**: The tool designed for programmatic construction of a snap package, including generating the related `snap.yaml` file. This tool is configured via a `snapcraft.yaml` file.*

To create a snap manually, you need to describe a snap using a `snap.yaml` file. Under normal (*almost all*) circumstances you will need/want to create a `snapcraft.yaml` file.

### `snapcraft.yaml` vs. `snap.yaml`

There are many similarities between a `snap.yaml` file and `snapcraft.yaml` file, because a `snapcraft.yaml` describes how to create a `snap.yaml` file. This is likely the greatest single point of confusion when authoring a snap package.

> *WARNING: When reading documentation, be sure to confirm you are referencing "snapcraft" documentation and syntax and not "snap" documentation and syntax.*

### Snapcraft Benefits

When authoring a snap, you should **ONLY** use the `snapcraft` tool (if at all possible). By using facilities available in `snapcraft`, you ensure your snap and **ALL** dependencies can be resolved programmatically. In fact, Canonical has enabled CI (continuous integration) via [launchpad](https://launchpad.net/), which will build your snap for all platforms*.

**visit [Snapcraft.io](https://snapcraft.io/docs/build-snaps/ci-integration) for details.*

Setting up the build environment
--------------------------------

If you have Ubuntu, then it's easy to install the tools required to author, update and publish snap packages.

```bash
$ sudo apt update && apt install -y snapd snapcraft
```

> If you are not using an Ubuntu image, then see [Ubuntu's documentation](https://snapcraft.io/docs/build-snaps/trusty) for installing the tools on other platforms.

Authoring the Azure IoT Edge Snap
------------------------------------

To begin, create a folder for the project, then call `snapcraft init` to create the basic snapcraft folder heirarchy and template.

```bash
$ mkdir aziot
$ cd aziot/
$ snapcraft init
```

This provides a `snap` folder, containing a basic template for a `snapcraft.yaml` file. All that is required to create a snap package, is to update the data in the template. Below, the template has been updated for the Azure IoT Edge snap, and now `snapcraft.yaml` looks like the following...

```yaml
name: azure-iot-gateway
version: '2017-04-12'
summary: Azure IoT Edge for tailored IoT solutions.
description: |
  Azure IoT Edge lets you build IoT solutions tailored
  to your exact scenario. Connect new or existing devices,
  regardless of protocol. Process the data in an on-premises
  gateway using the language of your choice (Java, C#, Node.js,
  or C), before sending it to the cloud. Deploy the solution on
  hardware that meets your performance needs and runs the
  operating system of your choice.

grade: stable
confinement: strict

apps:
  create-from-json:
    command: hello-world-sample
    plugs:
      - home

parts:
  iot-edge:
    source: https://github.com/azure/iot-edge.git
    source-tag: '2017-04-12'
    plugin: cmake
    build-packages:
      - build-essential
      - curl
      - libcurl4-openssl-dev
      - libglib2.0-dev
      - libssl-dev
      - libtool
    stage-packages:
      - libglib2.0-dev
      - libssl-dev
      - uuid-dev
    install: |
      cp samples/hello-world/hello-world-sample $SNAPCRAFT_PART_INSTALL/
```

> *NOTE: There are several more "stanzas" (not-shown) that can be used to make up a `snapcraft.yaml`. Here we only cover the stanzas required to produce the Azure IoT Edge snap.*
> 
> *To see the source visit the [Azure IoT launchpad.net repository](https://git.launchpad.net/azure-iot-gateway-snap/tree/snap/snapcraft.yaml)*
>
> *For more info check out: [Snapcraft.io: Build Snaps](https://snapcraft.io/docs/build-snaps/syntax)*

As you can see, there is not much to it. In fact, it's hard to believe so few lines can do so much (i.e. download source code and dependencies, build from source and package an application)!

### Breaking down `snapcraft.yaml`

Let's step through each stanza, line-by-line, describing how it affects the resulting snap package.

#### Snap details

```yaml
name: azure-iot-gateway
```

This is the name of your resulting snap package. It needs to be globally unique (i.e. it must be unique in the Ubuntu Store), if you wish to publish it.

```yaml
version: '2017-04-12'
```

The version is a human readable string for the consumer, it has *NOTHING* to do with actual versioning. Those versions begin at 1 and are monotomically incremented every time a snap package is generated (*note "generated" not "published"*).

```yaml
summary: Azure IoT Edge for tailored IoT solutions.
description: |
  Azure IoT Edge lets you build IoT solutions tailored
  to your exact scenario. Connect new or existing devices,
  regardless of protocol. Process the data in an on-premises
  gateway using the language of your choice (Java, C#, Node.js,
  or C), before sending it to the cloud. Deploy the solution on
  hardware that meets your performance needs and runs the
  operating system of your choice.
```

The summary and description of the package. These descriptions will be visible in the Ubuntu Store to let users know about your snap package.

```yaml
grade: stable
```

This defines the quality grade of the snap. It can be either `devel` (i.e. a development version of the snap, so not to be published to the `stable` or `candidate` channels) or `stable` (i.e. a stable release or release candidate, which can be released to all channels).*

> **taken directly from the [documentation](https://snapcraft.io/docs/build-snaps/syntax#Appsandcommands)*

```yaml
confinement: strict
```

The confinement describes the sandboxing rules. `classic` means "no sandbox" and the snap must be manually reviewed by the Ubuntu Store team. `strict` means full sandbox rules apply and the snap can be automatically published in the Ubuntu Store. Finally, `devmode` uses the same rules as `strict` but all security denials are turned into warnings and the snap cannot be published in the stable or candidate channels.

> *see also: [Confinement](https://snapcraft.io/docs/reference/confinement)*

#### Applications

```yaml
apps:
  create-from-json:
    command: hello-world-sample
    plugs:
      - home
```

Here the apps made available from the snap package. This package defines a single application, entitled `create-from-json`. Once the snap has been installed, `azure-iot-gateway.create-from-json` will be available to use from the command line.

Looking to the `command:` section, you can see that `create-from-json` is nothing more than an alias for the `hello-world-sample`. In case you are not familiar, `hello-world-sample` takes a JSON config file, which provides relative paths to any modules (dynamic libraries) it should load. As it turns out, `hello-world-sample` it a completely generic loader, and, other than its name, bears no reference to anything "hello world".

Any `plugs:` specified under the application will be passed on to the resulting [`snap.yaml`](https://snapcraft.io/docs/snaps/metadata#snap.yaml). These `plugs:` identify which [interfaces](https://snapcraft.io/docs/reference/interfaces) will be consumed by the application. In this example, the `home` interface has been supplied, which allows the snap package to access any files in the user's home directory.

Now to bring it all together. In order for the `create-from-json` application to work, both the JSON config file and the modules (dynamic libraries) to be loaded, must be located in (or in a child folder of) the user's home directory.

#### Components and Dependencies

```yaml
parts:
  iot-edge:
    source: https://github.com/azure/iot-edge.git
    source-tag: '2017-04-12'
    plugin: cmake
    build-packages:
      - build-essential
      - curl
      - libcurl4-openssl-dev
      - libglib2.0-dev
      - libssl-dev
      - libtool
    stage-packages:
      - libglib2.0-dev
      - libssl-dev
      - uuid-dev
    install: |
      cp samples/hello-world/hello-world-sample $SNAPCRAFT_PART_INSTALL/
```

> ***NOTE:** By simply including this "part" in your snap package, Azure IoT Edge will be downloaded, built and installed into your snap package.*

Now onto the `parts:` stanza of the `snapcraft.yaml`. The first line is an informal name. You can choose anything that helps you remember how this part interacts with your snap; this part is named "iot-edge".

Snapcraft has the ability to download code from a remote source, as specified by `source:`. As you can see above, snapcraft has been directed to look for the source code at the Azure GitHub repository. Futhermore, a particular tag has been designated for checkout via the `source-tag:` property.

Snapcraft has a large collection of plugins used to build projects from source. In this case, Azure IoT Edge is a CMake project, which is supported by Snapcraft. By setting the `plugin:` property to `cmake`, Snapcraft knows how to build this project.

The next two properties, `build-packages` and `stage-packages`, allow you to describe any `.deb` package dependencies. The `build-packages:` property identifies which `.deb` packages are required to "build" the project using. In other words, any packages listed as `build-packages` will be automatically downloaded for anyone trying to "create" this snap. `stage-packages:` calls out which `.deb` packages are required to satisfy the runtime dependencies, or `.deb` packages that need to be shipped* with the snap.

Lastly, consider the `install:` property, which is also known as a [scriptlet](https://snapcraft.io/docs/build-snaps/scriptlets). As its name indicates, this particular scriptlet is executed during the "install" (or snapping) step. In this scriptlet, the output of the build phase (i.e. the `hello-world-sample` program) is copied into the root directory of the snap, $SNAPCRAFT_PART_INSTALL. Now, it can be reached by the `command:` property listed earlier in the `apps:` stanza under the `create-from-json` application.

**.deb packages are not actually shipped within the snap, but are downloaded at the time of installation if necessary.*

Building the Snap
-----------------

There is a single command that will download, build and package your snap, `snapcraft`. In order to use it, you must be on a command line at the root folder of your project and you call `snapcraft`. It will automatically find the `snap/snapcraft.yaml` file and create the snap package.

```bash
$ snapcraft
...
Snapping 'azure-iot-gateway' -
Snapped azure-iot-gateway_2017-04-12_amd64.snap
```

There are five stages of building:
- pull
- build
- stage
- prime
- snap

You can request any step of snap creation by name, and it will walk through all prerequisite steps as well as the requested step.

```bash
$ snapcraft clean azure-iot-gateway -s build
Cleaning priming area for iot-edge
Cleaning staging area for iot-edge
Cleaning build for iot-edge
Cleaning up priming area
Cleaning up staging area
$ snapcraft build azure-iot-gateway
Skipping pull iot-edge
Copying needed target link from the system /lib/x86_64-linux-gnu/libz.so.1.2.8
Copying needed target link from the system /lib/x86_64-linux-gnu/libuuid.so.1.3.0
Copying needed target link from the system /lib/x86_64-linux-gnu/libpcre.so.3.13.2
Building iot-edge
...
Install the project...
...
```

Above is a common example of iterating on the build step, after the snap has been built previously.

The `snapcraft clean build` command moves through the steps backward cleaning until it reaches the requested step, `build`. Then `snapcraft build` moves forward from the beginning through the requested step.

#### Learning More

Obviously, it's easy to build a snap once it has been created properly. For a more in-depth look at building snaps, you should follow the [Ubuntu Tutorial](https://tutorials.ubuntu.com/tutorial/create-first-snap?utm_source=snapcraft.io&utm_medium=buildsnapsindex&utm_campaign=tutorials&_ga=1.266630734.1998447024.1493313581#0) designed to assist you in creating your first snap.

To understand the snap creation steps in greater detail, view [this StackOverflow post](http://stackoverflow.com/questions/43556433/snap-packages-what-is-the-purpose-of-the-staging-and-priming-steps), where an Ubuntu developer describes each step in detail.

Publishing to the Ubuntu Store
------------------------------

You can also publish your snap package using Snapcraft, but first you will need to create an account online at [ubuntu.com](https://login.ubuntu.com/). 

Once you have an account, you will login from your device.

```bash
$ snapcraft login
Enter your Ubuntu One SSO credentials.
Email: edgedevs@microsoft.com
Password: 
Login successful.
```

Then you will reserve the name of your snap. This will be the name consumers will use to search for and install your snap.

```bash
$ snapcraft register azure-iot-gateway
Registering azure-iot-gateway.
Congratulations! You're now the publisher for 'azure-iot-gateway'.
```

Once you believe you have finalized your `snapcraft.yaml`, it's always good to uninstall, delete, rebuild it from scratch and reinstall. In the case of `azure-iot-gateway` it looks like the following.

```bash
$ snap remove azure-iot-gateway
azure-iot-gateway removed
$ rm -rf parts/ prime/ stage/
$ rm azure-iot-gateway_2017-04-12_amd64.snap
$ snapcraft
...
Snapping 'azure-iot-gateway' -
Snapped azure-iot-gateway_2017-04-12_amd64.snap
$ snapcraft install azure-iot-gateway_2017-04-12_amd64.snap --dangerous
azure-iot-gateway 2017-04-12 installed
```

Now we will push our snap to the `edge` channel. We chose `edge` because we didn't want our snap to be searchable. However, this restriction is self-imposed, because we have marked this snap with `grade: stable` and `confinement: strict`.

```bash
$ snapcraft push azure-iot-gateway_2017-04-12_amd64.snap --release=edge
Uploading azure-iot-gateway_2017-04-12_amd64.snap.
Uploading azure-iot-gateway_2017-04-12_amd64.snap [================================] 100%
Ready to release!|
Revision 1 of 'azure-iot-gateway' created.
The edge channel is now open.

Channel    Version        Revision
---------  ---------      ----------
stable     -              -
candidate  -              -
beta       -              -
edge       '2017-04-17`   1
```

Now our snap package is available for download!

Downloading and Installing Azure IoT Edge
-----------------------------------------

```bash
$ snap install azure-iot-gateway --edge
```

That's all there is to it, it couldn't hardly be any easier...
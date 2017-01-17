# Dockerfile

## Overview
In `tools/` directory there is a Dockerfile provided that allows you to quickly setup linux environment
for the Gateway project.

## Building
To build the image you need to already have [Docker installed and running](https://www.docker.com/products/docker). You should step into `tools/`
directory and run:
```bash
docker build -t azure-iot-gateway .
```
After a few minutes a reusable image will be created ready to build the Gateway with java and nodejs
bindings working. There is no need to rebuild the image afterwards as each run will make it's own copy of it.

## Stepping into
If you want to step into the enviorment and work from there the simplest way is to run:
`docker run -ti --entrypoint bash azure-iot-gateway` which will create a writeable container from the
copy of the azure-iot-gateway image with bash running inside. 

In the interactive shell there will be no Gateway source so you need to clone it yourself, but NodeJS will be already
build and JDK environment set up.

## Tests
By default the entrypoint of the image is to run unit tests for the Gateway and bindings from the upstream
repository on `master` branch. Additionally tests are also checked for memory leaks with valgrind.
You can easily run the tests with `docker run --rm azure-iot-gateway`.

You can change which repository and branch you want to download and test the Gateway from by overriding
`GATEWAY_REPO` and `COMMIT_ID` environment variables with this command:
`docker run -e GATEWAY_REPO=http://my.repo.git -e COMMIT_ID=master azure-iot-gateway`

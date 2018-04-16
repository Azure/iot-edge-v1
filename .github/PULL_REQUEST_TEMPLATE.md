<!--
Thank you for helping us improve the Azure IoT Edge!

Pull requests are accepted by this project. Azure IoT Edge shares the same [contribution guidelines](https://github.com/Azure/azure-iot-sdk-c/blob/master/.github/CONTRIBUTING.md) as
the other Azure IoT SDKs. 
--> 

# Reference/Link to the issue solved with this PR (if any)

# Description of the problem
<!-- Please be as precise as possible: what issue you experienced, how often... -->

# Description of the solution
<!-- How you solved the issue and the other things you considered and maybe rejected --> 

## Notes on Unit Tests 

Before submitting the PR, it is **recommended** to run UTs(especially for bug fixes, feature additions etc. to existing code) 

Before running the UTs, make sure you have performed a recursive clone of the repository:
```
git clone --recursive https://github.com/Azure/iot-edge
```

# Running UTs in Linux
```
v1/tools/build.sh  --run-unittests
```

# Running UTs in Windows
```
v1\tools\build.cmd --run-unittests
```
The Azure IoT Gateway SDK shares the same [contribution guidelines](https://github.com/Azure/azure-iot-sdks/blob/master/CONTRIBUTING.md) as
the other Azure IoT SDKs. The gateway SDK team also monitors our [GitHub issues](https://github.com/Azure/azure-iot-gateway-sdk/issues)
section and Stack Overflow, especially the [azure-iot-gateway-sdk](http://stackoverflow.com/questions/tagged/azure-iot-gateway-sdk) tag.

----------

## Get your module featured in our README
Modules featured in our README need to meet certain requirements in the areas of documentation, automated tests, and samples. If your module meets these requirements and you want to get it featured in our README, just make the change to the README and submit a pull request! Here are the details:

### Documentation
Provide documentation for your module, including:
- Licensing terms
- How to contact someone about the module
- How to file a bug
- A description of the module's features
- How to install it
- How to use it

### Automated tests
If your module is available as open-source code, make automated tests available (e.g., published to the module's code repository, with instructions for building and running them) so anyone can confirm that the module behaves as described. The tests must:
- Verify domain-specific behavior (e.g. if it's a protocol translation module, does it successfully and accurately translate from protocol A to protocol B?)
- Validate that the module uses the gateway SDK correctly (Note: in the future we'd like to provide an acceptance suite to help you with this part)

If your module is _not_ available as open-source code, we still recommend automated tests but don't require them.

### Samples
We recommend that you provide one or more samples, in the form of code or documentation, that demonstrate how to configure and use your module in a simple scenario.

### Pull Request
Open a [Pull Request](https://github.com/Azure/azure-iot-gateway-sdk/compare) that adds a row about your module to the bottom of the Featured Modules table in our [README](https://github.com/Azure/azure-iot-gateway-sdk/blob/master/README.md), e.g.,

```
>| Name         | More information                | Targets gateway SDK version |
>|--------------|---------------------------------|-----------------------------|
>| ...          | ...                             | ...                         |
>| XYZ protocol | http://contoso.com/xyz_protocol | 2016-09-01                  |
```

#### "More information" field
This field in the table must contain a publicly accessible URL to at least one of the following resources:
- The root of your module's source code repository, if the source code is publicly available
- An appropriate "landing page" for your module otherwise

Your module's documentation must be discoverable from the page at this URL.

#### PR Comments

Add comments to the PR to describe how your module meets the requirements listed here. Also describe what steps you've taken to test your module against the gateway version referenced in the **Targets gateway SDK version** field.

**Note**: We reserve the right to remove a module's entry from the table if it hasn't been updated in a while.

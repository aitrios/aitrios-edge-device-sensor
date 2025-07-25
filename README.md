# Sensor Component for Edge Device Core

## Overview

The **Edge Device Core Sensor** is a sensor component of the [Edge Device Core](https://github.com/aitrios/aitrios-edge-device-core) project.
It provides the common APIs for accessing the sensor devices and interacting with the [System Application for Edge Device Core](https://github.com/aitrios/aitrios-edge-device-system-app) and [Edge Application SDK for AITRIOS](https://github.com/SonySemiconductorSolutions/aitrios-sdk-edge-app).

### Features

This component provides two key functionalities:
- Offers a **SensCord** package that integrates **SensCord Core** with **SensCord Component**, specifically designed for the Raspberry Pi AI Camera Kit.
- Enables deployment of AI models from System Application for Edge Device Core

### Supported Environments

-   **Raspberry Pi OS** with the [Raspberry Pi Camera Module](https://www.raspberrypi.com/documentation/accessories/ai-camera.html)

## How to Build

For detailed build instructions, refer to the [build documentation](docs/build.md).

## Directory Structure
```
.  
├── CODE_OF_CONDUCT.md                  # Community guidelines
├── CONTRIBUTING.md                     # Contribution guidelines
├── LICENSE                             # License file
├── PrivacyPolicy.md                    # Privacy policy
├── README.md                           # This file
├── SECURITY.md                         # Security policy and vulnerability reporting
├── docs/                               # Documentation
├── meson.build                         # Build configuration
├── meson_options.txt                   # Build options
├── script/                             # Build scripts
├── src/                                # Source code
├── subprojects/                        # Dependency configuration
└── subprojects-custom/                 # Custom dependency configuration
```

## Contribution

We welcome and encourage contributions to this project! We appreciate bug reports, feature requests, and any other form of community engagement.

-   **Issues and Pull Requests:** Please submit issues and pull requests for any bugs or feature enhancements.
-   **Contribution Guidelines:** For details on how to contribute, please see [CONTRIBUTING.md](CONTRIBUTING.md).
-   **Code of Conduct:** To ensure a welcoming and inclusive community, please review our [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

## Security

For information on reporting vulnerabilities and our security policy, please refer to the [SECURITY.md](SECURITY.md) file.

## License

This project is licensed under the Apache License 2.0. For more details, please see the [LICENSE](LICENSE) file.

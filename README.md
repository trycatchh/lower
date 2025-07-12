
# 🌐 Lower Web Framework
![Version](https://img.shields.io/badge/version-0.0.1-blue.svg)
![License](https://img.shields.io/github/license/trycatchh/lower?style=flat-square)
[![Issue](https://img.shields.io/github/issues/trycatchh/lower/good%20first%20issue?style=flat-square&color=%232EA043&label=Issue)](https://github.com/trycatchh/lower/labels/good%20first%20issue)

[📦 LowPM (Package Manager)](https://trycatch.network/) । [📚 Docs](https://trycatch.network/) । [👥 Community](https://trycatch.network/) । [🤝 Join Us](https://trycatch.network/)

- [What is Lower?](https://github.com/trycatchh/lower?tab=readme-ov-file#what-is-lower)
- [Getting Started & Integration](https://github.com/trycatchh/lower/blob/main/README.md#getting-started--integration)
- [For Example](https://github.com/trycatchh/lower?tab=readme-ov-file#for-example)
- [How can I contribute?](https://github.com/trycatchh/lower?tab=readme-ov-file#how-can-i-contribute)
- [License (MIT)](https://github.com/trycatchh/lower?tab=readme-ov-file#license)

## What is Lower?
Lower Framework is a lightweight, modular web framework written in C that speeds up development with its flexibility and high performance. It allows you to customize and extend modules easily to fit your needs. With [LowPM](https://trycatch.network), integrating external libraries and managing modules becomes simple and efficient, making your projects faster and more maintainable.

## Getting Started & Integration
Include basic modules for run
```shell
git clone https://github.com/trycatchh/lower.git
```
Add the library to your code file
```c
#include "lower/run.h" // Runtime module
```
To add libraries, please refer to the [documentation of the LowPM](https://trycatch.network/) repository.

## For Example
##### Create a handler
```c
void hello_handler(Request *req, Response *res) {
  res->status = 200;                          // HTTP status code
  res->content = "text/plain";                // Content-Type header
  res->body = "Hello from Lower Framework";   // Response body
}
```
##### Register the handler and run the server
```c
int main() {
  LW_Server *server = lw_run(8080);           // Create and start server on port 8080
  lw_route(server, "/hello", hello_handler);  // Register route and handler
  lw_start(server);                           // Start handling requests

  return 0;
}
```

## How can I contribute?
We welcome your contributions. If there is a software problem, do not hesitate to tell us and we will do our best.
We are always open to participation in our team. Please contact us on [Discord](https://discord.gg/mepa8X7j6w) or [Mail](mailto:p0unter@proton.me). We are always open to new ideas
- Create an [issue page](https://github.com/trycatchh/lower/issues) related to the problem.
- Request a suggestion: please submit your suggestion via the [issue page](https://github.com/trycatchh/lower/issues).
- Join us, [active team](https://github.com/trycatchh/lower/graphs/contributors).

## License
This project is licensed under the [MIT License](https://github.com/trycatchh/lower/blob/main/LICENSE), a permissive open-source license that lets you freely use, modify, and distribute the software. You must include the original copyright and license notice in any copies or substantial portions of the software.

The license provides the software "as is," without any warranty, protecting the authors from liability. Its simplicity and flexibility make it widely used and trusted in the open-source community.

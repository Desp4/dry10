# dry1
A Vulkan rendering engine in the works.
### Structure
dry1 consists of the main rendering engine `dry1` and an asset archiver `dab`. Neither require each other in the build process, but both are needed when building tests.
### Bulding
Requires CMake, just running it on the directory or adding as a subdirectory is sufficient. All dependencies are built along with dry1, one can supply library targets before including the project in which case user defined libraries will be used instead.  
Requires a compiler with some of C++20 features, tested on msvc(19.29, versions up to 19.23 should work fine too), gcc and clang support all the features used but have not been tested. Thanks to my laziness currently only a Windows build is supported.
### Usage?
All the relevant headers are in `src/engine`, tests may shed some light on how to actually use the thing.
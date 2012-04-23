Redis on Windows prototype
===
## What's new in this release

- Based on Redis 2.4.9
- Removed dependency on the pthreads library
- Improved the snapshotting (save on disk) algorithm. Implemented Copy-On-Write at the application level so snapshotting behavior is similar to the Linux version.

===
Special thanks to Dušan Majkic (https://github.com/dmajkic, https://github.com/dmajkic/redis/) for his project on GitHub that gave us the opportunity to quickly learn some on the intricacies of Redis code. His project also helped us to build our prototype quickly.

## How to build Redis using Visual Studio

You can use the free Express Edition available at http://www.microsoft.com/visualstudio/en-us/products/2010-editions/visual-cpp-express.

- The new code is on a separate branch so before compiling you need to switch to the new branch:
    git checkout bksavecow

- Open the solution file msvs\redisserver.sln in Visual Studio 10, and build.

    This should create the following executables in the msvs\$(Configuration) folder:

    - redis-server.exe
    - redis-benchmark.exe
    - redis-cli.exe
    - redis-check-dump.exe
    - redis-check-aof.exe


### Release Notes

This is a pre-release version of the software and is not yet fully tested.  
This is intended to be a 32bit release only. No work has been done in order to produce a 64bit version of Redis on Windows.
To run the test suite requires some manual work:

- The tests assume that the binaries are in the src folder, so you need to copy the binaries from the msvs folder to src. 
- The tests make use of TCL. This must be installed separately.
- To run the tests in a command window: `tclsh8.5.exe tests/test_helper.tcl`.

### Plan for the next release

- Improve test coverage
- Fix some performance issues on the Copy On Write code
- Add 64bit support


 
# ganglia-ibcounters

Ganglia metrics module that reads InfiniBand (mlx4/5) counters

## Build

A CMake build system is included with the package.  Several variables should be specified to enable CMake to find the Ganglia metrics build infrastructure, etc.

- `APR_ROOT_DIR`:  install prefix for apr-1 library
- `LIBCONFUSE_ROOT_DIR`:  install prefix for the Confuse configuration file library
- `GANGLIA_ROOT_DIR`:  install prefix for the Ganglia software
- `GANGLIA_BUILD_ROOT_DIR`:  path to the directory used to build Ganglia (for finding libmetrics infrastructure)

If `GANGLIA_BUILD_ROOT_DIR` is not provided it is inferred to be `${GANGLIA_ROOT_DIR}/src`.  So for a typical install we do in `/opt/shared/ganglia/<version>` with source in `/opt/shared/ganglia/<version>/src` and APR and Confuse present in the OS:

```
$ mkdir build
$ cd build
$ cmake -DGANGLIA_ROOT_DIR=/opt/shared/ganglia/3.7.2 ..
-- The C compiler identification is GNU 4.8.5
-- Check for working C compiler: /usr/bin/cc
-- Check for working C compiler: /usr/bin/cc -- works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Found APR: /usr/include/apr-1 (found version "1.4.8") 
-- Found libconfuse: /usr/include  
-- Found Ganglia: /opt/shared/ganglia/3.7.2/include  
-- Found GangliaMetric: /opt/shared/ganglia/3.7.2/src/libmetrics  
-- Configuring done
-- Generating done
-- Build files have been written to: /opt/shared/ganglia/add-ons/ganglia-ibcounters/build
```

The completed metrics module will be installed to the `lib/ganglia` or `lib64/ganglia` subdirectory of the `GANGLIA_ROOT_DIR`.

## Configuration

An example module configuration file, `ibcounters.conf`, is included.  We install it in `/etc/ganglia/conf.d`.

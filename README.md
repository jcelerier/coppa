![coppa 
logo](https://github.com/jcelerier/coppa/blob/master/logo_new.jpg) 

# Common Protocols for Parameters

The API is not yet stable. This aims to provide an example of a possible implementation of the OSCQuery protocol, in the most generic possible way to allowto easily adapt to specification changes.

## Requirements

* An up-to-date C++14 compiler (So far, only tested with clang-3.6; building 
works with GCC-5.1 but it crashes at runtime)
* Boost
* CMake

## Setup

    $ git clone https://github.com/jcelerier/coppa
    $ cd coppa
    $ git submodule update --init
    $ cd jeayeson
    $ ./configure

## Building

In a separate build folder : 

    $ cmake -DCMAKE_CXX_COMPILER=[at least clang-3.6] [coppa folder]
    $ make -j4

## Running a test server

    ./server_test # Server
    ./client_test # Client

# coppa
![coppa logo](https://github.com/jcelerier/coppa/blob/master/logo.png) Common Protocol for Parameters

## Requirements

* An up-to-date C++14 compiler (So far, only tested with clang-3.6)
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
    $ make

## Running a test server

    ./coppa_test # Server
    ./client_test # Client

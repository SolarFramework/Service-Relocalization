#!/bin/bash
#
# Copyright (c) 2021 B-com http://www.b-com.com/
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set -e

#VERBOSE=1
function log_verbose()
{
    if [ -v VERBOSE ]
    then
        echo -e "\e[33m$1\e[0m"
    fi
}

DEFAULT_PROTO_FILE=solar_mapping_and_relocalization_proxy.proto

log_verbose "Default proto file: "$DEFAULT_PROTO_FILE
log_verbose "cpp output dir: "$CPP_OUT_DIR
log_verbose "csharp output dir: "$CSHARP_OUT_DIR

if [[ $OSTYPE == *"linux"* ]]; then
    XPCF_MODULE_ROOT_OS="linux-gcc"
    PLUGIN_EXT=""
else
    # TODO: better OS detection   
    log_verbose "Warning: detected system is not Linux, assuming this is Windows"
    XPCF_MODULE_ROOT_OS="win-cl-14.1"
    PLUGIN_EXT=".exe"
fi

if [ "$XPCF_MODULE_ROOT" != "" ]
then
    log_verbose "XPCF_MODULE_ROOT env var is defined to $XPCF_MODULE_ROOT"
else
    DEFAULT_XPCF_MODULE_ROOT="${HOME}/.remaken/packages/$XPCF_MODULE_ROOT_OS"
    log_verbose "XPCF_MODULE_ROOT env var is not defined, will use default ($DEFAULT_XPCF_MODULE_ROOT)"
    XPCF_MODULE_ROOT="$DEFAULT_XPCF_MODULE_ROOT"
fi

if [ ! -d $XPCF_MODULE_ROOT ]
then
    echo "Error: target location for XPCF_MODULE_ROOT does not exist: '$XPCF_MODULE_ROOT'"
    echo "Run '`basename "$0"` -h' for help"
    exit 1
fi

DEFAULT_GRPC_INSTALL_DIR=$XPCF_MODULE_ROOT/grpc/1.37.1

function help()
{
    echo
    echo "Generate gRPC stubs and services for SolARDeviceGrpcService component"
    echo
    echo "By default uses grpc 1.37.1 found in your XPCF_MODULE_ROOT directory"
    echo "(if XPCF_MODULE_ROOT is not defined, defaults to ~/.remaken/packages)."
    echo
    echo "You can specify another installation root directory with -i/--grpc-install-dir."
    echo
    echo "The script will find the bin/ and lib/ directories according to the type"
    echo "of installation which can be set with -t/--install-type."
    echo
    echo "By default, remaken file hierarchy is used, which means lib and bin"
    echo "directories will be searched in:"
    echo "'<install-dir>/[ bin | dir ]/x86_64/[ static | shared ]/release'"
    echo
    echo "If 'standard' is selected, bin and lib directories will only be searched in:"
    echo "'<install-dir>/[ bin | dir ]'"
    echo "which is the case correspond to a standard CMake build."
    echo
    usage
}

function usage()
{
    echo "Usage:"
    echo "[VERBOSE=1] `basename "$0"` [<option>=<value> ]+"
    echo "Options:"
    echo "   -p, --proto: override default proto file"
    echo "   -i, --grpc-install-dir: path to grpc install directory (default: $XPCF_MODULE_ROOT/grpc/1.37.1)"
    echo "   -t, --install-type: [ standard | remaken-shared | remaken-static ] (default: remaken-shared)"
 }

# Parse args
while [ "$1" != "" ]; do
    PARAM=`echo $1 | awk -F= '{print $1}'`
    VALUE=`echo $1 | awk -F= '{print $2}'`
    case $PARAM in
        -h | --help)
            help
            exit
            ;;
        -i | --grpc-install-dir)
            USER_GRPC_INSTALL_DIR=$VALUE
            ;;
         -t | --install-type)
            USER_GRPC_INSTALL_TYPE=$VALUE
            ;;
         -p | --proto)
            USER_PROTO_FILE=$VALUE
            ;;
        *)
            echo "ERROR: unknown parameter '$PARAM'"
            usage
            exit 1
            ;;
    esac
    shift
done

PROTO_FILE=$DEFAULT_PROTO_FILE
if [ "$USER_PROTO_FILE" != "" ]
then
    PROTO_FILE=$USER_PROTO_FILE
fi

GEN_OUT_DIR=$PROTO_FILE-gen
CPP_OUT_DIR=$GEN_OUT_DIR/cpp
CSHARP_OUT_DIR=$GEN_OUT_DIR/csharp

GRPC_INSTALL_DIR=$DEFAULT_GRPC_INSTALL_DIR
if [ "$USER_GRPC_INSTALL_DIR" != "" ]
then
    GRPC_INSTALL_DIR=$USER_GRPC_INSTALL_DIR
fi
log_verbose "GRPC_INSTALL_DIR=$GRPC_INSTALL_DIR"

GRPC_INSTALL_TYPE="remaken-shared"
if [ "$USER_GRPC_INSTALL_TYPE" != "" ]
then
    GRPC_INSTALL_TYPE=$USER_GRPC_INSTALL_TYPE
fi

if [ "$GRPC_INSTALL_TYPE" == "standard" ]
then
    GRPC_BIN_DIR=$GRPC_INSTALL_DIR/bin
    GRPC_LIB_DIR=$GRPC_INSTALL_DIR/lib
elif [ "$GRPC_INSTALL_TYPE" == "remaken-shared" ]
then
    GRPC_BIN_DIR=$GRPC_INSTALL_DIR/bin/x86_64/shared/release
    GRPC_LIB_DIR=$GRPC_INSTALL_DIR/lib/x86_64/shared/release
elif [ "$GRPC_INSTALL_TYPE" == "remaken-static" ]
then
    GRPC_BIN_DIR=$GRPC_INSTALL_DIR/bin/x86_64/static/release
    GRPC_LIB_DIR=$GRPC_INSTALL_DIR/lib/x86_64/static/release
else
    echo "ERROR: unknown install type: '$GRPC_INSTALL_TYPE'"
    usage
    exit 1
fi

log_verbose "PROTO_FILE=$USER_PROTO_FILE"
log_verbose "GRPC_INSTALL_TYPE=$GRPC_INSTALL_TYPE"
log_verbose "GRPC_BIN_DIR=$GRPC_BIN_DIR"
log_verbose "GRPC_LIB_DIR=$GRPC_LIB_DIR"

if [ ! -d "$GRPC_INSTALL_DIR" ] 
then
    echo "Specified gRPC installation dir $GRPC_INSTALL_DIR does not exist." 
    exit 1
fi

if [ ! -d "$GRPC_BIN_DIR" ] 
then
    echo "Specified gRPC bin dir $GRPC_BIN_DIR does not exist." 
    exit 1
fi

if [ ! -d "$GRPC_LIB_DIR" ] 
then
    echo "Specified gRPC lib dir $GRPC_LIB_DIR does not exist." 
    exit 1
fi

log_verbose "Delete and create dirs for generated files..."
rm -rf $GEN_OUT_DIR
mkdir -p $CPP_OUT_DIR
mkdir -p $CSHARP_OUT_DIR
log_verbose "Done"

log_verbose "Generating services and stubs..."
LD_LIBRARY_PATH=$GRPC_LIB_DIR $GRPC_BIN_DIR/protoc -I ./  --csharp_out=$CSHARP_OUT_DIR $PROTO_FILE --grpc_out=$CSHARP_OUT_DIR --plugin=protoc-gen-grpc=$GRPC_BIN_DIR/grpc_csharp_plugin$PLUGIN_EXT
LD_LIBRARY_PATH=$GRPC_LIB_DIR $GRPC_BIN_DIR/protoc -I ./  --cpp_out=$CPP_OUT_DIR $PROTO_FILE --grpc_out=$CPP_OUT_DIR --plugin=protoc-gen-grpc=$GRPC_BIN_DIR/grpc_cpp_plugin$PLUGIN_EXT
log_verbose "Done"
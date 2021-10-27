#!/bin/bash
#  Copyright 2018-present Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
#  Licensed under the Apache License, Version 2.0 (the "License").
#  You may not use this file except in compliance with the License.
#  A copy of the License is located at
#
#   http://aws.amazon.com/apache2.0
#
#  or in the "license" file accompanying this file. This file is distributed
#  on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
#  express or implied. See the License for the specific language governing
#  permissions and limitations under the License.

# Modified by Josef Cullhed 2021

set -euo pipefail

print_help() {
    echo -e "Usage: packager [OPTIONS] <binary name>\n"
    echo -e "OPTIONS\n"
    echo -e "\t-d,--default-libc\t Use the target host libc libraries. This will not package the C library files.\n"
}

if [ $# -lt 1 ]; then
    echo -e "Error: missing arguments\n"
    print_help
    exit 1
fi

POSITIONAL=()
INCLUDE_LIBC=true
while [[ $# -gt 0 ]]
do
    key="$1"
    case $key in
        -d|--default-libc)
            INCLUDE_LIBC=false
            shift # past argument
            ;;
        *)    # unknown option
            POSITIONAL+=("$1") # save it in an array for later
            shift # past argument
            ;;
    esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

PKG_BIN_PATH=$1
architecture=$(arch)

if [ ! -d "$PKG_BIN_PATH" ]; then
    echo "$PKG_BIN_PATH" - No such directory.;
    exit 1;
fi

if ! type zip > /dev/null 2>&1; then
    echo "zip utility is not found. Please install it and re-run this script"
    exit 1
fi
function package_libc_via_pacman {
    if grep --extended-regexp "Arch Linux|Manjaro Linux" < /etc/os-release > /dev/null 2>&1; then
        if type pacman > /dev/null 2>&1; then
            pacman --query --list --quiet glibc | sed -E '/\.so$|\.so\.[0-9]+$/!d'
        fi
    fi
}

function package_libc_via_dpkg() {
    if type dpkg-query > /dev/null 2>&1; then
        if [[ $(dpkg-query --listfiles libc6 | wc -l) -gt 0 ]]; then
            dpkg-query --listfiles libc6 | sed -E '/\.so$|\.so\.[0-9]+$/!d'
        fi
    fi
}

function package_libc_via_rpm() {
    if type rpm > /dev/null 2>&1; then
       if [[ $(rpm --query --list glibc.$architecture | wc -l) -gt 1 ]]; then
           rpm --query --list glibc.$architecture | sed -E '/\.so$|\.so\.[0-9]+$/!d'
       fi
    fi
}

# hasElement expects an element and an array parameter
# it's equivalent to array.contains(element)
# e.g. hasElement "needle" ${haystack[@]}
function hasElement() {
    local el key=$1
    shift
    for el in "$@"
    do
        [[ "$el" == "$key" ]] && return 0
    done
    return 1
}

PKG_BIN_FILENAME=alexandria
PKG_DIR=tmp
PKG_LD=""

list=$(ldd "$PKG_BIN_PATH/server" | awk '{print $(NF-1)}')
libc_libs=()
libc_libs+=($(package_libc_via_dpkg))
libc_libs+=($(package_libc_via_rpm))
libc_libs+=($(package_libc_via_pacman))

mkdir -p "$PKG_DIR/bin" "$PKG_DIR/lib"

for i in $list
do
    if [[ ! -f $i ]]; then # ignore linux-vdso.so.1
        continue
    fi

    # Do not copy libc files which are directly linked unless it's the dynamic loader
    if hasElement "$i" "${libc_libs[@]}"; then
        filename=$(basename "$i")
        if [[ -z "${filename##ld-*}" ]]; then
            PKG_LD=$filename # Use this file as the loader
            cp "$i" "$PKG_DIR/lib"
        fi
        continue
    fi

    cp "$i" $PKG_DIR/lib
done

if [[ $INCLUDE_LIBC == true ]]; then
    for i in "${libc_libs[@]}"
    do
        filename=$(basename "$i")
        if [[ -z "${filename##ld-*}" ]]; then
            # if the loader is empty, then the binary is probably linked to a symlink of the loader. The symlink will
            # not show up when quering the package manager for libc files. So, in this case, we want to copy the loader
            if [[ -z "$PKG_LD" ]]; then 
                PKG_LD=$filename
                cp "$i" "$PKG_DIR/lib" # we want to follow the symlink (default behavior)
            fi
            continue # We don't want the dynamic loader's symlink because its target is an absolute path (/lib/ld-*).
        fi
        cp --no-dereference "$i" "$PKG_DIR/lib"
    done
fi

if [[ -z "$PKG_LD" ]]; then
    echo "Failed to identify, locate or package the loader. Please file an issue on Github!" 1>&2
    exit 1
fi

bootstrap_script_server=$(cat <<EOF
#!/bin/bash
set -euo pipefail
ALEXANDRIA_LIVE=1 ./lib/$PKG_LD --library-path ./lib ./bin/server
EOF
)

bootstrap_script_indexer=$(cat <<EOF
#!/bin/bash
set -euo pipefail
ALEXANDRIA_LIVE=1 ./lib/$PKG_LD --library-path ./lib ./bin/indexer
EOF
)

cp "$PKG_BIN_PATH/server" "$PKG_DIR/bin"
cp "$PKG_BIN_PATH/cc_indexer" "$PKG_DIR/bin"
echo -e "$bootstrap_script_server" > "$PKG_DIR/server"
echo -e "$bootstrap_script_indexer" > "$PKG_DIR/indexer"
chmod +x "$PKG_DIR/server"
chmod +x "$PKG_DIR/indexer"
# some shenanigans to create the right layout in the zip file without extraneous directories
pushd "$PKG_DIR" > /dev/null
zip --symlinks --recurse-paths "$PKG_BIN_FILENAME".zip -- *
ORIGIN_DIR=$(dirs -l +1)
mv "$PKG_BIN_FILENAME".zip "$ORIGIN_DIR"
popd > /dev/null
rm -r "$PKG_DIR"
echo Created "$ORIGIN_DIR/$PKG_BIN_FILENAME".zip


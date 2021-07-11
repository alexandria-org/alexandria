#!/bin/bash

cd `dirname $0`
cd ../build
spawn-fcgi -p 8000 -n server


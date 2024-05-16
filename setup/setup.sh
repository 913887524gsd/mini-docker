#!/bin/bash

mkdir -p /etc/mini-docker/

rm -rf /etc/mini-docker/images
mkdir -p /etc/mini-docker/images
tar xzf centos.tar.gz --directory=/etc/mini-docker/images

#!/bin/sh
sudo cp -r x86_adapt* /usr/src
sudo dkms add -m x86_adapt_defs -v 0.1
sudo dkms add -m x86_adapt_driver -v 0.3

sudo dkms build -m x86_adapt_defs -v 0.1
sudo dkms build -m x86_adapt_driver -v 0.3


sudo dkms install -m x86_adapt_defs -v 0.1
sudo dkms install -m x86_adapt_driver -v 0.3

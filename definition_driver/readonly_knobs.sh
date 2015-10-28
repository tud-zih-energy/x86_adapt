#!/bin/bash

mv knobs knobs_orig && mkdir knobs && cp $(grep readonly knobs_orig/ -r | cut -d ':' -f 1) knobs/

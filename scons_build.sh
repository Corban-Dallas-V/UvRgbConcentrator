#!/usr/bin/env sh

scons build profile=debug size bin 

arm-none-eabi-objcopy -O ihex ../build/UvRgbConcentrator/debug/UvRgbConcentrator.elf ../build/UvRgbConcentrator/debug/UvRgbConcentrator.hex
arm-none-eabi-objdump -dC ../build/UvRgbConcentrator/debug/UvRgbConcentrator.elf > ../build/UvRgbConcentrator/debug/UvRgbConcentrator.lss
# Copyright (c) 2017-2018, Niklas Hauser
#
# This file is part of the modm project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

#!/usr/bin/env python3

import os
from os.path import join, abspath

# User Configurable Options
project_name = "UvRgbConcentrator"
build_path = "../build/UvRgbConcentrator"
generated_paths = ['modm']
# SCons environment with all tools
env = DefaultEnvironment(tools=[], ENV=os.environ)
env["CONFIG_BUILD_BASE"] = abspath(build_path)
env["CONFIG_PROJECT_NAME"] = project_name

# Building all libraries
env.SConscript(dirs=generated_paths, exports="env")

env.Append(CPPPATH=".")
ignored = [".lbuild_cache", build_path] + generated_paths
sources = []
# Finding application sources
sources += env.FindSourceFiles(".", ignorePaths=ignored)
# So you want to add or remove compile options?
#   0. Check what options you want to add to GCC:
#      https://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html
#   1. Check what the environment variables are called in SCons:
#      https://www.scons.org/doc/latest/HTML/scons-user/apa.html
#   2. You can append one or multiple options like this
#       env.Append(CCFLAGS="-pedantic")
#       env.Append(CCFLAGS=["-pedantic", "-pedantic-errors"])
#   3. If you need to remove options, you need to do this:
#       env["CCFLAGS"].remove("-pedantic")
#      Note that a lot of options also have a "-no-{option}" option
#      that may overwrite previous options.
#   4. Add your environment changes below these instructions
#      inside this SConstruct file.
#   5. COMMIT THIS SCONSTRUCT FILE NOW!
#   6. Tell lbuild not to overwrite this SConstruct file anymore:
#       <option name="modm:build:scons:include_sconstruct">False</option>
#   7. Anyone using your project now also benefits from your environment changes.

env.BuildTarget(sources)
#===----------------------------------------------------------------------===#
#
# Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
# See /LICENSE for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===#

import os

import lit.formats

config.name = "Eter"
config.test_format = lit.formats.ShTest(True)
config.suffixes = [".mlir", ".smoke"]
config.excludes = ["CMakeLists.txt", "lit.cfg.py", "lit.site.cfg.py.in", "lit.local.cfg"]
config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = os.path.join(config.eter_obj_root, "test")

# Tool substitutions (LLVM style)
# Add more tools here as the project grows
config.substitutions.append(("%eter", '"{}"'.format(config.eter_executable)))
config.substitutions.append(
    ("%FileCheck", '"{}"'.format(config.filecheck_executable))
)

# Add future tools here as needed:
# config.substitutions.append(("%eter-opt", '"{}"'.format(config.eter_opt_executable)))
# config.substitutions.append(("%eter-translate", '"{}"'.format(config.eter_translate_executable)))

config.environment["PATH"] = os.pathsep.join(
    [
        os.path.dirname(config.eter_executable),
        os.path.dirname(config.filecheck_executable),
        config.environment.get("PATH", ""),
    ]
)

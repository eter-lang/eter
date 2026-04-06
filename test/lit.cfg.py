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
config.excludes = ["CMakeLists.txt", "lit.cfg.py", "lit.site.cfg.py.in"]
config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = os.path.join(config.eter_obj_root, "test")

config.substitutions.append(("%eter", '"{}"'.format(config.eter_executable)))
config.substitutions.append(
    ("%FileCheck", '"{}"'.format(config.filecheck_executable))
)

config.environment["PATH"] = os.pathsep.join(
    [
        os.path.dirname(config.eter_executable),
        os.path.dirname(config.filecheck_executable),
        config.environment.get("PATH", ""),
    ]
)

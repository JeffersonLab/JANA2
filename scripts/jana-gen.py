#!/usr/bin/env python3
from sys import argv

copyright_notice = """//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//"""

jobject_template_h = """{copyright_notice}

This is a test {name}

"""

jeventsource_template_h = """{copyright_notice}

This is a test {name}

"""


def create_jobject(name):
    filename = name + ".h"
    text = jobject_template_h.format(copyright_notice=copyright_notice, name=name)
    with open(filename, 'w') as f:
        f.write(text)


def create_jeventsource(name):
    hfilename = name + ".h"
    text = jeventsource_template_h.format(copyright_notice=copyright_notice, name=name)
    with open(hfilename, 'w') as f:
        f.write(text)


def create_jeventprocessor(name):
    pass


def create_jfactory(name):
    pass


def create_plugin(name):
    pass


def create_executable(name):
    pass


def create_project(name):
    pass


def print_usage():
    print("Usage: jana-gen [type] [name]")
    print("  type: jobject jeventsource jeventprocessor jfactory plugin executable project")
    print("  name: camel or snake case")


if __name__ == '__main__':

    if len(argv) < 3:
        print("Error: Wrong number of arguments!")
        print_usage()
        exit()

    dispatch_table = {'jobject': create_jobject,
                      'jeventsource': create_jeventsource,
                      'jeventprocessor': create_jeventprocessor,
                      'jfactory': create_jfactory,
                      'plugin': create_plugin,
                      'executable': create_executable,
                      'project': create_project
                      }

    option = argv[1]
    name = argv[2]
    if option in dispatch_table:
        dispatch_table[option](name)
    else:
        print("Error: Invalid type!")
        print_usage()
        exit()



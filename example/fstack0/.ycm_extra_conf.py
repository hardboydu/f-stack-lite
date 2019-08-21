import os

flags = [
        '-Wall',
        '-Wno-unused-parameter',
        '-Wno-unused-but-set-variable',
        '-Wextra',
        '-Werror',
        '-msse4.2',
        '-I/usr/include',
        '-I/opt/lib64/dpdk-18.11.1/usr/local/include',
        '-I/opt/lib64/dpdk-18.11.1/usr/local/include/dpdk',
        '-I/opt/lib64/f-stack/include/',
        ]

def FlagsForFile( filename, **kwargs ):
  return {
    'flags': flags
  }


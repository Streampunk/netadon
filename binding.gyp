{
  "targets": [
    {
      "target_name": "netadon",
      "sources": [ "src/netadon.cc", 
                   "src/UdpPort.cc" ],
      "include_dirs": [ "<!(node -e \"require('nan')\")" ],
      'conditions': [
        ['OS=="linux"', {
          "cflags_cc!": [ 
            "-fno-rtti",
            "-fno-exceptions"
          ],
          "cflags_cc": [
            "-std=c++11",
            "-fexceptions"
          ],
        }],
        ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_RTTI': 'YES',
            'MACOSX_DEPLOYMENT_TARGET': '10.7',
            'OTHER_CPLUSPLUSFLAGS': [
              '-std=c++11',
              '-stdlib=libc++',
              '-fexceptions'
            ]
          },
        }],
        ['OS=="win"', {
          "sources": [ "src/RioNetwork.cc" ],
          "configurations": {
            "Release": {
              "msvs_settings": {
                "VCCLCompilerTool": {
                  "RuntimeTypeInfo": "true",
                  "ExceptionHandling": 1
                }
              }
            }
          },
          "libraries": [ 
            "-lMswsock.lib",
            "-lWs2_32.lib",
          ],
        }]
      ],
    }
  ]
}

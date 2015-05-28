#
# Copyright 2010-2013 Fabric Engine Inc. All rights reserved.
#

import os, sys, platform, copy

Import(
  'parentEnv',
  'FABRIC_DIR',
  'FABRIC_SPLICE_VERSION',
  'STAGE_DIR',
  'FABRIC_BUILD_OS',
  'FABRIC_BUILD_TYPE',
  'SOFTIMAGE_INCLUDE_DIR',
  'SOFTIMAGE_LIB_DIR',
  'SOFTIMAGE_VERSION',
  'sharedCapiFlags',
  'spliceFlags'
  )

env = parentEnv.Clone()

softimageFlags = {
  'CPPPATH': [
      SOFTIMAGE_INCLUDE_DIR
    ],
  'LIBPATH': [
    SOFTIMAGE_LIB_DIR,
  ],
}

softimageFlags['LIBS'] = ['sicoresdk', 'sicppsdk']
if FABRIC_BUILD_OS == 'Windows':
  softimageFlags['CCFLAGS'] = ['/DNT_PLUGIN']
elif FABRIC_BUILD_OS == 'Linux':
  softimageFlags['CCFLAGS'] = ['-DLINUX']

env.MergeFlags(softimageFlags)
env.Append(CPPDEFINES = ["_SPLICE_SOFTIMAGE_VERSION="+str(SOFTIMAGE_VERSION[:4])])



env.Append(CPPPATH = [os.path.join(os.environ['FABRIC_UI_DIR'], 'stage', 'include')])
env.Append(CPPPATH = [os.path.join(os.environ['FABRIC_UI_DIR'], 'stage', 'include', 'FabricUI')])
env.Append(LIBPATH = [os.path.join(os.environ['FABRIC_DIR'], 'lib')])
env.Append(LIBPATH = [os.path.join(os.environ['FABRIC_UI_DIR'], 'stage', 'lib')])
env.Append(CPPPATH = [os.path.join(os.environ['FABRIC_DIR'], 'include')])
env.Append(CPPPATH = [os.path.join(os.environ['FABRIC_DIR'], 'include', 'FabricServices')])



env.MergeFlags(sharedCapiFlags)
env.MergeFlags(spliceFlags)

if FABRIC_BUILD_OS == 'Linux':
  env.Append(LIBS=['boost_filesystem', 'boost_system'])
  exportsFile = env.File('Linux.exports').srcnode()
  env.Append(SHLINKFLAGS = ['-Wl,--version-script='+str(exportsFile)])
elif FABRIC_BUILD_OS == 'Windows':
  env.Append(LIBS = ['OpenGL32.lib'])




#env.Append(LIBS = ['FabricSplice-2.0', 'FabricCore-2.0'])
if platform.system().lower().startswith('win'):
  env.Append(LIBS = ['FabricServices-MSVC-12.0-mt'])
else:
  env.Append(LIBS = ['FabricServices'])
env.Append(LIBS = ['FabricSplitSearch'])





target = 'FabricSpliceSoftimage'

softimageModule = env.SharedLibrary(target = target, source = Glob('*.cpp'), SHLIBPREFIX='')

if FABRIC_BUILD_OS == 'Linux':
  env.Depends(softimageModule, exportsFile)

softimageFiles = []

installedModule = env.Install(os.path.join(STAGE_DIR.abspath, 'Application', 'Plugins'), softimageModule)
softimageFiles.append(installedModule)

softimageFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'Application', 'UI'), os.path.join('UI', 'FE_logo.bmp')))

softimageFiles.append(env.Install(STAGE_DIR, env.File('license.txt')))

# also install the FabricCore dynamic library
if FABRIC_BUILD_OS == 'Linux':
  env.Append(LINKFLAGS = [Literal('-Wl,-rpath,$ORIGIN/../../../../lib/')])
if FABRIC_BUILD_OS == 'Windows':
  FABRIC_CORE_VERSION = FABRIC_SPLICE_VERSION.rpartition('.')[0]
  softimageFiles.append(
    env.Install(
      os.path.join(STAGE_DIR.abspath, 'Application', 'Plugins'),
      env.Glob(os.path.join(FABRIC_DIR, 'bin', '*.dll'))
      )
    )
  softimageFiles.append(
    env.Install(
      os.path.join(STAGE_DIR.abspath, 'Application', 'Plugins'),
      os.path.join(FABRIC_DIR, 'bin', 'FabricCore-' + FABRIC_CORE_VERSION + '.pdb')
      )
    )

softimageFiles.append(
  env.Install(
    os.path.join(STAGE_DIR.abspath, 'Application', 'Plugins'),
    env.File('FabricSplice_Python.py')
    )
  )


# todo: install the python client

alias = env.Alias('splicesoftimage', softimageFiles)
spliceData = (alias, softimageFiles)
Return('spliceData')

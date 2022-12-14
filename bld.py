if __name__ == '__main__':
    import sys, shutil, subprocess
    sys.argv[0:1] = [sys.executable, shutil.which("scons"), "-f", __file__]
    sys.exit(subprocess.run(sys.argv).returncode)

################################################################################

import msvc_env
cfg = msvc_env.BuildCfg(
    subsystem="console",
    ver=msvc_env.VC9,
    arch=msvc_env.X86,
    )
env = msvc_env.MsvcEnvironment(cfg)
env.set_build_dir('src', 'build')
env.Append(CCFLAGS=['/Isrc'])
objs = env.Object(source=["tatdylf.cpp", "tatdylf_ui.cpp"])
res = env.RES("tatdylf.rc")
libs = ["kernel32.lib", "ws2_32.lib", "user32.lib", "shell32.lib"]
exe = env.Program('tatdylf.exe', objs + res, LIBS=libs)

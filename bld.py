if __name__ == '__main__':
    import sys, subprocess
    sys.argv[0:1] = [sys.executable, "-m", "SCons", "-f", __file__]
    sys.exit(subprocess.run(sys.argv).returncode)

################################################################################

import scons_msvc_env as msvc_env
cfg = msvc_env.BuildCfg(
    subsystem="console",
    ver=msvc_env.VC9,
    arch=msvc_env.X86,
    )
env = msvc_env.MsvcEnvironment(cfg)
env.set_build_dir('src', 'build')
env.Append(CPPPATH=["."])
objs = env.Object(source=["tatdylf.cpp", "tatdylf_ui.cpp"])
res = env.RES("tatdylf.rc")
libs = ["kernel32.lib", "ws2_32.lib", "user32.lib", "shell32.lib"]
exe = env.Program('tatdylf.exe', objs + res, LIBS=libs)

if env.cfg.arch == msvc_env.X64:
    sexe = env.Squab(None, exe)
    env.Default(sexe)
else:
    env.Default(exe)

env.install_relative_to_parent_dir("bin", [exe, "../../tatdylf.ini"])

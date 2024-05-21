from setuptools import Extension, setup
import sysconfig

sysconfig.os.environ["LDSHARED"] = sysconfig.get_config_var("LDSHARED").replace("-fno-openmp-implicit-rpath", "")

setup(
    ext_modules=[
        Extension(
            name="chessapi",
            sources=["chessapi.c", "libchess.c"],
        )
    ]
)

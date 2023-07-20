from setuptools import Extension, setup

setup(
    ext_modules=[
        Extension(
            name="core",
            sources=["pywrk/core.c"],
        ),
    ]
)

from setuptools import setup, find_packages
import pathlib

# Read README for long description
here = pathlib.Path(__file__).parent.resolve()
long_description = (here / "README.md").read_text(encoding="utf-8")

setup(
    name="arcfour-crypto",
    version="1.0.0",
    description="Enhanced RC4 stream cipher library with static memory allocation support",
    long_description=long_description,
    long_description_content_type="text/markdown",
    author="harmonium-HSP",
    author_email="harmonium-hsp@users.noreply.github.com",
    url="https://github.com/harmonium-HSP/arcfour",
    packages=find_packages(),
    include_package_data=True,
    install_requires=[
        "cffi>=1.15.0"
    ],
    setup_requires=[
        "cffi>=1.15.0"
    ],
    cffi_modules=[
        "arcfour_cffi.py:ffi"
    ],
    zip_safe=False,
    classifiers=[
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Topic :: Security :: Cryptography",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
        "Intended Audience :: Developers",
    ],
    keywords="cryptography, rc4, stream-cipher, encryption",
    project_urls={
        "Bug Reports": "https://github.com/harmonium-HSP/arcfour/issues",
        "Source": "https://github.com/harmonium-HSP/arcfour",
        "Documentation": "https://arcfour.readthedocs.io/",
    },
)

if __name__ == "__main__":
    setup()
